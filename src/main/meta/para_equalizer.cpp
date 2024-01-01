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

#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/shared/meta/developers.h>
#include <private/meta/para_equalizer.h>

#define LSP_PLUGINS_PARA_EQUALIZER_VERSION_MAJOR         1
#define LSP_PLUGINS_PARA_EQUALIZER_VERSION_MINOR         0
#define LSP_PLUGINS_PARA_EQUALIZER_VERSION_MICRO         22

#define LSP_PLUGINS_PARA_EQUALIZER_VERSION  \
    LSP_MODULE_VERSION( \
        LSP_PLUGINS_PARA_EQUALIZER_VERSION_MAJOR, \
        LSP_PLUGINS_PARA_EQUALIZER_VERSION_MINOR, \
        LSP_PLUGINS_PARA_EQUALIZER_VERSION_MICRO  \
    )

namespace lsp
{
    namespace meta
    {
        //-------------------------------------------------------------------------
        // Parametric Equalizer
        static const int plugin_classes[]           = { C_PARA_EQ, -1 };
        static const int clap_features_mono[]       = { CF_AUDIO_EFFECT, CF_EQUALIZER, CF_MONO, -1 };
        static const int clap_features_stereo[]     = { CF_AUDIO_EFFECT, CF_EQUALIZER, CF_STEREO, -1 };

        static const port_item_t filter_slopes[] =
        {
            { "x1",             "eq.slope.x1" },
            { "x2",             "eq.slope.x2" },
            { "x3",             "eq.slope.x3" },
            { "x4",             "eq.slope.x4" },
            { NULL, NULL }
        };

        static const port_item_t equalizer_eq_modes[] =
        {
            { "IIR",            "eq.type.iir" },
            { "FIR",            "eq.type.fir" },
            { "FFT",            "eq.type.fft" },
            { "SPM",            "eq.type.spm" },
            { NULL, NULL }
        };

        static const port_item_t filter_types[] =
        {
            { "Off",            "eq.flt.off" },
            { "Bell",           "eq.flt.bell" },
            { "Hi-pass",        "eq.flt.hipass" },
            { "Hi-shelf",       "eq.flt.hishelf" },
            { "Lo-pass",        "eq.flt.lopass" },
            { "Lo-shelf",       "eq.flt.loshelf" },
            { "Notch",          "eq.flt.notch" },
            { "Resonance",      "eq.flt.resonance" },
            { "Allpass",        "eq.flt.allpass" },
            { "Bandpass",       "eq.flt.bandpass" },
            { "Ladder-pass",    "eq.flt.ladpass" },
            { "Ladder-rej",     "eq.flt.ladrej" },

            // Additional stuff
        #ifdef LSP_USE_EXPERIMENTAL
            { "Allpass2",       "eq.flt.allpass2" },
            { "Envelope",       "eq.flt.envelope" },
            { "LUFS",           "eq.flt.lufs" },
        #endif /* LSP_USE_EXPERIMENTAL */
            { NULL, NULL }
        };

