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

#ifndef PRIVATE_META_PARA_EQUALIZER_H_
#define PRIVATE_META_PARA_EQUALIZER_H_

#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/dsp-units/misc/windows.h>
#include <lsp-plug.in/dsp-units/misc/envelope.h>

namespace lsp
{
    //-------------------------------------------------------------------------
    // Parametric equalizer metadata
    namespace meta
    {
        struct para_equalizer_metadata
        {
            static constexpr float          FREQ_MIN            = SPEC_FREQ_MIN;
            static constexpr float          FREQ_MAX            = SPEC_FREQ_MAX;
            static constexpr float          FREQ_DFL            = 1000;
            static constexpr float          FREQ_STEP           = 0.002;

            static constexpr size_t         FFT_RANK            = 13;
            static constexpr size_t         FFT_ITEMS           = 1 << FFT_RANK;
            static constexpr size_t         MESH_POINTS         = 640;
            static constexpr size_t         FILTER_MESH_POINTS  = MESH_POINTS + 2;
            static constexpr size_t         FFT_WINDOW          = lsp::dspu::windows::HANN;
            static constexpr size_t         FFT_ENVELOPE        = lsp::dspu::envelope::PINK_NOISE;

            static constexpr float          REACT_TIME_MIN      = 0.000;
            static constexpr float          REACT_TIME_MAX      = 1.000;
            static constexpr float          REACT_TIME_DFL      = 0.200;
            static constexpr float          REACT_TIME_STEP     = 0.001;

            static constexpr float          ZOOM_MIN            = GAIN_AMP_M_36_DB;
            static constexpr float          ZOOM_MAX            = GAIN_AMP_0_DB;
            static constexpr float          ZOOM_DFL            = GAIN_AMP_0_DB;
            static constexpr float          ZOOM_STEP           = 0.025f;

            static constexpr float          IN_GAIN_DFL         = 1.0f;
            static constexpr float          OUT_GAIN_DFL        = 1.0f;
            static constexpr size_t         MODE_DFL            = 0;

            static constexpr float          PITCH_MIN           = -120.0f;
            static constexpr float          PITCH_MAX           = 120.0f;
            static constexpr float          PITCH_DFL           = 0.0f;
            static constexpr float          PITCH_STEP          = 0.01f;

            static constexpr float          INSPECT_MIN         = 0.0f;
            static constexpr float          INSPECT_MAX         = 5.0f;
            static constexpr float          INSPECT_DFL         = 1.0f;
            static constexpr float          INSPECT_STEP        = 0.005f;

            static constexpr size_t         REFRESH_RATE        = 20;

            enum eq_filter_t
            {
                EQF_OFF,
                EQF_BELL,
                EQF_HIPASS,
                EQF_HISHELF,
                EQF_LOPASS,
                EQF_LOSHELF,
                EQF_NOTCH,
                EQF_RESONANCE,
                EQF_ALLPASS,

            #ifdef LSP_USE_EXPERIMENTAL
                EQF_ALLPASS2,
                EQF_LADDERPASS,
                EQF_LADDERREJ,
                EQF_ENVELOPE,
                EQF_BANDPASS,
                EQF_LUFS,
            #endif /* LSP_USE_EXPERIMENTAL */
            };

            enum eq_filter_mode_t
            {
                EFM_RLC_BT,
                EFM_RLC_MT,
                EFM_BWC_BT,
                EFM_BWC_MT,
                EFM_LRX_BT,
                EFM_LRX_MT,
                EFM_APO_DR
            };

            enum para_eq_mode_t
            {
                PEM_IIR,
                PEM_FIR,
                PEM_FFT,
                PEM_SPM
            };
        };

        extern const meta::plugin_t para_equalizer_x16_mono;
        extern const meta::plugin_t para_equalizer_x16_stereo;
        extern const meta::plugin_t para_equalizer_x16_lr;
        extern const meta::plugin_t para_equalizer_x16_ms;
        extern const meta::plugin_t para_equalizer_x32_mono;
        extern const meta::plugin_t para_equalizer_x32_stereo;
        extern const meta::plugin_t para_equalizer_x32_lr;
        extern const meta::plugin_t para_equalizer_x32_ms;

    } // namespace meta
} // namespace lsp

#endif /* PRIVATE_META_PARA_EQUALIZER_H_ */
