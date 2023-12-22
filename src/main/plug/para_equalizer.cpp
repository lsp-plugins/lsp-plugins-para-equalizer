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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/stdlib/math.h>

#include <lsp-plug.in/shared/debug.h>
#include <lsp-plug.in/shared/id_colors.h>

#include <private/plugins/para_equalizer.h>

#define EQ_BUFFER_SIZE          0x400U
#define EQ_RANK                 12

namespace lsp
{
    namespace plugins
    {
        //-------------------------------------------------------------------------
        // Plugin factory
        typedef struct plugin_settings_t
        {
            const meta::plugin_t   *metadata;
            uint8_t                 channels;
            uint8_t                 mode;
        } plugin_settings_t;

        static const meta::plugin_t *plugins[] =
        {
            &meta::para_equalizer_x8_mono,
            &meta::para_equalizer_x8_stereo,
            &meta::para_equalizer_x8_lr,
            &meta::para_equalizer_x8_ms,
            &meta::para_equalizer_x16_mono,
            &meta::para_equalizer_x16_stereo,
            &meta::para_equalizer_x16_lr,
            &meta::para_equalizer_x16_ms,
            &meta::para_equalizer_x32_mono,
            &meta::para_equalizer_x32_stereo,
            &meta::para_equalizer_x32_lr,
            &meta::para_equalizer_x32_ms
        };

        static const plugin_settings_t plugin_settings[] =
        {
            { &meta::para_equalizer_x8_mono,     8, para_equalizer::EQ_MONO         },
            { &meta::para_equalizer_x8_stereo,   8, para_equalizer::EQ_STEREO       },
            { &meta::para_equalizer_x8_lr,       8, para_equalizer::EQ_LEFT_RIGHT   },
            { &meta::para_equalizer_x8_ms,       8, para_equalizer::EQ_MID_SIDE     },
            { &meta::para_equalizer_x16_mono,   16, para_equalizer::EQ_MONO         },
            { &meta::para_equalizer_x16_stereo, 16, para_equalizer::EQ_STEREO       },
            { &meta::para_equalizer_x16_lr,     16, para_equalizer::EQ_LEFT_RIGHT   },
            { &meta::para_equalizer_x16_ms,     16, para_equalizer::EQ_MID_SIDE     },
            { &meta::para_equalizer_x32_mono,   32, para_equalizer::EQ_MONO         },
            { &meta::para_equalizer_x32_stereo, 32, para_equalizer::EQ_STEREO       },
            { &meta::para_equalizer_x32_lr,     32, para_equalizer::EQ_LEFT_RIGHT   },
            { &meta::para_equalizer_x32_ms,     32, para_equalizer::EQ_MID_SIDE     },

            { NULL, 0, false }
        };

        static plug::Module *plugin_factory(const meta::plugin_t *meta)
        {
            for (const plugin_settings_t *s = plugin_settings; s->metadata != NULL; ++s)
                if (s->metadata == meta)
                    return new para_equalizer(s->metadata, s->channels, s->mode);
            return NULL;
        }

        static plug::Factory factory(plugin_factory, plugins, 12);

        //-------------------------------------------------------------------------
        para_equalizer::para_equalizer(const meta::plugin_t *metadata, size_t filters, size_t mode): plug::Module(metadata)
        {
            nFilters        = filters;
            nMode           = mode;
            vChannels       = NULL;
            vFreqs          = NULL;
            vIndexes        = NULL;
            fGainIn         = 1.0f;
            fZoom           = 1.0f;
            bListen         = false;
            bSmoothMode     = false;
            pIDisplay       = NULL;

            pBypass         = NULL;
            pGainIn         = NULL;
            pGainOut        = NULL;
            pReactivity     = NULL;
            pListen         = NULL;
            pShiftGain      = NULL;
            pZoom           = NULL;
            pEqMode         = NULL;
            pBalance        = NULL;
            pInspect        = NULL;
            pInspectRange   = NULL;
        }

        para_equalizer::~para_equalizer()
        {
            do_destroy();
        }

        inline void para_equalizer::decode_filter(size_t *ftype, size_t *slope, size_t mode)
        {
            #define EQF(x) meta::para_equalizer_metadata::EQF_ ## x
            #define EQS(k, t, ks) case meta::para_equalizer_metadata::EFM_ ## k:    \
                    *ftype = dspu::t; \
                    *slope = ks * *slope; \
                    return;
            #define EQDFL  default: \
                    *ftype = dspu::FLT_NONE; \
                    *slope = 1; \
                    return;

            switch (*ftype)
            {
                case EQF(BELL):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_BELL, 1)
                        EQS(RLC_MT, FLT_MT_RLC_BELL, 1)
                        EQS(BWC_BT, FLT_BT_BWC_BELL, 1)
                        EQS(BWC_MT, FLT_MT_BWC_BELL, 1)
                        EQS(LRX_BT, FLT_BT_LRX_BELL, 1)
                        EQS(LRX_MT, FLT_MT_LRX_BELL, 1)
                        EQS(APO_DR, FLT_DR_APO_PEAKING, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(HIPASS):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_HIPASS, 2)
                        EQS(RLC_MT, FLT_MT_RLC_HIPASS, 2)
                        EQS(BWC_BT, FLT_BT_BWC_HIPASS, 2)
                        EQS(BWC_MT, FLT_MT_BWC_HIPASS, 2)
                        EQS(LRX_BT, FLT_BT_LRX_HIPASS, 1)
                        EQS(LRX_MT, FLT_MT_LRX_HIPASS, 1)
                        EQS(APO_DR, FLT_DR_APO_HIPASS, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(HISHELF):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_HISHELF, 1)
                        EQS(RLC_MT, FLT_MT_RLC_HISHELF, 1)
                        EQS(BWC_BT, FLT_BT_BWC_HISHELF, 1)
                        EQS(BWC_MT, FLT_MT_BWC_HISHELF, 1)
                        EQS(LRX_BT, FLT_BT_LRX_HISHELF, 1)
                        EQS(LRX_MT, FLT_MT_LRX_HISHELF, 1)
                        EQS(APO_DR, FLT_DR_APO_HISHELF, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(LOPASS):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_LOPASS, 2)
                        EQS(RLC_MT, FLT_MT_RLC_LOPASS, 2)
                        EQS(BWC_BT, FLT_BT_BWC_LOPASS, 2)
                        EQS(BWC_MT, FLT_MT_BWC_LOPASS, 2)
                        EQS(LRX_BT, FLT_BT_LRX_LOPASS, 1)
                        EQS(LRX_MT, FLT_MT_LRX_LOPASS, 1)
                        EQS(APO_DR, FLT_DR_APO_LOPASS, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(LOSHELF):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_LOSHELF, 1)
                        EQS(RLC_MT, FLT_MT_RLC_LOSHELF, 1)
                        EQS(BWC_BT, FLT_BT_BWC_LOSHELF, 1)
                        EQS(BWC_MT, FLT_MT_BWC_LOSHELF, 1)
                        EQS(LRX_BT, FLT_BT_LRX_LOSHELF, 1)
                        EQS(LRX_MT, FLT_MT_LRX_LOSHELF, 1)
                        EQS(APO_DR, FLT_DR_APO_LOSHELF, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(NOTCH):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_NOTCH, 1)
                        EQS(RLC_MT, FLT_MT_RLC_NOTCH, 1)
                        EQS(BWC_BT, FLT_BT_RLC_NOTCH, 1)
                        EQS(BWC_MT, FLT_MT_RLC_NOTCH, 1)
                        EQS(LRX_BT, FLT_BT_RLC_NOTCH, 1)
                        EQS(LRX_MT, FLT_MT_RLC_NOTCH, 1)
                        EQS(APO_DR, FLT_DR_APO_NOTCH, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(ALLPASS):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_ALLPASS, 1)
                        EQS(RLC_MT, FLT_BT_RLC_ALLPASS, 1)
                        EQS(BWC_BT, FLT_BT_BWC_ALLPASS, 2)
                        EQS(BWC_MT, FLT_BT_BWC_ALLPASS, 2)
                        EQS(LRX_BT, FLT_BT_LRX_ALLPASS, 1)
                        EQS(LRX_MT, FLT_BT_LRX_ALLPASS, 1)
                        EQS(APO_DR, FLT_DR_APO_ALLPASS, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(RESONANCE):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_RESONANCE, 1)
                        EQS(RLC_MT, FLT_MT_RLC_RESONANCE, 1)
                        EQS(BWC_BT, FLT_BT_RLC_RESONANCE, 1)
                        EQS(BWC_MT, FLT_MT_RLC_RESONANCE, 1)
                        EQS(LRX_BT, FLT_BT_RLC_RESONANCE, 1)
                        EQS(LRX_MT, FLT_MT_RLC_RESONANCE, 1)
                        EQS(APO_DR, FLT_DR_APO_PEAKING, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(BANDPASS):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_BANDPASS, 1)
                        EQS(RLC_MT, FLT_MT_RLC_BANDPASS, 1)
                        EQS(BWC_BT, FLT_BT_BWC_BANDPASS, 1)
                        EQS(BWC_MT, FLT_MT_BWC_BANDPASS, 1)
                        EQS(LRX_BT, FLT_BT_LRX_BANDPASS, 1)
                        EQS(LRX_MT, FLT_MT_LRX_BANDPASS, 1)
                        EQS(APO_DR, FLT_DR_APO_BANDPASS, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(LADDERPASS):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_LADDERPASS, 1)
                        EQS(RLC_MT, FLT_MT_RLC_LADDERPASS, 1)
                        EQS(BWC_BT, FLT_BT_BWC_LADDERPASS, 1)
                        EQS(BWC_MT, FLT_MT_BWC_LADDERPASS, 1)
                        EQS(LRX_BT, FLT_BT_LRX_LADDERPASS, 1)
                        EQS(LRX_MT, FLT_MT_LRX_LADDERPASS, 1)
                        EQS(APO_DR, FLT_DR_APO_LADDERPASS, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(LADDERREJ):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_LADDERREJ, 1)
                        EQS(RLC_MT, FLT_MT_RLC_LADDERREJ, 1)
                        EQS(BWC_BT, FLT_BT_BWC_LADDERREJ, 1)
                        EQS(BWC_MT, FLT_MT_BWC_LADDERREJ, 1)
                        EQS(LRX_BT, FLT_BT_LRX_LADDERREJ, 1)
                        EQS(LRX_MT, FLT_MT_LRX_LADDERREJ, 1)
                        EQS(APO_DR, FLT_DR_APO_LADDERREJ, 1)
                        EQDFL
                    }
                    break;
                }

