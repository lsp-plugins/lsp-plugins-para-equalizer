/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-para-equalizer
 * Created on: 2 авг. 2021 г.
 *
 * lsp-plugins-para-equalizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-para-equalizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-para-equalizer. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_PLUGINS_PARA_EQUALIZER_H_
#define PRIVATE_PLUGINS_PARA_EQUALIZER_H_

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/IDBuffer.h>
#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
#include <lsp-plug.in/dsp-units/filters/Equalizer.h>
#include <lsp-plug.in/dsp-units/util/Analyzer.h>
#include <lsp-plug.in/dsp-units/util/Delay.h>

#include <private/meta/para_equalizer.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * Parametric equalizer plugin
         */
        class para_equalizer: public plug::Module
        {
            public:
                enum eq_mode_t
                {
                    EQ_MONO,
                    EQ_STEREO,
                    EQ_LEFT_RIGHT,
                    EQ_MID_SIDE
                };

            protected:
                enum chart_state_t
                {
                    CS_UPDATE       = 1 << 0,
                    CS_SYNC_AMP     = 1 << 1
                };

                enum fft_position_t
                {
                    FFTP_NONE,
                    FFTP_POST,
                    FFTP_PRE,
                    FFTP_BOTH,
                };

                typedef struct eq_filter_t
                {
                    float              *vTrRe;          // Transfer function (real part)
                    float              *vTrIm;          // Transfer function (imaginary part)
                    size_t              nSync;          // Chart state
                    bool                bSolo;          // Soloing filter
                    dspu::filter_params_t sOldFP;       // Old filter parameters
                    dspu::filter_params_t sFP;          // Filter parameters

                    plug::IPort        *pType;          // Filter type
                    plug::IPort        *pMode;          // Filter mode
                    plug::IPort        *pFreq;          // Filter frequency
                    plug::IPort        *pWidth;         // Filter width
                    plug::IPort        *pSlope;         // Filter slope
                    plug::IPort        *pSolo;          // Solo port
                    plug::IPort        *pMute;          // Mute port
                    plug::IPort        *pGain;          // Filter gain
                    plug::IPort        *pQuality;       // Quality factor
                    plug::IPort        *pActivity;      // Filter activity flag
                    plug::IPort        *pTrAmp;         // Amplitude chart
                } eq_filter_t;

                typedef struct eq_channel_t
                {
                    dspu::Equalizer     sEqualizer;     // Equalizer
                    dspu::Bypass        sBypass;        // Bypass
                    dspu::Delay         sDryDelay;      // Dry delay

                    size_t              nLatency;       // Latency of the channel
                    float               fInGain;        // Input gain
                    float               fOutGain;       // Output gain
                    float               fPitch;         // Frequency shift
                    eq_filter_t        *vFilters;       // List of filters
                    float              *vDryBuf;        // Dry buffer
                    float              *vBuffer;        // Buffer for temporary data
                    float              *vIn;            // Input buffer
                    float              *vOut;           // Output buffer
                    float              *vAnalyzer;      // Buffer for analyzer
                    size_t              nSync;          // Chart state
                    bool                bHasSolo;       // Channel has soloing filter

                    float              *vTrRe;          // Transfer function (real part)
                    float              *vTrIm;          // Transfer function (imaginary part)

                    plug::IPort        *pIn;            // Input port
                    plug::IPort        *pOut;           // Output port
                    plug::IPort        *pInGain;        // Input gain
                    plug::IPort        *pTrAmp;         // Amplitude chart
                    plug::IPort        *pPitch;         // Frequency shift
                    plug::IPort        *pFft;           // FFT input/output switch
                    plug::IPort        *pFftInSwitch;   // FFT input switch
                    plug::IPort        *pFftOutSwitch;  // FFT output switch
                    plug::IPort        *pFftInMesh;     // FFT input mesh
                    plug::IPort        *pFftOutMesh;    // FFT output mesh
                    plug::IPort        *pVisible;       // Visibility flag
                    plug::IPort        *pInMeter;       // Output level meter
                    plug::IPort        *pOutMeter;      // Output level meter
                } eq_channel_t;

            protected:
                dspu::Analyzer      sAnalyzer;              // Analyzer
                size_t              nFilters;               // Number of filters
                size_t              nMode;                  // Operating mode
                eq_channel_t       *vChannels;              // List of channels
                float              *vFreqs;                 // Frequency list
                uint32_t           *vIndexes;               // FFT indexes
                float               fGainIn;                // Input gain
                float               fZoom;                  // Zoom gain
                bool                bListen;                // Listen mode (only for MS para_equalizer)
                bool                bSmoothMode;            // Smooth mode for the equalizer
                core::IDBuffer     *pIDisplay;              // Inline display buffer
                fft_position_t      nFftPosition;           // FFT input/output switch

                plug::IPort        *pBypass;                // Bypass port
                plug::IPort        *pGainIn;                // Input gain port
                plug::IPort        *pGainOut;               // Output gain port
                plug::IPort        *pFftMode;               // FFT mode
                plug::IPort        *pReactivity;            // FFT reactivity
                plug::IPort        *pListen;                // Listen mode (only for MS equalizer)
                plug::IPort        *pShiftGain;             // Shift gain
                plug::IPort        *pZoom;                  // Graph zoom
                plug::IPort        *pEqMode;                // Equalizer mode
                plug::IPort        *pBalance;               // Output balance
                plug::IPort        *pInspect;               // Inspected filter index
                plug::IPort        *pInspectRange;          // Inspecting range

            protected:
                static inline dspu::equalizer_mode_t get_eq_mode(ssize_t mode);
                static inline void  decode_filter(size_t *ftype, size_t *slope, size_t mode);
                static inline bool  adjust_gain(size_t filter_type);
                static bool         filter_inspect_can_be_enabled(eq_channel_t *c, eq_filter_t *f);
                static bool         filter_has_width(size_t type);

            protected:
                void                do_destroy();
                void                perform_analysis(size_t samples);
                void                process_channel(eq_channel_t *c, size_t start, size_t samples);

                void                dump_channel(dspu::IStateDumper *v, const eq_channel_t *c) const;
                static void         dump_filter(dspu::IStateDumper *v, const eq_filter_t *f);
                static void         dump_filter_params(dspu::IStateDumper *v, const char *id, const dspu::filter_params_t *fp);

            public:
                explicit para_equalizer(const meta::plugin_t *metadata, size_t filters, size_t mode);
                virtual ~para_equalizer() override;

            public:
                virtual void        init(plug::IWrapper *wrapper, plug::IPort **ports) override;
                virtual void        destroy() override;
                virtual void        ui_activated() override;
                virtual void        ui_deactivated() override;

                virtual void        update_settings() override;
                virtual void        update_sample_rate(long sr) override;

                virtual void        process(size_t samples) override;
                virtual bool        inline_display(plug::ICanvas *cv, size_t width, size_t height) override;

                virtual void        dump(dspu::IStateDumper *v) const override;
        };

    } /* namespace plugins */
} /* namespace lsp */


#endif /* PRIVATE_PLUGINS_PARA_EQUALIZER_H_ */