        static const port_item_t filter_modes[] =
        {
            { "RLC (BT)",       "eq.mode.rlc_bt" },
            { "RLC (MT)",       "eq.mode.rlc_mt" },
            { "BWC (BT)",       "eq.mode.bwc_bt" },
            { "BWC (MT)",       "eq.mode.bwc_mt" },
            { "LRX (BT)",       "eq.mode.lrx_bt" },
            { "LRX (MT)",       "eq.mode.lrx_mt" },
            { "APO (DR)",       "eq.mode.apo_dr" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_skip[] =
        {
            { NULL, NULL }
        };

        static const port_item_t equalizer_fft_mode[] =
        {
            { "Off",            "metering.fft.off" },
            { "Post-eq",        "metering.fft.post_eq" },
            { "Pre-eq",         "metering.fft.pre_eq" },
            { "Both",           "metering.fft.both" },
            { NULL, NULL }
        };

        static const port_item_t equalizer_fft_speed[] =
        {
            { "Slowest",      "Slowest" },
            { "Slower",       "Slower" },
            { "Slow",         "Slow" },
            { "Normal",       "Normal" },
            { "Fast",         "Fast" },
            { "Faster",       "Faster" },
            { "Fastest",      "Fastest" },
            { NULL, NULL }
        };


        #define EQ_FILTER(id, label, x, total, f) \
                COMBO("ft" id "_" #x, "Filter type " label #x, 0, filter_types), \
                COMBO("fm" id "_" #x, "Filter mode " label #x, 0, filter_modes), \
                COMBO("s" id "_" #x, "Filter slope " label #x, 0, filter_slopes), \
                SWITCH("xs" id "_" #x, "Filter solo " label #x, 0.0f), \
                SWITCH("xm" id "_" #x, "Filter mute " label #x, 0.0f), \
                LOG_CONTROL_DFL("f" id "_" #x, "Frequency " label #x, U_HZ, para_equalizer_metadata::FREQ, f), \
                CONTROL("w" id "_" #x, "Filter Width " label #x, U_OCTAVES, para_equalizer_metadata::WIDTH), \
                { "g" id "_" #x, "Gain " label # x, U_GAIN_AMP, R_CONTROL, F_IN | F_LOG | F_UPPER | F_LOWER | F_STEP, GAIN_AMP_M_36_DB, GAIN_AMP_P_36_DB, GAIN_AMP_0_DB, 0.01, NULL, NULL }, \
                { "q" id "_" #x, "Quality factor " label #x, U_NONE, R_CONTROL, F_IN | F_UPPER | F_LOWER | F_STEP, 0.0f, 100.0f, 0.0f, 0.025f, NULL        }, \
                { "hue" id "_" #x, "Hue " label #x, U_NONE, R_CONTROL, F_IN | F_UPPER | F_LOWER | F_STEP | F_CYCLIC, 0.0f, 1.0f, (float(x) / float(total)), 0.25f/360.0f, NULL     }, \
                BLINK("fv" id "_" #x, "Filter visibility " label #x), \
                MESH("agf" id "_" #x, "Amplitude graph " label #x, 2, para_equalizer_metadata::FILTER_MESH_POINTS)

        #define EQ_FILTER_MONO(x, total, f)     EQ_FILTER("", "", x, total, f)
        #define EQ_FILTER_STEREO(x, total, f)   EQ_FILTER("", "", x, total, f)

        #define EQ_COMMON(filters) \
                BYPASS, \
                AMP_GAIN("g_in", "Input gain", para_equalizer_metadata::IN_GAIN_DFL, 10.0f), \
                AMP_GAIN("g_out", "Output gain", para_equalizer_metadata::OUT_GAIN_DFL, 10.0f), \
                COMBO("mode", "Equalizer mode", 0, equalizer_eq_modes), \
                COMBO("fft", "FFT analysis", 3, equalizer_fft_mode), \
                COMBO("fftsp", "FFT speed", 3, equalizer_fft_speed), \
                LOG_CONTROL("react", "FFT reactivity", U_MSEC, para_equalizer_metadata::REACT_TIME), \
                AMP_GAIN("shift", "Shift gain", 1.0f, 100.0f), \
                LOG_CONTROL("zoom", "Graph zoom", U_GAIN_AMP, para_equalizer_metadata::ZOOM), \
                INT_CONTROL_RANGE("insp_id", "Inspected filter identifier", U_NONE, -1, (filters-1), -1, 1), \
                CONTROL("insp_r", "Inspect frequency range", U_OCTAVES, para_equalizer_metadata::INSPECT), \
                SWITCH("insp_on", "Automatically inspect filter when editing", 0)

        #define EQ_MONO_PORTS \
                MESH("ag", "Amplitude graph", 2, para_equalizer_metadata::MESH_POINTS), \
                CONTROL("frqs", "Frequency shift", U_SEMITONES, para_equalizer_metadata::PITCH), \
                METER_GAIN("im", "Input signal meter", GAIN_AMP_P_12_DB), \
                METER_GAIN("sm", "Output signal meter", GAIN_AMP_P_12_DB)

        #define EQ_STEREO_PORTS \
                PAN_CTL("bal", "Output balance", 0.0f), \
                MESH("ag", "Amplitude graph", 2, para_equalizer_metadata::MESH_POINTS), \
                CONTROL("frqs", "Frequency shift", U_SEMITONES, para_equalizer_metadata::PITCH), \
                METER_GAIN("iml", "Input signal meter Left", GAIN_AMP_P_12_DB), \
                METER_GAIN("sml", "Output signal meter Left", GAIN_AMP_P_12_DB), \
                METER_GAIN("imr", "Input signal meter Right", GAIN_AMP_P_12_DB), \
                METER_GAIN("smr", "Output signal meter Right", GAIN_AMP_P_12_DB)

        #define CHANNEL_ANALYSIS(id, label) \
            SWITCH("ife" id, "Input FFT graph enable" label, 1.0f), \
            SWITCH("ofe" id, "Output FFT graph enable" label, 1.0f), \
            MESH("ifg" id, "Input FFT graph" label, 2, para_equalizer_metadata::MESH_POINTS + 2), \
            MESH("ofg" id, "Output FFT graph" label, 2, para_equalizer_metadata::MESH_POINTS)


        static const port_t para_equalizer_x32_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            EQ_COMMON(32),
            CHANNEL_ANALYSIS("", ""),
            EQ_MONO_PORTS,
            EQ_FILTER_MONO(0, 32, 16.0f),
            EQ_FILTER_MONO(1, 32, 20.0f),
            EQ_FILTER_MONO(2, 32, 25.0f),
            EQ_FILTER_MONO(3, 32, 31.5f),
            EQ_FILTER_MONO(4, 32, 40.0f),
            EQ_FILTER_MONO(5, 32, 50.0f),
            EQ_FILTER_MONO(6, 32, 63.0f),
            EQ_FILTER_MONO(7, 32, 80.0f),
            EQ_FILTER_MONO(8, 32, 100.0f),
            EQ_FILTER_MONO(9, 32, 125.0f),
            EQ_FILTER_MONO(10, 32, 160.0f),
            EQ_FILTER_MONO(11, 32, 200.0f),
            EQ_FILTER_MONO(12, 32, 250.0f),
            EQ_FILTER_MONO(13, 32, 315.0f),
            EQ_FILTER_MONO(14, 32, 400.0f),
            EQ_FILTER_MONO(15, 32, 500.0f),
            EQ_FILTER_MONO(16, 32, 630.0f),
            EQ_FILTER_MONO(17, 32, 800.0f),
            EQ_FILTER_MONO(18, 32, 1000.0f),
            EQ_FILTER_MONO(19, 32, 1250.0f),
            EQ_FILTER_MONO(20, 32, 1600.0f),
            EQ_FILTER_MONO(21, 32, 2000.0f),
            EQ_FILTER_MONO(22, 32, 2500.0f),
            EQ_FILTER_MONO(23, 32, 3150.0f),
            EQ_FILTER_MONO(24, 32, 4000.0f),
            EQ_FILTER_MONO(25, 32, 5000.0f),
            EQ_FILTER_MONO(26, 32, 6300.0f),
            EQ_FILTER_MONO(27, 32, 8000.0f),
            EQ_FILTER_MONO(28, 32, 10000.0f),
            EQ_FILTER_MONO(29, 32, 12500.0f),
            EQ_FILTER_MONO(30, 32, 16000.0f),
            EQ_FILTER_MONO(31, 32, 20000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x32_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(32),
            CHANNEL_ANALYSIS("_l", " Left"),
            CHANNEL_ANALYSIS("_r", " Right"),
            EQ_STEREO_PORTS,
            EQ_FILTER_STEREO(0, 32, 16.0f),
            EQ_FILTER_STEREO(1, 32, 20.0f),
            EQ_FILTER_STEREO(2, 32, 25.0f),
            EQ_FILTER_STEREO(3, 32, 31.5f),
            EQ_FILTER_STEREO(4, 32, 40.0f),
            EQ_FILTER_STEREO(5, 32, 50.0f),
            EQ_FILTER_STEREO(6, 32, 63.0f),
            EQ_FILTER_STEREO(7, 32, 80.0f),
            EQ_FILTER_STEREO(8, 32, 100.0f),
            EQ_FILTER_STEREO(9, 32, 125.0f),
            EQ_FILTER_STEREO(10, 32, 160.0f),
            EQ_FILTER_STEREO(11, 32, 200.0f),
            EQ_FILTER_STEREO(12, 32, 250.0f),
            EQ_FILTER_STEREO(13, 32, 315.0f),
            EQ_FILTER_STEREO(14, 32, 400.0f),
            EQ_FILTER_STEREO(15, 32, 500.0f),
            EQ_FILTER_STEREO(16, 32, 630.0f),
            EQ_FILTER_STEREO(17, 32, 800.0f),
            EQ_FILTER_STEREO(18, 32, 1000.0f),
            EQ_FILTER_STEREO(19, 32, 1250.0f),
            EQ_FILTER_STEREO(20, 32, 1600.0f),
            EQ_FILTER_STEREO(21, 32, 2000.0f),
            EQ_FILTER_STEREO(22, 32, 2500.0f),
            EQ_FILTER_STEREO(23, 32, 3150.0f),
            EQ_FILTER_STEREO(24, 32, 4000.0f),
            EQ_FILTER_STEREO(25, 32, 5000.0f),
            EQ_FILTER_STEREO(26, 32, 6300.0f),
            EQ_FILTER_STEREO(27, 32, 8000.0f),
            EQ_FILTER_STEREO(28, 32, 10000.0f),
            EQ_FILTER_STEREO(29, 32, 12500.0f),
            EQ_FILTER_STEREO(30, 32, 16000.0f),
            EQ_FILTER_STEREO(31, 32, 20000.0f),

            PORTS_END
        };

        const meta::bundle_t para_equalizer_bundle =
        {
            "para_equalizer",
            "Parametric Equalizer",
            B_EQUALIZERS,
            "TfpJPsiouuU",
            "This plugin allows one to perform parametric equalization of input signal.\nUp to 32 different filters are simultaneously available for processing."
        };

        const meta::plugin_t para_equalizer_x32_mono =
        {
            "LSP EQ (Mono)",
            "EQ (Mono)",
            "LSP EQ (Mono)",
            "PE32M",
            &developers::v_sadovnikov,
            "para_equalizer_x32_mono",
            LSP_LV2_URI("para_equalizer_x32_mono"),
            LSP_LV2UI_URI("para_equalizer_x32_mono"),
            "i0px",
            LSP_LADSPA_PARA_EQUALIZER_BASE + 1,
            LSP_LADSPA_URI("para_equalizer_x32_mono"),
            LSP_CLAP_URI("para_equalizer_x32_mono"),
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_mono,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x32_mono_ports,
            "equalizer/parametric/stereo.xml",
            "equalizer/parametric/stereo",
            mono_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x32_stereo =
        {
            "LSP EQ",
            "EQ",
            "LSP EQ",
            "PE32S",
            &developers::v_sadovnikov,
            "para_equalizer_x32_stereo",
            LSP_LV2_URI("para_equalizer_x32_stereo"),
            LSP_LV2UI_URI("para_equalizer_x32_stereo"),
            "s2nz",
            LSP_LADSPA_PARA_EQUALIZER_BASE + 3,
            LSP_LADSPA_URI("para_equalizer_x32_stereo"),
            LSP_CLAP_URI("para_equalizer_x32_stereo"),
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x32_stereo_ports,
            "equalizer/parametric/stereo.xml",
            "equalizer/parametric/stereo",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };
    } /* namespace meta */
} /* namespace lsp */