            #ifdef LSP_USE_EXPERIMENTAL
                case EQF(ALLPASS2):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_ALLPASS2, 1)
                        EQS(RLC_MT, FLT_BT_RLC_ALLPASS2, 1)
                        EQS(BWC_BT, FLT_BT_RLC_ALLPASS2, 1)
                        EQS(BWC_MT, FLT_BT_RLC_ALLPASS2, 1)
                        EQS(LRX_BT, FLT_BT_RLC_ALLPASS2, 1)
                        EQS(LRX_MT, FLT_BT_RLC_ALLPASS2, 1)
                        EQS(APO_DR, FLT_DR_APO_ALLPASS2, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(ENVELOPE):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_BT_RLC_ENVELOPE, 1)
                        EQS(RLC_MT, FLT_MT_RLC_ENVELOPE, 1)
                        EQS(BWC_BT, FLT_BT_RLC_ENVELOPE, 1)
                        EQS(BWC_MT, FLT_MT_RLC_ENVELOPE, 1)
                        EQS(LRX_BT, FLT_BT_RLC_ENVELOPE, 1)
                        EQS(LRX_MT, FLT_MT_RLC_ENVELOPE, 1)
                        EQS(APO_DR, FLT_MT_RLC_ENVELOPE, 1)
                        EQDFL
                    }
                    break;
                }

                case EQF(LUFS):
                {
                    switch (mode)
                    {
                        EQS(RLC_BT, FLT_A_WEIGHTED, 1)
                        EQS(RLC_MT, FLT_B_WEIGHTED, 1)
                        EQS(BWC_BT, FLT_C_WEIGHTED, 1)
                        EQS(BWC_MT, FLT_D_WEIGHTED, 1)
                        EQS(LRX_BT, FLT_K_WEIGHTED, 1)
                        EQS(LRX_MT, FLT_K_WEIGHTED, 1)
                        EQS(APO_DR, FLT_K_WEIGHTED, 1)
                        EQDFL
                    }
                    break;
                }
            #endif /* LSP_USE_EXPERIMENTAL */

                case EQF(OFF):
                    EQDFL;
            }
            #undef EQDFL
            #undef EQS
            #undef EQF
        }

        inline bool para_equalizer::adjust_gain(size_t filter_type)
        {
            switch (filter_type)
            {
                case dspu::FLT_NONE:

                case dspu::FLT_BT_RLC_LOPASS:
                case dspu::FLT_MT_RLC_LOPASS:
                case dspu::FLT_BT_RLC_HIPASS:
                case dspu::FLT_MT_RLC_HIPASS:
                case dspu::FLT_BT_RLC_NOTCH:
                case dspu::FLT_MT_RLC_NOTCH:
                case dspu::FLT_BT_RLC_ALLPASS:
                case dspu::FLT_MT_RLC_ALLPASS:
                case dspu::FLT_BT_RLC_ALLPASS2:
                case dspu::FLT_MT_RLC_ALLPASS2:
                case dspu::FLT_BT_RLC_BANDPASS:
                case dspu::FLT_MT_RLC_BANDPASS:

                case dspu::FLT_BT_BWC_LOPASS:
                case dspu::FLT_MT_BWC_LOPASS:
                case dspu::FLT_BT_BWC_HIPASS:
                case dspu::FLT_MT_BWC_HIPASS:
                case dspu::FLT_BT_BWC_ALLPASS:
                case dspu::FLT_MT_BWC_ALLPASS:
                case dspu::FLT_BT_BWC_BANDPASS:
                case dspu::FLT_MT_BWC_BANDPASS:

                case dspu::FLT_BT_LRX_LOPASS:
                case dspu::FLT_MT_LRX_LOPASS:
                case dspu::FLT_BT_LRX_HIPASS:
                case dspu::FLT_MT_LRX_HIPASS:
                case dspu::FLT_BT_LRX_ALLPASS:
                case dspu::FLT_MT_LRX_ALLPASS:
                case dspu::FLT_BT_LRX_BANDPASS:
                case dspu::FLT_MT_LRX_BANDPASS:

                // Disable gain adjust for several APO filters, too
                case dspu::FLT_DR_APO_NOTCH:
                case dspu::FLT_DR_APO_LOPASS:
                case dspu::FLT_DR_APO_HIPASS:
                case dspu::FLT_DR_APO_ALLPASS:
                case dspu::FLT_DR_APO_ALLPASS2:
                case dspu::FLT_DR_APO_BANDPASS:
                    return false;
                default:
                    break;
            }
            return true;
        }

        inline dspu::equalizer_mode_t para_equalizer::get_eq_mode(ssize_t mode)
        {
            switch (mode)
            {
                case meta::para_equalizer_metadata::PEM_IIR: return dspu::EQM_IIR;
                case meta::para_equalizer_metadata::PEM_FIR: return dspu::EQM_FIR;
                case meta::para_equalizer_metadata::PEM_FFT: return dspu::EQM_FFT;
                case meta::para_equalizer_metadata::PEM_SPM: return dspu::EQM_SPM;
                default:
                    break;
            }
            return dspu::EQM_BYPASS;
        }

        void para_equalizer::init(plug::IWrapper *wrapper, plug::IPort **ports)
        {
            // Pass wrapper
            plug::Module::init(wrapper, ports);

            // Determine number of channels
            size_t channels     = (nMode == EQ_MONO) ? 1 : 2;
            size_t max_latency  = 0;

            // Allocate channels
            vChannels           = new eq_channel_t[channels];
            if (vChannels == NULL)
                return;

            // Initialize global parameters
            fGainIn             = 1.0f;
            bListen             = false;

            // Allocate indexes
            vIndexes            = new uint32_t[meta::para_equalizer_metadata::MESH_POINTS];
            if (vIndexes == NULL)
                return;

            // Calculate amount of bulk data to allocate
            size_t allocate     = (2 * meta::para_equalizer_metadata::MESH_POINTS * (nFilters + 2) + EQ_BUFFER_SIZE * 3) * channels +
                                  meta::para_equalizer_metadata::MESH_POINTS;
            float *abuf         = new float[allocate];
            if (abuf == NULL)
                return;
            lsp_guard_assert(float *save = &abuf[allocate]);

            // Clear all floating-point buffers
            dsp::fill_zero(abuf, allocate);

            // Frequency list buffer
            vFreqs              = abuf;
            abuf               += meta::para_equalizer_metadata::MESH_POINTS;

            // Initialize each channel
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];

                c->nLatency         = 0;
                c->fInGain          = 1.0f;
                c->fOutGain         = 1.0f;
                c->fPitch           = 1.0f;
                c->vFilters         = NULL;
                c->vDryBuf          = advance_ptr<float>(abuf, EQ_BUFFER_SIZE);
                c->vBuffer          = advance_ptr<float>(abuf, EQ_BUFFER_SIZE);
                c->vTrRe            = advance_ptr<float>(abuf, meta::para_equalizer_metadata::MESH_POINTS);
                c->vTrIm            = advance_ptr<float>(abuf, meta::para_equalizer_metadata::MESH_POINTS);

                // Input and output ports
                c->vIn              = NULL;
                c->vOut             = NULL;

                c->vAnalyzer        = abuf;
                abuf               += EQ_BUFFER_SIZE;

                // Ports
                c->pIn              = NULL;
                c->pOut             = NULL;
                c->pInGain          = NULL;
                c->pTrAmp           = NULL;
                c->pPitch           = NULL;
                c->pFftInSwitch     = NULL;
                c->pFftOutSwitch    = NULL;
                c->pFftInMesh       = NULL;
                c->pFftOutMesh      = NULL;
                c->pVisible         = NULL;
                c->pInMeter         = NULL;
                c->pOutMeter        = NULL;
            }

            // Allocate data
            for (size_t i=0; i<channels; ++i)
            {
                // Allocate data
                eq_channel_t *c     = &vChannels[i];
                c->nSync            = CS_UPDATE;
                c->bHasSolo         = false;
                c->vFilters         = new eq_filter_t[nFilters+1];
                if (c->vFilters == NULL)
                    return;

                c->sEqualizer.init(nFilters + 1, EQ_RANK);
                c->sEqualizer.set_smooth(true);
                max_latency         = lsp_max(max_latency, c->sEqualizer.max_latency());

                // Initialize filters
                for (size_t j=0; j<=nFilters; ++j)
                {
                    eq_filter_t *f      = &c->vFilters[j];

                    // Filter characteristics
                    f->vTrRe            = advance_ptr<float>(abuf, meta::para_equalizer_metadata::MESH_POINTS);
                    f->vTrIm            = advance_ptr<float>(abuf, meta::para_equalizer_metadata::MESH_POINTS);
                    f->nSync            = CS_UPDATE;
                    f->bSolo            = false;

                    // Init filter parameters
                    f->sOldFP.nType     = dspu::FLT_NONE;
                    f->sOldFP.fFreq     = 0.0f;
                    f->sOldFP.fFreq2    = 0.0f;
                    f->sOldFP.fGain     = GAIN_AMP_0_DB;
                    f->sOldFP.nSlope    = 0;
                    f->sOldFP.fQuality  = 0.0f;

                    f->sFP.nType        = dspu::FLT_NONE;
                    f->sFP.fFreq        = 0.0f;
                    f->sFP.fFreq2       = 0.0f;
                    f->sFP.fGain        = GAIN_AMP_0_DB;
                    f->sFP.nSlope       = 0;
                    f->sFP.fQuality     = 0.0f;

                    // Additional parameters
                    f->pType            = NULL;
                    f->pMode            = NULL;
                    f->pFreq            = NULL;
                    f->pWidth           = NULL;
                    f->pGain            = NULL;
                    f->pQuality         = NULL;
                    f->pActivity        = NULL;
                    f->pTrAmp           = NULL;
                }
            }
            lsp_assert(abuf <= save);

            // Initialize latency compensation delay
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];
                if (!c->sDryDelay.init(max_latency))
                    return;
            }

            // Bind ports
            size_t port_id          = 0;

            // Bind audio ports
            lsp_trace("Binding audio ports");
            for (size_t i=0; i<channels; ++i)
                vChannels[i].pIn        =   trace_port(ports[port_id++]);
            for (size_t i=0; i<channels; ++i)
                vChannels[i].pOut       =   trace_port(ports[port_id++]);

            // Bind common ports
            lsp_trace("Binding common ports");
            pBypass                 = trace_port(ports[port_id++]);
            pGainIn                 = trace_port(ports[port_id++]);
            pGainOut                = trace_port(ports[port_id++]);
            pEqMode                 = trace_port(ports[port_id++]);
            pReactivity             = trace_port(ports[port_id++]);
            pShiftGain              = trace_port(ports[port_id++]);
            pZoom                   = trace_port(ports[port_id++]);
            trace_port(ports[port_id++]); // Skip filter selector
            pInspect                = trace_port(ports[port_id++]);
            pInspectRange           = trace_port(ports[port_id++]);
            trace_port(ports[port_id++]); // Skip auto inspect switch

            // Meters
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];

                c->pFftInSwitch         = trace_port(ports[port_id++]);
                c->pFftOutSwitch        = trace_port(ports[port_id++]);
                c->pFftInMesh           = trace_port(ports[port_id++]);
                c->pFftOutMesh          = trace_port(ports[port_id++]);
            }

            // Balance
            if (channels > 1)
                pBalance                = trace_port(ports[port_id++]);

            // Listen port
            if (nMode == EQ_MID_SIDE)
            {
                pListen                 = trace_port(ports[port_id++]);
                vChannels[0].pInGain    = trace_port(ports[port_id++]);
                vChannels[1].pInGain    = trace_port(ports[port_id++]);
            }

            for (size_t i=0; i<channels; ++i)
            {
                if ((nMode == EQ_STEREO) && (i > 0))
                {
                    vChannels[i].pTrAmp     = NULL;
                    vChannels[i].pPitch     = vChannels[i-1].pPitch;
                }
                else
                {
                    vChannels[i].pTrAmp     = trace_port(ports[port_id++]);
                    vChannels[i].pPitch     = trace_port(ports[port_id++]);
                }
                vChannels[i].pInMeter   =   trace_port(ports[port_id++]);
                vChannels[i].pOutMeter  =   trace_port(ports[port_id++]);

                if ((nMode == EQ_LEFT_RIGHT) || (nMode == EQ_MID_SIDE))
                    vChannels[i].pVisible   = trace_port(ports[port_id++]); // Skip eq curve visibility
                else
                    vChannels[i].pVisible   = NULL;
            }

            // Bind filters
            lsp_trace("Binding filter ports");

            for (size_t i=0; i<nFilters; ++i)
            {
                for (size_t j=0; j<channels; ++j)
                {
                    eq_filter_t *f      = &vChannels[j].vFilters[i];

                    if ((nMode == EQ_STEREO) && (j > 0))
                    {
                        // 1 port controls 2 filters
                        eq_filter_t *sf     = &vChannels[0].vFilters[i];
                        f->pType            = sf->pType;
                        f->pMode            = sf->pMode;
                        f->pSlope           = sf->pSlope;
                        f->pSolo            = sf->pSolo;
                        f->pMute            = sf->pMute;
                        f->pFreq            = sf->pFreq;
                        f->pWidth           = sf->pWidth;
                        f->pGain            = sf->pGain;
                        f->pQuality         = sf->pQuality;
                        f->pActivity        = sf->pActivity;
                        f->pTrAmp           = NULL;
                    }
                    else
                    {
                        // 1 port controls 1 filter
                        f->pType        = trace_port(ports[port_id++]);
                        f->pMode        = trace_port(ports[port_id++]);
                        f->pSlope       = trace_port(ports[port_id++]);
                        f->pSolo        = trace_port(ports[port_id++]);
                        f->pMute        = trace_port(ports[port_id++]);
                        f->pFreq        = trace_port(ports[port_id++]);
                        f->pWidth       = trace_port(ports[port_id++]);
                        f->pGain        = trace_port(ports[port_id++]);
                        f->pQuality     = trace_port(ports[port_id++]);
                        trace_port(ports[port_id++]); // Skip hue
                        f->pActivity    = trace_port(ports[port_id++]);
                        f->pTrAmp       = trace_port(ports[port_id++]);
                    }
                }
            }
        }

        void para_equalizer::ui_activated()
        {
            size_t channels     = ((nMode == EQ_MONO) || (nMode == EQ_STEREO)) ? 1 : 2;
            for (size_t i=0; i<channels; ++i)
            {
                for (size_t j=0; j<=nFilters; ++j)
                    vChannels[i].vFilters[j].nSync = CS_UPDATE;
                vChannels[i].nSync = CS_UPDATE;
            }
            pWrapper->request_settings_update();
        }

        void para_equalizer::ui_deactivated()
        {
            pWrapper->request_settings_update();
        }

        void para_equalizer::destroy()
        {
            do_destroy();
        }

        void para_equalizer::do_destroy()
        {
            size_t channels     = (nMode == EQ_MONO) ? 1 : 2;

            // Delete channels
            if (vChannels != NULL)
            {
                for (size_t i=0; i<channels; ++i)
                {
                    eq_channel_t *c     = &vChannels[i];
                    if (c->vFilters != NULL)
                    {
                        delete [] c->vFilters;
                        c->vFilters         = NULL;
                    }
                }

                delete [] vChannels;
                vChannels = NULL;
            }

            if (vIndexes != NULL)
            {
                delete [] vIndexes;
                vIndexes    = NULL;
            }

            // Delete frequencies
            if (vFreqs != NULL)
            {
                delete [] vFreqs;
                vFreqs = NULL;
            }

            if (pIDisplay != NULL)
            {
                pIDisplay->destroy();
                pIDisplay   = NULL;
            }

            // Destroy analyzer
            sAnalyzer.destroy();
        }

        bool para_equalizer::filter_inspect_can_be_enabled(eq_channel_t *c, eq_filter_t *f)
        {
            if (f == NULL)
                return false;

            bool mute       = f->pMute->value() >= 0.5f;
            if ((mute) || ((c->bHasSolo) && (!f->bSolo)))
                return false;

            // The filter should be enabled
            return size_t(f->pType->value()) != meta::para_equalizer_metadata::EQF_OFF;
        }

        bool para_equalizer::filter_has_width(size_t type)
        {
            switch (type)
            {
                case dspu::FLT_BT_RLC_BANDPASS:
                case dspu::FLT_MT_RLC_BANDPASS:
                case dspu::FLT_BT_BWC_BANDPASS:
                case dspu::FLT_MT_BWC_BANDPASS:
                case dspu::FLT_BT_LRX_BANDPASS:
                case dspu::FLT_MT_LRX_BANDPASS:
                case dspu::FLT_BT_RLC_LADDERPASS:
                case dspu::FLT_MT_RLC_LADDERPASS:
                case dspu::FLT_BT_RLC_LADDERREJ:
                case dspu::FLT_MT_RLC_LADDERREJ:
                case dspu::FLT_BT_BWC_LADDERPASS:
                case dspu::FLT_MT_BWC_LADDERPASS:
                case dspu::FLT_BT_BWC_LADDERREJ:
                case dspu::FLT_MT_BWC_LADDERREJ:
                case dspu::FLT_BT_LRX_LADDERPASS:
                case dspu::FLT_MT_LRX_LADDERPASS:
                case dspu::FLT_BT_LRX_LADDERREJ:
                case dspu::FLT_MT_LRX_LADDERREJ:
                case dspu::FLT_DR_APO_LADDERPASS:
                case dspu::FLT_DR_APO_LADDERREJ:
                    return true;
            }

            return false;
        }

        void para_equalizer::update_settings()
        {
            // Check sample rate
            if (fSampleRate <= 0)
                return;

            // Update common settings
            if (pGainIn != NULL)
                fGainIn     = pGainIn->value();
            if (pZoom != NULL)
            {
                float zoom  = pZoom->value();
                if (zoom != fZoom)
                {
                    fZoom       = zoom;
                    pWrapper->query_display_draw();
                }
            }

            // Calculate balance
            float bal[2] = { 1.0f, 1.0f };
            if (pBalance != NULL)
            {
                float xbal      = pBalance->value();
                bal[0]          = (100.0f - xbal) * 0.01f;
                bal[1]          = (xbal + 100.0f) * 0.01f;
            }
            if (pGainOut != NULL)
            {
                float out_gain  = pGainOut->value();
                bal[0]         *= out_gain;
                bal[1]         *= out_gain;
            }

            // Listen
            if (pListen != NULL)
                bListen     = pListen->value() >= 0.5f;

            size_t channels     = (nMode == EQ_MONO) ? 1 : 2;

            // Configure analyzer
            size_t n_an_channels = 0;
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];
                bool in_fft         = c->pFftInSwitch->value() >= 0.5f;
                bool out_fft        = c->pFftOutSwitch->value() >= 0.5f;

                // channel:        0     1     2      3
                // designation: in_l out_l  in_r  out_r
                sAnalyzer.enable_channel(i*2, in_fft);
                sAnalyzer.enable_channel(i*2+1, out_fft);
                if ((in_fft) || (out_fft))
                    ++n_an_channels;
            }

            // Update reactivity
            sAnalyzer.set_activity(n_an_channels > 0);
            sAnalyzer.set_reactivity(pReactivity->value());

            // Update shift gain
            if (pShiftGain != NULL)
                sAnalyzer.set_shift(pShiftGain->value() * 100.0f);

            // Process the 'Solo' button
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];
                c->bHasSolo         = false;

                // Update each filter configuration except the inspection ones
                for (size_t j=0; j<nFilters; ++j)
                {
                    eq_filter_t *f      = &c->vFilters[j];
                    f->bSolo            = f->pSolo->value() >= 0.5f;
                    if (f->bSolo)
                        c->bHasSolo         = true;
                }
            }

            // Check that inspection mode is ON
            ssize_t i_value         = (ui_active()) ? ssize_t(pInspect->value()) : -1;

            size_t i_channel        = i_value / ssize_t(nFilters);
            size_t i_filter         = i_value % ssize_t(nFilters);
            if ((i_value >= 0) && (i_channel < channels))
            {
                eq_channel_t *c     = &vChannels[i_channel];
                if (!filter_inspect_can_be_enabled(c, &c->vFilters[i_filter]))
                    i_value             = -1;
            }
            else
                i_value             = -1;

            // Update equalizer mode
            dspu::equalizer_mode_t eq_mode  = get_eq_mode(pEqMode->value());
            bool bypass                     = pBypass->value() >= 0.5f;
            bool mode_changed               = false;
            bSmoothMode                     = false;

            // For each channel
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];
                bool visible        = (c->pVisible == NULL) ?  true : (c->pVisible->value() >= 0.5f);

                // Change the operating mode for the equalizer
                if (c->sEqualizer.mode() != eq_mode)
                {
                    c->sEqualizer.set_mode(eq_mode);
                    mode_changed        = true;
                }

                // Update settings
                if (c->sBypass.set_bypass(bypass))
                    pWrapper->query_display_draw();
                c->fOutGain         = bal[i];
                c->fInGain          = (c->pInGain != NULL) ? c->pInGain->value() : 1.0f;
                c->fPitch           = dspu::semitones_to_frequency_shift(c->pPitch->value());

                // Update each filter configuration depending on solo except the inspection one
                for (size_t j=0; j<nFilters; ++j)
                {
                    eq_filter_t *f      = &c->vFilters[j];
                    f->sOldFP           = f->sFP;
                    dspu::filter_params_t *fp = &f->sFP;
                    dspu::filter_params_t *op = &f->sOldFP;

                    // Compute filter params
                    bool mute           = f->pMute->value() >= 0.5f;
                    if ((mute) || ((c->bHasSolo) && (!f->bSolo)))
                    {
                        fp->nType           = dspu::FLT_NONE;
                        fp->nSlope          = 1;
                    }
                    else if (i_value >= 0)
                    {
                        if ((j != i_filter) ||
                            ((i != i_channel) && ((nMode == EQ_LEFT_RIGHT) || (nMode == EQ_MID_SIDE))))
                        {
                            fp->nType           = dspu::FLT_NONE;
                            fp->nSlope          = 1;
                        }
                        else
                        {
                            fp->nType           = f->pType->value();
                            fp->nSlope          = f->pSlope->value() + 1;
                            decode_filter(&fp->nType, &fp->nSlope, f->pMode->value());
                        }
                    }
                    else
                    {
                        fp->nType     		= f->pType->value();
                        fp->nSlope          = f->pSlope->value() + 1;
                        decode_filter(&fp->nType, &fp->nSlope, f->pMode->value());
                    }

                    if (filter_has_width(fp->nType))
                    {
                        float center        = f->pFreq->value() * c->fPitch;
                        float k             = powf(2, (f->pWidth->value() * 0.5f));
                        fp->fFreq           = center/k;
                        fp->fFreq2          = center*k;
                    }
                    else
                    {
                        fp->fFreq           = f->pFreq->value() * c->fPitch;
                        fp->fFreq2          = fp->fFreq;
                    }

                    fp->fGain           = (adjust_gain(fp->nType)) ? f->pGain->value() : 1.0f;
                    fp->fQuality        = f->pQuality->value();

                    c->sEqualizer.limit_params(j, fp);
                    bool type_changed   =
                        (fp->nType != op->nType) ||
                        (fp->nSlope != op->nSlope);
                    bool param_changed  =
                        (fp->fGain != op->fGain) ||
                        (fp->fFreq != op->fFreq) ||
                        (fp->fFreq2 != op->fFreq2) ||
                        (fp->fQuality != op->fQuality);

                    // Apply filter params if theey have changed
                    if ((type_changed) || (param_changed))
                    {
                        c->sEqualizer.set_params(j, fp);
                        f->nSync            = CS_UPDATE;

                        if (type_changed)
                            mode_changed    = true;
                        if (param_changed)
                            bSmoothMode     = true;
                    }

                    // Output filter activity
                    if (f->pActivity != NULL)
                        f->pActivity->set_value(((visible) && (fp->nType != dspu::FLT_NONE)) ? 1.0f : 0.0f);
                }

                // Update the settings of the inspection filter
                {
                    eq_filter_t *f      = &c->vFilters[nFilters];
                    f->sOldFP           = f->sFP;
                    dspu::filter_params_t *fp = &f->sFP;
                    dspu::filter_params_t *op = &f->sOldFP;
                    float f_range       = expf(M_LN2 * 0.5f * pInspectRange->value());
                    size_t xi_channel   = ((nMode == EQ_LEFT_RIGHT) || (nMode == EQ_MID_SIDE)) ? i : i_channel;

                    // Set filter parameters (for mono and stereo do it for both channels)
                    if ((i_value >= 0) && (xi_channel == i_channel))
                    {
                        eq_filter_t *sf     = &c->vFilters[i_filter];
                        float f1            = sf->sFP.fFreq / f_range;
                        float f2            = sf->sFP.fFreq * f_range;
                        fp->fGain           = 1.0f;

                        switch (ssize_t(sf->pType->value()))
                        {
                            case meta::para_equalizer_metadata::EQF_BELL:
                            case meta::para_equalizer_metadata::EQF_RESONANCE:
                            case meta::para_equalizer_metadata::EQF_NOTCH:
                                fp->nType       = dspu::FLT_BT_BWC_BANDPASS;
                                fp->fFreq       = f1;
                                fp->fFreq2      = f2;
                                fp->nSlope      = 4;
                                fp->fQuality    = 0.707f;
                                break;

                            case meta::para_equalizer_metadata::EQF_HISHELF:
                                fp->nType       = dspu::FLT_BT_BWC_HIPASS;
                                fp->fFreq       = f1;
                                fp->fFreq2      = f1;
                                fp->nSlope      = 8;
                                fp->fQuality    = 0.707f;
                                break;

                            case meta::para_equalizer_metadata::EQF_LOSHELF:
                                fp->nType       = dspu::FLT_BT_BWC_LOPASS;
                                fp->fFreq       = f2;
                                fp->fFreq2      = f2;
                                fp->nSlope      = 8;
                                fp->fQuality    = 0.707f;
                                break;

                            default:
                                fp->nType       = dspu::FLT_NONE;
                                fp->nSlope      = 1;
                                fp->fQuality    = 0.0f;
                                break;
                        }
                    }
                    else if (i_value >= 0)
                    {
                        c->fInGain      = 0.0f; // Mute the channel without the inspected filter
                        fp->nType       = dspu::FLT_NONE;
                        fp->nSlope      = 1;
                    }
                    else
                    {
                        fp->nType       = dspu::FLT_NONE;
                        fp->nSlope      = 1;
                    }

                    // Update the filter settings
                    c->sEqualizer.limit_params(nFilters, fp);
                    bool type_changed   =
                        (fp->nType != op->nType) ||
                        (fp->nSlope != op->nSlope);
                    bool param_changed  =
                        (fp->fGain != op->fGain) ||
                        (fp->fFreq != op->fFreq) ||
                        (fp->fFreq2 != op->fFreq2) ||
                        (fp->fQuality != op->fQuality);

                    // Apply filter params if theey have changed
                    if ((type_changed) || (param_changed))
                    {
                        c->sEqualizer.set_params(nFilters, fp);
                        f->nSync            = CS_UPDATE;

                        if (type_changed)
                            mode_changed    = true;
                        if (param_changed)
                            bSmoothMode     = true;
                    }
                } /* ivalue */
            }

            // Do not enable smooth mode if significant changes have been applied
            if ((mode_changed) || (eq_mode != dspu::EQM_IIR))
                bSmoothMode             = false;

            // Update analyzer
            if (sAnalyzer.needs_reconfiguration())
            {
                sAnalyzer.reconfigure();
                sAnalyzer.get_frequencies(vFreqs, vIndexes, SPEC_FREQ_MIN, SPEC_FREQ_MAX, meta::para_equalizer_metadata::MESH_POINTS);
            }

            // Update latency
            size_t latency          = 0;
            for (size_t i=0; i<channels; ++i)
                latency                 = lsp_max(latency, vChannels[i].sEqualizer.get_latency());

            for (size_t i=0; i<channels; ++i)
            {
                vChannels[i].sDryDelay.set_delay(latency);
                sAnalyzer.set_channel_delay(i*2, latency);
            }
            set_latency(latency);
        }

        void para_equalizer::update_sample_rate(long sr)
        {
            size_t channels     = (nMode == EQ_MONO) ? 1 : 2;

            sAnalyzer.set_sample_rate(sr);
            size_t max_latency  = 1 << (meta::para_equalizer_metadata::FFT_RANK + 1);

            // Initialize channels
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];
                c->sBypass.init(sr);
                c->sEqualizer.set_sample_rate(sr);
            }

            // Initialize analyzer
            if (!sAnalyzer.init(channels*2, meta::para_equalizer_metadata::FFT_RANK,
                                sr, meta::para_equalizer_metadata::REFRESH_RATE,
                                max_latency))
                return;

            sAnalyzer.set_sample_rate(sr);
            sAnalyzer.set_rank(meta::para_equalizer_metadata::FFT_RANK);
            sAnalyzer.set_activity(false);
            sAnalyzer.set_envelope(meta::para_equalizer_metadata::FFT_ENVELOPE);
            sAnalyzer.set_window(meta::para_equalizer_metadata::FFT_WINDOW);
            sAnalyzer.set_rate(meta::para_equalizer_metadata::REFRESH_RATE);
        }

        void para_equalizer::perform_analysis(size_t samples)
        {
            // Prepare processing
            size_t channels     = (nMode == EQ_MONO) ? 1 : 2;

            const float *bufs[4] = { NULL, NULL, NULL, NULL };
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c         = &vChannels[i];
                bufs[i*2]               = c->vAnalyzer;
                bufs[i*2+1]             = c->vBuffer;
            }

            // Perform FFT analysis
            sAnalyzer.process(bufs, samples);
        }

        void para_equalizer::process_channel(eq_channel_t *c, size_t start, size_t samples)
        {
            // Process the signal by the equalizer
            if (bSmoothMode)
            {
                float den   = 1.0f / samples;

                // In smooth mode, we need to update filter parameters for each sample
                for (size_t offset=0; offset<samples; ++offset)
                {
                    // Tune the filters
                    float k                     = float(start + offset) * den;
                    for (size_t j=0; j<=nFilters; ++j)
                    {
                        eq_filter_t *f              = &c->vFilters[j];
                        dspu::filter_params_t fp;

                        fp.nType                    = f->sFP.nType;
                        fp.fFreq                    = f->sOldFP.fFreq * expf(logf(f->sFP.fFreq/f->sOldFP.fFreq)*k);
                        fp.fFreq2                   = f->sOldFP.fFreq2 * expf(logf(f->sFP.fFreq2/f->sOldFP.fFreq2)*k);
                        fp.nSlope                   = f->sFP.nSlope;
                        fp.fGain                    = f->sOldFP.fGain * expf(logf(f->sFP.fGain/f->sOldFP.fGain)*k);
                        fp.fQuality                 = f->sOldFP.fQuality + (f->sFP.fQuality -f->sOldFP.fQuality)*k;

                        c->sEqualizer.set_params(j, &fp);
                    }

                    // Apply processing
                    c->sEqualizer.process(&c->vBuffer[offset], &c->vBuffer[offset], 1);
                }
            }
            else
                c->sEqualizer.process(c->vBuffer, c->vBuffer, samples);

            if (c->fInGain != 1.0f)
                dsp::mul_k2(c->vBuffer, c->fInGain, samples);
        }

        void para_equalizer::process(size_t samples)
        {
            size_t channels     = (nMode == EQ_MONO) ? 1 : 2;

            // Initialize buffer pointers
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];
                c->vIn              = c->pIn->buffer<float>();
                c->vOut             = c->pOut->buffer<float>();
            }

            for (size_t offset = 0; offset < samples; )
            {
                // Determine buffer size for processing
                size_t to_process   = lsp_min(samples-offset, EQ_BUFFER_SIZE);

                // Store unprocessed data
                for (size_t i=0; i<channels; ++i)
                {
                    eq_channel_t *c     = &vChannels[i];
                    c->sDryDelay.process(c->vDryBuf, c->vIn, to_process);
                }

                // Pre-process data
                if (nMode == EQ_MID_SIDE)
                {
                    if (!bListen)
                    {
                        vChannels[0].pInMeter->set_value(dsp::abs_max(vChannels[0].vIn, to_process));
                        vChannels[1].pInMeter->set_value(dsp::abs_max(vChannels[1].vIn, to_process));
                    }
                    dsp::lr_to_ms(vChannels[0].vBuffer, vChannels[1].vBuffer, vChannels[0].vIn, vChannels[1].vIn, to_process);
                    if (bListen)
                    {
                        vChannels[0].pInMeter->set_value(dsp::abs_max(vChannels[0].vBuffer, to_process));
                        vChannels[1].pInMeter->set_value(dsp::abs_max(vChannels[1].vBuffer, to_process));
                    }
                    if (fGainIn != 1.0f)
                    {
                        dsp::mul_k2(vChannels[0].vBuffer, fGainIn, to_process);
                        dsp::mul_k2(vChannels[1].vBuffer, fGainIn, to_process);
                    }
                }
                else if (nMode == EQ_MONO)
                {
                    vChannels[0].pInMeter->set_value(dsp::abs_max(vChannels[0].vIn, to_process));
                    if (fGainIn != 1.0f)
                        dsp::mul_k3(vChannels[0].vBuffer, vChannels[0].vIn, fGainIn, to_process);
                    else
                        dsp::copy(vChannels[0].vBuffer, vChannels[0].vIn, to_process);
                }
                else
                {
                    vChannels[0].pInMeter->set_value(dsp::abs_max(vChannels[0].vIn, to_process));
                    vChannels[1].pInMeter->set_value(dsp::abs_max(vChannels[1].vIn, to_process));
                    if (fGainIn != 1.0f)
                    {
                        dsp::mul_k3(vChannels[0].vBuffer, vChannels[0].vIn, fGainIn, to_process);
                        dsp::mul_k3(vChannels[1].vBuffer, vChannels[1].vIn, fGainIn, to_process);
                    }
                    else
                    {
                        dsp::copy(vChannels[0].vBuffer, vChannels[0].vIn, to_process);
                        dsp::copy(vChannels[1].vBuffer, vChannels[1].vIn, to_process);
                    }
                }

                // Store data for analysis
                for (size_t i=0; i<channels; ++i)
                {
                    eq_channel_t *c     = &vChannels[i];
                    if (sAnalyzer.channel_active(i*2))
                        dsp::copy(c->vAnalyzer, c->vBuffer, to_process);
                }

                // Process each channel individually
                for (size_t i=0; i<channels; ++i)
                    process_channel(&vChannels[i], offset, to_process);

                // Call analyzer
                perform_analysis(to_process);

                // Post-process data (if needed)
                if ((nMode == EQ_MID_SIDE) && (!bListen))
                    dsp::ms_to_lr(vChannels[0].vBuffer, vChannels[1].vBuffer, vChannels[0].vBuffer, vChannels[1].vBuffer, to_process);

                // Process data via bypass
                for (size_t i=0; i<channels; ++i)
                {
                    eq_channel_t *c     = &vChannels[i];

                    // Apply output gain
                    if (c->fOutGain != 1.0f)
                        dsp::mul_k2(c->vBuffer, c->fOutGain, to_process);

                    // Do output metering
                    if (c->pOutMeter != NULL)
                        c->pOutMeter->set_value(dsp::abs_max(c->vBuffer, to_process));

                    // Process via bypass
                    c->sBypass.process(c->vOut, c->vDryBuf, c->vBuffer, to_process);

                    c->vIn             += to_process;
                    c->vOut            += to_process;
                }

                // Update offset
                offset             += to_process;
            } // for offset

            // Output FFT curves for each channel and report latency
            size_t latency          = 0;

            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];

                if (latency < c->sEqualizer.get_latency())
                    latency         = c->sEqualizer.get_latency();

                // Input FFT mesh
                plug::mesh_t *mesh          = c->pFftInMesh->buffer<plug::mesh_t>();
                if ((mesh != NULL) && (mesh->isEmpty()))
                {
                    // Add extra points
                    mesh->pvData[0][0] = SPEC_FREQ_MIN * 0.5f;
                    mesh->pvData[0][meta::para_equalizer_metadata::MESH_POINTS+1] = SPEC_FREQ_MAX * 2.0f;
                    mesh->pvData[1][0] = 0.0f;
                    mesh->pvData[1][meta::para_equalizer_metadata::MESH_POINTS+1] = 0.0f;

                    // Copy frequency points
                    dsp::copy(&mesh->pvData[0][1], vFreqs, meta::para_equalizer_metadata::MESH_POINTS);
                    sAnalyzer.get_spectrum(i*2, &mesh->pvData[1][1], vIndexes, meta::para_equalizer_metadata::MESH_POINTS);

                    // Mark mesh containing data
                    mesh->data(2, meta::para_equalizer_metadata::MESH_POINTS+2);
                }

                // Output FFT mesh
                mesh                        = c->pFftOutMesh->buffer<plug::mesh_t>();
                if ((mesh != NULL) && (mesh->isEmpty()))
                {
                    // Copy frequency points
                    dsp::copy(mesh->pvData[0], vFreqs, meta::para_equalizer_metadata::MESH_POINTS);
                    sAnalyzer.get_spectrum(i*2+1, mesh->pvData[1], vIndexes, meta::para_equalizer_metadata::MESH_POINTS);

                    // Mark mesh containing data
                    mesh->data(2, meta::para_equalizer_metadata::MESH_POINTS);
                }
            }

            set_latency(latency);

            // For Mono and Stereo channels only the first channel should be processed
            if (nMode == EQ_STEREO)
                channels        = 1;

            // Sync meshes
            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c     = &vChannels[i];

                // Synchronize filters
                for (size_t j=0; j<=nFilters; ++j)
                {
                    // Update transfer chart of the filter
                    eq_filter_t *f  = &c->vFilters[j];
                    if (f->nSync & CS_UPDATE)
                    {
                        c->sEqualizer.freq_chart(j, f->vTrRe, f->vTrIm, vFreqs, meta::para_equalizer_metadata::MESH_POINTS);
                        f->nSync    = CS_SYNC_AMP;
                        c->nSync    = CS_UPDATE;
                    }

                    // Output amplification curve
                    if ((f->pTrAmp != NULL) && (f->nSync & CS_SYNC_AMP))
                    {
                        plug::mesh_t *mesh  = f->pTrAmp->buffer<plug::mesh_t>();
                        if ((mesh != NULL) && (mesh->isEmpty()))
                        {
                            if (c->sEqualizer.filter_active(j))
                            {
                                // Add extra points
                                mesh->pvData[0][0] = SPEC_FREQ_MIN*0.5f;
                                mesh->pvData[0][meta::para_equalizer_metadata::MESH_POINTS+1] = SPEC_FREQ_MAX*2.0;
                                mesh->pvData[1][0] = 1.0f;
                                mesh->pvData[1][meta::para_equalizer_metadata::MESH_POINTS+1] = 1.0f;

                                // Fill mesh
                                dsp::copy(&mesh->pvData[0][1], vFreqs, meta::para_equalizer_metadata::MESH_POINTS);
                                dsp::complex_mod(&mesh->pvData[1][1], f->vTrRe, f->vTrIm, meta::para_equalizer_metadata::MESH_POINTS);

                                mesh->data(2, meta::para_equalizer_metadata::FILTER_MESH_POINTS);
                            }
                            else
                                mesh->data(2, 0);

                            f->nSync           &= ~CS_SYNC_AMP;
                        }
                    }
                }

                // Synchronize main transfer function of the channel
                if (c->nSync & CS_UPDATE)
                {
                    // Initialize complex numbers for transfer function
                    dsp::fill_one(c->vTrRe, meta::para_equalizer_metadata::MESH_POINTS);
                    dsp::fill_zero(c->vTrIm, meta::para_equalizer_metadata::MESH_POINTS);

                    for (size_t j=0; j<=nFilters; ++j)
                    {
                        eq_filter_t *f  = &c->vFilters[j];
                        dsp::complex_mul2(c->vTrRe, c->vTrIm, f->vTrRe, f->vTrIm, meta::para_equalizer_metadata::MESH_POINTS);
                    }
                    c->nSync    = CS_SYNC_AMP;
                }

                // Output amplification curve
                if ((c->pTrAmp != NULL) && (c->nSync & CS_SYNC_AMP))
                {
                    // Sync mesh
                    plug::mesh_t *mesh  = c->pTrAmp->buffer<plug::mesh_t>();
                    if ((mesh != NULL) && (mesh->isEmpty()))
                    {
                        dsp::copy(mesh->pvData[0], vFreqs, meta::para_equalizer_metadata::MESH_POINTS);
                        dsp::complex_mod(mesh->pvData[1], c->vTrRe, c->vTrIm, meta::para_equalizer_metadata::MESH_POINTS);
                        mesh->data(2, meta::para_equalizer_metadata::MESH_POINTS);

                        c->nSync           &= ~CS_SYNC_AMP;
                    }

                    // Request for redraw
                    if (pWrapper != NULL)
                        pWrapper->query_display_draw();
                }
            }

            // Reset smooth mode
            if (bSmoothMode)
            {
                // Apply actual settings of equalizer at the end
                for (size_t i=0; i<channels; ++i)
                {
                    eq_channel_t *c     = &vChannels[i];
                    for (size_t j=0; j<=nFilters; ++j)
                        c->sEqualizer.set_params(j, &c->vFilters[j].sFP);
                }

                bSmoothMode     = false;
            }
        }

        bool para_equalizer::inline_display(plug::ICanvas *cv, size_t width, size_t height)
        {
            // Check proportions
            if (height > (M_RGOLD_RATIO * width))
                height  = M_RGOLD_RATIO * width;

            // Init canvas
            if (!cv->init(width, height))
                return false;
            width   = cv->width();
            height  = cv->height();

            // Clear background
            bool bypassing = vChannels[0].sBypass.bypassing();
            cv->set_color_rgb((bypassing) ? CV_DISABLED : CV_BACKGROUND);
            cv->paint();

            // Draw axis
            cv->set_line_width(1.0);

            float zx    = 1.0f/SPEC_FREQ_MIN;
            float zy    = fZoom/GAIN_AMP_M_48_DB;
            float dx    = width/(logf(SPEC_FREQ_MAX)-logf(SPEC_FREQ_MIN));
            float dy    = height/(logf(GAIN_AMP_M_48_DB/fZoom)-logf(GAIN_AMP_P_48_DB*fZoom));

            // Draw vertical lines
            cv->set_color_rgb(CV_YELLOW, 0.5f);
            for (float i=100.0f; i<SPEC_FREQ_MAX; i *= 10.0f)
            {
                float ax = dx*(logf(i*zx));
                cv->line(ax, 0, ax, height);
            }

            // Draw horizontal lines
            cv->set_color_rgb(CV_WHITE, 0.5f);
            for (float i=GAIN_AMP_M_48_DB; i<GAIN_AMP_P_48_DB; i *= GAIN_AMP_P_12_DB)
            {
                float ay = height + dy*(logf(i*zy));
                cv->line(0, ay, width, ay);
            }

            // Allocate buffer: f, x, y, re, im
            pIDisplay           = core::IDBuffer::reuse(pIDisplay, 5, width+2);
            core::IDBuffer *b   = pIDisplay;
            if (b == NULL)
                return false;

            // Initialize mesh
            b->v[0][0]          = SPEC_FREQ_MIN*0.5f;
            b->v[0][width+1]    = SPEC_FREQ_MAX*2.0f;
            b->v[3][0]          = 1.0f;
            b->v[3][width+1]    = 1.0f;
            b->v[4][0]          = 0.0f;
            b->v[4][width+1]    = 0.0f;

            size_t channels = ((nMode == EQ_MONO) || (nMode == EQ_STEREO)) ? 1 : 2;
            static uint32_t c_colors[] =
            {
                CV_MIDDLE_CHANNEL, CV_MIDDLE_CHANNEL,
                CV_MIDDLE_CHANNEL, CV_MIDDLE_CHANNEL,
                CV_LEFT_CHANNEL, CV_RIGHT_CHANNEL,
                CV_MIDDLE_CHANNEL, CV_SIDE_CHANNEL
            };

            bool aa = cv->set_anti_aliasing(true);
            cv->set_line_width(2);

            for (size_t i=0; i<channels; ++i)
            {
                eq_channel_t *c = &vChannels[i];

                for (size_t j=0; j<width; ++j)
                {
                    size_t k        = (j*meta::para_equalizer_metadata::MESH_POINTS)/width;
                    b->v[0][j+1]    = vFreqs[k];
                    b->v[3][j+1]    = c->vTrRe[k];
                    b->v[4][j+1]    = c->vTrIm[k];
                }

                dsp::complex_mod(b->v[3], b->v[3], b->v[4], width+2);
                dsp::fill(b->v[1], 0.0f, width+2);
                dsp::fill(b->v[2], height, width+2);
                dsp::axis_apply_log1(b->v[1], b->v[0], zx, dx, width+2);
                dsp::axis_apply_log1(b->v[2], b->v[3], zy, dy, width+2);

                // Draw mesh
                uint32_t color = (bypassing || !(active())) ? CV_SILVER : c_colors[nMode*2 + i];
                Color stroke(color), fill(color, 0.5f);
                cv->draw_poly(b->v[1], b->v[2], width+2, stroke, fill);
            }
            cv->set_anti_aliasing(aa);

            return true;
        }

        void para_equalizer::dump_filter_params(dspu::IStateDumper *v, const char *id, const dspu::filter_params_t *fp)
        {
            v->begin_object(id, fp, sizeof(*fp));
            {
                v->write("nType", fp->nType);
                v->write("fFreq", fp->fFreq);
                v->write("fFreq2", fp->fFreq2);
                v->write("fGain", fp->fGain);
                v->write("nSlope", fp->nSlope);
                v->write("fQuality", fp->fQuality);
            }
            v->end_object();
        }

        void para_equalizer::dump_filter(dspu::IStateDumper *v, const eq_filter_t *f)
        {
            v->begin_object(f, sizeof(eq_filter_t));
            {
                v->write("vTrRe", f->vTrRe);
                v->write("vTrIm", f->vTrIm);
                v->write("nSync", f->nSync);
                v->write("bSolo", f->bSolo);

                dump_filter_params(v, "sOldFP", &f->sOldFP);
                dump_filter_params(v, "sFP", &f->sFP);

                v->write("pType", f->pType);
                v->write("pMode", f->pMode);
                v->write("pFreq", f->pFreq);
                v->write("pSlope", f->pSlope);
                v->write("pSolo", f->pSolo);
                v->write("pMute", f->pMute);
                v->write("pGain", f->pGain);
                v->write("pQuality", f->pQuality);
                v->write("pActivity", f->pActivity);
                v->write("pTrAmp", f->pTrAmp);
            }
            v->end_object();
        }

        void para_equalizer::dump_channel(dspu::IStateDumper *v, const eq_channel_t *c) const
        {
            v->begin_object(c, sizeof(eq_channel_t));
            {
                v->write_object("sEqualizer", &c->sEqualizer);
                v->write_object("sBypass", &c->sBypass);
                v->write_object("sDryDelay", &c->sDryDelay);

                v->write("nLatency", c->nLatency);
                v->write("fInGain", c->fInGain);
                v->write("fOutGain", c->fOutGain);
                v->write("fPitch", c->fPitch);
                v->begin_array("vFilters", c->vFilters, nFilters+1);
                {
                    for (size_t i=0; i<=nFilters; ++i)
                        dump_filter(v, &c->vFilters[i]);
                }
                v->end_array();
                v->write("vDryBuf", c->vDryBuf);
                v->write("vBuffer", c->vBuffer);
                v->write("vIn", c->vIn);
                v->write("vOut", c->vOut);
                v->write("vAnalyzer", c->vAnalyzer);
                v->write("nSync", c->nSync);
                v->write("bHasSolo", c->bHasSolo);

                v->write("vTrRe", c->vTrRe);
                v->write("vTrIm", c->vTrIm);

                v->write("pIn", c->pIn);
                v->write("pOut", c->pOut);
                v->write("pInGain", c->pInGain);
                v->write("pTrAmp", c->pTrAmp);
                v->write("pPitch", c->pPitch);
                v->write("pFftInSwitch", c->pFftInSwitch);
                v->write("pFftOutSwitch", c->pFftOutSwitch);
                v->write("pFftInMesh", c->pFftInMesh);
                v->write("pFftOutMesh", c->pFftOutMesh);
                v->write("pVisible", c->pVisible);
                v->write("pInMeter", c->pInMeter);
                v->write("pOutMeter", c->pOutMeter);
            }
            v->end_object();
        }

        void para_equalizer::dump(dspu::IStateDumper *v) const
        {
            plug::Module::dump(v);

            size_t channels     = (nMode == EQ_MONO) ? 1 : 2;

            v->write_object("sAnalyzer", &sAnalyzer);
            v->write("nFilters", nFilters);
            v->write("nMode", nMode);
            v->begin_array("vChannels", vChannels, channels);
            {
                for (size_t i=0; i<channels; ++i)
                    dump_channel(v, &vChannels[i]);
            }
            v->end_array();
            v->write("vFreqs", vFreqs);
            v->write("vIndexes", vIndexes);
            v->write("fGainIn", fGainIn);
            v->write("fZoom", fZoom);
            v->write("bListen", bListen);
            v->write("bSmoothMode", bSmoothMode);
            v->write_object("pIDisplay", pIDisplay);

            v->write("pBypass", pBypass);
            v->write("pGainIn", pGainIn);
            v->write("pGainOut", pGainOut);
            v->write("pReactivity", pReactivity);
            v->write("pListen", pListen);
            v->write("pShiftGain", pShiftGain);
            v->write("pZoom", pZoom);
            v->write("pEqMode", pEqMode);
            v->write("pBalance", pBalance);
        }

    } /* namespace plugins */
} /* namespace lsp */


