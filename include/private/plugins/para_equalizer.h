/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                    FFTP_PRE
                };

                typedef struct eq_filter_t
                {
                    float              *vTrRe;          // Transfer function (real part)
                    float              *vTrIm;          // Transfer function (imaginary part)
                    size_t              nSync;          // Chart state
                    bool                bSolo;          // Soloing filter

                    plug::IPort        *pType;          // Filter type
                    plug::IPort        *pMode;          // Filter mode
                    plug::IPort        *pFreq;          // Filter frequency
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
                    size_t              nSync;          // Chart state

                    float              *vTrRe;          // Transfer function (real part)
                    float              *vTrIm;          // Transfer function (imaginary part)

                    plug::IPort        *pIn;            // Input port
                    plug::IPort        *pOut;           // Output port
                    plug::IPort        *pInGain;        // Input gain
                    plug::IPort        *pTrAmp;         // Amplitude chart
                    plug::IPort        *pPitch;         // Frequency shift
                    plug::IPort        *pFft;           // FFT chart
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
                fft_position_t      nFftPosition;           // FFT position
                core::IDBuffer     *pIDisplay;              // Inline display buffer

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

            protected:
                void                destroy_state();
                inline void         decode_filter(size_t *ftype, size_t *slope, size_t mode);
                inline bool         adjust_gain(size_t filter_type);
                inline dspu::equalizer_mode_t get_eq_mode();

                void                dump_channel(dspu::IStateDumper *v, const eq_channel_t *c) const;
                static void         dump_filter(dspu::IStateDumper *v, const eq_filter_t *f);

            public:
                explicit para_equalizer(const meta::plugin_t *metadata, size_t filters, size_t mode);
                virtual ~para_equalizer();

            public:
                virtual void        init(plug::IWrapper *wrapper, plug::IPort **ports);
                virtual void        destroy();
                virtual void        ui_activated();

                virtual void        update_settings();
                virtual void        update_sample_rate(long sr);

                virtual void        process(size_t samples);
                virtual bool        inline_display(plug::ICanvas *cv, size_t width, size_t height);

                virtual void        dump(dspu::IStateDumper *v) const;
        };

    } // namespace plugins
} // namespace lsp


#endif /* PRIVATE_PLUGINS_PARA_EQUALIZER_H_ */
