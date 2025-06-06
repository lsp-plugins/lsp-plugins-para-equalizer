/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#define LSP_PLUGINS_PARA_EQUALIZER_VERSION_MICRO         30

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

        static const port_item_t filter_select_8[] =
        {
            { "Filters 0-7",            "para_eq.flt_0:7" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_8lr[] =
        {
            { "Filters Left 0-7",       "para_eq.flt_l_0:7" },
            { "Filters Right 0-7",      "para_eq.flt_r_0:7" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_8ms[] =
        {
            { "Filters Middle 0-7",     "para_eq.flt_m_0:7" },
            { "Filters Side 0-7",       "para_eq.flt_s_0:7" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_16[] =
        {
            { "Filters 0-7",            "para_eq.flt_0:7" },
            { "Filters 8-15",           "para_eq.flt_8:15" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_16lr[] =
        {
            { "Filters Left 0-7",       "para_eq.flt_l_0:7" },
            { "Filters Right 0-7",      "para_eq.flt_r_0:7" },
            { "Filters Left 8-15",      "para_eq.flt_l_8:15" },
            { "Filters Right 8-15",     "para_eq.flt_r_8:15" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_16ms[] =
        {
            { "Filters Middle 0-7",     "para_eq.flt_m_0:7" },
            { "Filters Side 0-7",       "para_eq.flt_s_0:7" },
            { "Filters Middle 8-15",    "para_eq.flt_m_8:15" },
            { "Filters Side 8-15",      "para_eq.flt_s_8:15" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_32[] =
        {
            { "Filters 0-7",            "para_eq.flt_0:7" },
            { "Filters 8-15",           "para_eq.flt_8:15" },
            { "Filters 16-23",          "para_eq.flt_16:23" },
            { "Filters 24-31",          "para_eq.flt_24:31" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_32lr[] =
        {
            { "Filters Left 0-7",       "para_eq.flt_l_0:7" },
            { "Filters Right 0-7",      "para_eq.flt_r_0:7" },
            { "Filters Left 8-15",      "para_eq.flt_l_8:15" },
            { "Filters Right 8-15",     "para_eq.flt_r_8:15" },
            { "Filters Left 16-23",     "para_eq.flt_l_16:23" },
            { "Filters Right 16-23",    "para_eq.flt_r_16:23" },
            { "Filters Left 24-31",     "para_eq.flt_l_24:31" },
            { "Filters Right 24-31",    "para_eq.flt_r_24:31" },
            { NULL, NULL }
        };

        static const port_item_t filter_select_32ms[] =
        {
            { "Filters Middle 0-7",     "para_eq.flt_m_0:7" },
            { "Filters Side 0-7",       "para_eq.flt_s_0:7" },
            { "Filters Middle 8-15",    "para_eq.flt_m_8:15" },
            { "Filters Side 8-15",      "para_eq.flt_s_8:15" },
            { "Filters Mid 16-23",      "para_eq.flt_m_16:23" },
            { "Filters Side 16-23",     "para_eq.flt_s_16:23" },
            { "Filters Mid 24-31",      "para_eq.flt_m_24:31" },
            { "Filters Side 24-31",     "para_eq.flt_s_24:31" },
            { NULL, NULL }
        };

        #define EQ_FILTER(id, label, alias, x, total, f) \
            COMBO("ft" id "_" #x, "Filter type " label #x, "Type " #x alias, 0, filter_types), \
            COMBO("fm" id "_" #x, "Filter mode " label #x, "Mode " #x alias, 0, filter_modes), \
            COMBO("s" id "_" #x, "Filter slope " label #x, "Slope " #x alias, 0, filter_slopes), \
            SWITCH("xs" id "_" #x, "Filter solo " label #x, "Solo " #x alias, 0.0f), \
            SWITCH("xm" id "_" #x, "Filter mute " label #x, "Mute " #x alias, 0.0f), \
            LOG_CONTROL_DFL("f" id "_" #x, "Frequency " label #x, "Freq " #x alias, U_HZ, para_equalizer_metadata::FREQ, f), \
            CONTROL("w" id "_" #x, "Filter Width " label #x, "Width " #x alias, U_OCTAVES, para_equalizer_metadata::WIDTH), \
            LOG_CONTROL_ALL("g" id "_" #x, "Gain " label # x, "Gain " #x alias, U_GAIN_AMP, GAIN_AMP_M_36_DB, GAIN_AMP_P_36_DB, GAIN_AMP_0_DB, 0.01), \
            CONTROL_ALL("q" id "_" #x, "Quality factor " label #x, "Q " #x alias, U_NONE, 0.0f, 100.0f, 0.0f, 0.025f), \
            CYC_CONTROL_ALL("hue" id "_" #x, "Hue " label #x, "Hue " #x alias, U_NONE, 0.0f, 1.0f, (float(x) / float(total)), 0.25f/360.0f), \
            BLINK("fv" id "_" #x, "Filter visibility " label #x), \
            MESH("agf" id "_" #x, "Amplitude graph " label #x, 2, para_equalizer_metadata::FILTER_MESH_POINTS)

        #define EQ_FILTER_MONO(x, total, f)     EQ_FILTER("", "", "", x, total, f)
        #define EQ_FILTER_STEREO(x, total, f)   EQ_FILTER("", "", "", x, total, f)
        #define EQ_FILTER_LR(x, total, f)       EQ_FILTER("l", "Left ", " L", x, total, f), EQ_FILTER("r", "Right ", " R", x, total, f)
        #define EQ_FILTER_MS(x, total, f)       EQ_FILTER("m", "Mid ", " M", x, total, f), EQ_FILTER("s", "Side ", " S", x, total, f)

        #define EQ_COMMON(fselect, filters) \
            BYPASS, \
            AMP_GAIN("g_in", "Input gain", "Input gain", para_equalizer_metadata::IN_GAIN_DFL, 10.0f), \
            AMP_GAIN("g_out", "Output gain", "Output gain", para_equalizer_metadata::OUT_GAIN_DFL, 10.0f), \
            COMBO("mode", "Equalizer mode", "Mode", 0, equalizer_eq_modes), \
            LOG_CONTROL("react", "FFT reactivity", "Reactivity", U_MSEC, para_equalizer_metadata::REACT_TIME), \
            AMP_GAIN("shift", "Shift gain", "Shift gain", 1.0f, 100.0f), \
            LOG_CONTROL("zoom", "Graph zoom", "Zoom", U_GAIN_AMP, para_equalizer_metadata::ZOOM), \
            COMBO("fsel", "Filter select", "Filter", 0, fselect), \
            INT_CONTROL_ALL("insp_id", "Inspected filter identifier", "Inspect index", U_NONE, -1, (filters-1), -1, 1), \
            CONTROL("insp_r", "Inspect frequency range", "Inspect range", U_OCTAVES, para_equalizer_metadata::INSPECT), \
            SWITCH("insp_on", "Automatically inspect filter when editing", "Auto inspect", 0)

        #define EQ_MONO_PORTS \
            MESH("ag", "Amplitude graph", 2, para_equalizer_metadata::MESH_POINTS), \
            CONTROL("frqs", "Frequency shift", "Freq shift", U_SEMITONES, para_equalizer_metadata::PITCH), \
            METER_GAIN("im", "Input signal meter", GAIN_AMP_P_12_DB), \
            METER_GAIN("sm", "Output signal meter", GAIN_AMP_P_12_DB)

        #define EQ_STEREO_PORTS \
            PAN_CTL("bal", "Output balance", "Out balance", 0.0f), \
            MESH("ag", "Amplitude graph", 2, para_equalizer_metadata::MESH_POINTS), \
            CONTROL("frqs", "Frequency shift", "Freq shift", U_SEMITONES, para_equalizer_metadata::PITCH), \
            METER_GAIN("iml", "Input signal meter Left", GAIN_AMP_P_12_DB), \
            METER_GAIN("sml", "Output signal meter Left", GAIN_AMP_P_12_DB), \
            METER_GAIN("imr", "Input signal meter Right", GAIN_AMP_P_12_DB), \
            METER_GAIN("smr", "Output signal meter Right", GAIN_AMP_P_12_DB)

        #define EQ_LR_PORTS \
            PAN_CTL("bal", "Output balance", "Out balance", 0.0f), \
            MESH("ag_l", "Amplitude graph Left", 2, para_equalizer_metadata::MESH_POINTS), \
            CONTROL("frqs_l", "Frequency shift Left", "Freq shift L", U_SEMITONES, para_equalizer_metadata::PITCH), \
            METER_GAIN("iml", "Input signal meter Left", GAIN_AMP_P_12_DB), \
            METER_GAIN("sml", "Output signal meter Left", GAIN_AMP_P_12_DB), \
            SWITCH("fltv_l", "Filter visibility Left", "Show filter L", 1.0f), \
            MESH("ag_r", "Amplitude graph Right", 2, para_equalizer_metadata::MESH_POINTS), \
            CONTROL("frqs_r", "Frequency shift Right", "Freq shift R", U_SEMITONES, para_equalizer_metadata::PITCH), \
            METER_GAIN("imr", "Input signal meter Right", GAIN_AMP_P_12_DB), \
            METER_GAIN("smr", "Output signal meter Right", GAIN_AMP_P_12_DB), \
            SWITCH("fltv_r", "Filter visibility Right", "Show filter R", 1.0f)

        #define EQ_MS_PORTS \
            PAN_CTL("bal", "Output balance", "Out balance", 0.0f), \
            SWITCH("lstn", "Mid/Side listen", "M/S listen", 0.0f), \
            AMP_GAIN100("gain_m", "Mid gain", "Gain M", GAIN_AMP_0_DB), \
            AMP_GAIN100("gain_s", "Side gain", "Gain S", GAIN_AMP_0_DB), \
            MESH("ag_m", "Amplitude graph Mid", 2, para_equalizer_metadata::MESH_POINTS), \
            CONTROL("frqs_m", "Frequency shift Mid", "Freq shift M", U_SEMITONES, para_equalizer_metadata::PITCH), \
            METER_GAIN("iml", "Input signal meter Left", GAIN_AMP_P_12_DB), \
            METER_GAIN("sml", "Output signal meter Left", GAIN_AMP_P_12_DB), \
            SWITCH("fltv_m", "Filter visibility Mid", "Show filter M", 1.0f), \
            MESH("ag_s", "Amplitude graph Side", 2, para_equalizer_metadata::MESH_POINTS), \
            CONTROL("frqs_s", "Frequency shift Side", "Freq shift S", U_SEMITONES, para_equalizer_metadata::PITCH), \
            METER_GAIN("imr", "Input signal meter Right", GAIN_AMP_P_12_DB), \
            METER_GAIN("smr", "Output signal meter Right", GAIN_AMP_P_12_DB), \
            SWITCH("fltv_s", "Filter visibility Side", "Show filter S", 1.0f)

        #define EQ_COMMUNICATION_MONO \
            OPT_SEND_NAME("send", "Audio send"), \
            OPT_AUDIO_SEND("snd", "Audio send output", 0, "send"), \
            OPT_RETURN_NAME("return", "Audio return"), \
            OPT_AUDIO_RETURN("rtn", "Audio return input", 0, "return")

        #define EQ_COMMUNICATION_STEREO \
            OPT_SEND_NAME("send", "Audio send"), \
            OPT_AUDIO_SEND("snd_l", "Audio send output left", 0, "send"), \
            OPT_AUDIO_SEND("snd_r", "Audio send output right", 1, "send"), \
            OPT_RETURN_NAME("return", "Audio return"), \
            OPT_AUDIO_RETURN("rtn_l", "Audio return input left", 0, "return"), \
            OPT_AUDIO_RETURN("rtn_r", "Audio return input right", 1, "return")

        #define CHANNEL_ANALYSIS(id, label, alias) \
            SWITCH("ife" id, "Input FFT graph enable" label, "FFT In" alias, 1.0f), \
            SWITCH("ofe" id, "Output FFT graph enable" label, "FFT Out" alias, 1.0f), \
            SWITCH("rfe" id, "Return FFT graph enable" label, "FFT Ret" alias, 1.0f), \
            MESH("ifg" id, "Input FFT graph" label, 2, para_equalizer_metadata::MESH_POINTS + 2), \
            MESH("ofg" id, "Output FFT graph" label, 2, para_equalizer_metadata::MESH_POINTS), \
            MESH("rfg" id, "Return FFT graph" label, 2, para_equalizer_metadata::MESH_POINTS)

        static const port_t para_equalizer_x8_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            EQ_COMMON(filter_select_8, 8),
            EQ_COMMUNICATION_MONO,
            CHANNEL_ANALYSIS("", "", ""),
            EQ_MONO_PORTS,
            EQ_FILTER_MONO(0, 8, 16.0f),
            EQ_FILTER_MONO(1, 8, 40.0f),
            EQ_FILTER_MONO(2, 8, 100.0f),
            EQ_FILTER_MONO(3, 8, 250.0f),
            EQ_FILTER_MONO(4, 8, 630.0f),
            EQ_FILTER_MONO(5, 8, 1600.0f),
            EQ_FILTER_MONO(6, 8, 4000.0f),
            EQ_FILTER_MONO(7, 8, 10000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x16_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            EQ_COMMON(filter_select_16, 16),
            EQ_COMMUNICATION_MONO,
            CHANNEL_ANALYSIS("", "", ""),
            EQ_MONO_PORTS,
            EQ_FILTER_MONO(0, 16, 16.0f),
            EQ_FILTER_MONO(1, 16, 25.0f),
            EQ_FILTER_MONO(2, 16, 40.0f),
            EQ_FILTER_MONO(3, 16, 63.0f),
            EQ_FILTER_MONO(4, 16, 100.0f),
            EQ_FILTER_MONO(5, 16, 160.0f),
            EQ_FILTER_MONO(6, 16, 250.0f),
            EQ_FILTER_MONO(7, 16, 400.0f),
            EQ_FILTER_MONO(8, 16, 630.0f),
            EQ_FILTER_MONO(9, 16, 1000.0f),
            EQ_FILTER_MONO(10, 16, 1600.0f),
            EQ_FILTER_MONO(11, 16, 2500.0f),
            EQ_FILTER_MONO(12, 16, 4000.0f),
            EQ_FILTER_MONO(13, 16, 6300.0f),
            EQ_FILTER_MONO(14, 16, 10000.0f),
            EQ_FILTER_MONO(15, 16, 16000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x32_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            EQ_COMMON(filter_select_32, 32),
            EQ_COMMUNICATION_MONO,
            CHANNEL_ANALYSIS("", "", ""),
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

        static const port_t para_equalizer_x8_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_8, 8),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_l", " Left", " L"),
            CHANNEL_ANALYSIS("_r", " Right", " R"),
            EQ_STEREO_PORTS,
            EQ_FILTER_MONO(0, 8, 16.0f),
            EQ_FILTER_MONO(1, 8, 40.0f),
            EQ_FILTER_MONO(2, 8, 100.0f),
            EQ_FILTER_MONO(3, 8, 250.0f),
            EQ_FILTER_MONO(4, 8, 630.0f),
            EQ_FILTER_MONO(5, 8, 1600.0f),
            EQ_FILTER_MONO(6, 8, 4000.0f),
            EQ_FILTER_MONO(7, 8, 10000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x16_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_16, 16),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_l", " Left", " L"),
            CHANNEL_ANALYSIS("_r", " Right", " R"),
            EQ_STEREO_PORTS,
            EQ_FILTER_STEREO(0, 16, 16.0f),
            EQ_FILTER_STEREO(1, 16, 25.0f),
            EQ_FILTER_STEREO(2, 16, 40.0f),
            EQ_FILTER_STEREO(3, 16, 63.0f),
            EQ_FILTER_STEREO(4, 16, 100.0f),
            EQ_FILTER_STEREO(5, 16, 160.0f),
            EQ_FILTER_STEREO(6, 16, 250.0f),
            EQ_FILTER_STEREO(7, 16, 400.0f),
            EQ_FILTER_STEREO(8, 16, 630.0f),
            EQ_FILTER_STEREO(9, 16, 1000.0f),
            EQ_FILTER_STEREO(10, 16, 1600.0f),
            EQ_FILTER_STEREO(11, 16, 2500.0f),
            EQ_FILTER_STEREO(12, 16, 4000.0f),
            EQ_FILTER_STEREO(13, 16, 6300.0f),
            EQ_FILTER_STEREO(14, 16, 10000.0f),
            EQ_FILTER_STEREO(15, 16, 16000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x32_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_32, 32),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_l", " Left", " L"),
            CHANNEL_ANALYSIS("_r", " Right", " R"),
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

        static const port_t para_equalizer_x8_lr_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_8lr, 16),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_l", " Left", " L"),
            CHANNEL_ANALYSIS("_r", " Right", " R"),
            EQ_LR_PORTS,
            EQ_FILTER_LR(0, 8, 16.0f),
            EQ_FILTER_LR(1, 8, 40.0f),
            EQ_FILTER_LR(2, 8, 100.0f),
            EQ_FILTER_LR(3, 8, 250.0f),
            EQ_FILTER_LR(4, 8, 630.0f),
            EQ_FILTER_LR(5, 8, 1600.0f),
            EQ_FILTER_LR(6, 8, 4000.0f),
            EQ_FILTER_LR(7, 8, 10000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x16_lr_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_16lr, 32),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_l", " Left", " L"),
            CHANNEL_ANALYSIS("_r", " Right", " R"),
            EQ_LR_PORTS,
            EQ_FILTER_LR(0, 16, 16.0f),
            EQ_FILTER_LR(1, 16, 25.0f),
            EQ_FILTER_LR(2, 16, 40.0f),
            EQ_FILTER_LR(3, 16, 63.0f),
            EQ_FILTER_LR(4, 16, 100.0f),
            EQ_FILTER_LR(5, 16, 160.0f),
            EQ_FILTER_LR(6, 16, 250.0f),
            EQ_FILTER_LR(7, 16, 400.0f),
            EQ_FILTER_LR(8, 16, 630.0f),
            EQ_FILTER_LR(9, 16, 1000.0f),
            EQ_FILTER_LR(10, 16, 1600.0f),
            EQ_FILTER_LR(11, 16, 2500.0f),
            EQ_FILTER_LR(12, 16, 4000.0f),
            EQ_FILTER_LR(13, 16, 6300.0f),
            EQ_FILTER_LR(14, 16, 10000.0f),
            EQ_FILTER_LR(15, 16, 16000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x32_lr_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_32lr, 64),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_l", " Left", " L"),
            CHANNEL_ANALYSIS("_r", " Right", " R"),
            EQ_LR_PORTS,
            EQ_FILTER_LR(0, 32, 16.0f),
            EQ_FILTER_LR(1, 32, 20.0f),
            EQ_FILTER_LR(2, 32, 25.0f),
            EQ_FILTER_LR(3, 32, 31.5f),
            EQ_FILTER_LR(4, 32, 40.0f),
            EQ_FILTER_LR(5, 32, 50.0f),
            EQ_FILTER_LR(6, 32, 63.0f),
            EQ_FILTER_LR(7, 32, 80.0f),
            EQ_FILTER_LR(8, 32, 100.0f),
            EQ_FILTER_LR(9, 32, 125.0f),
            EQ_FILTER_LR(10, 32, 160.0f),
            EQ_FILTER_LR(11, 32, 200.0f),
            EQ_FILTER_LR(12, 32, 250.0f),
            EQ_FILTER_LR(13, 32, 315.0f),
            EQ_FILTER_LR(14, 32, 400.0f),
            EQ_FILTER_LR(15, 32, 500.0f),
            EQ_FILTER_LR(16, 32, 630.0f),
            EQ_FILTER_LR(17, 32, 800.0f),
            EQ_FILTER_LR(18, 32, 1000.0f),
            EQ_FILTER_LR(19, 32, 1250.0f),
            EQ_FILTER_LR(20, 32, 1600.0f),
            EQ_FILTER_LR(21, 32, 2000.0f),
            EQ_FILTER_LR(22, 32, 2500.0f),
            EQ_FILTER_LR(23, 32, 3150.0f),
            EQ_FILTER_LR(24, 32, 4000.0f),
            EQ_FILTER_LR(25, 32, 5000.0f),
            EQ_FILTER_LR(26, 32, 6300.0f),
            EQ_FILTER_LR(27, 32, 8000.0f),
            EQ_FILTER_LR(28, 32, 10000.0f),
            EQ_FILTER_LR(29, 32, 12500.0f),
            EQ_FILTER_LR(30, 32, 16000.0f),
            EQ_FILTER_LR(31, 32, 20000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x8_ms_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_8ms, 16),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_m", " Mid", " M"),
            CHANNEL_ANALYSIS("_s", " Side", " S"),
            EQ_MS_PORTS,
            EQ_FILTER_MS(0, 8, 16.0f),
            EQ_FILTER_MS(1, 8, 40.0f),
            EQ_FILTER_MS(2, 8, 100.0f),
            EQ_FILTER_MS(3, 8, 250.0f),
            EQ_FILTER_MS(4, 8, 630.0f),
            EQ_FILTER_MS(5, 8, 1600.0f),
            EQ_FILTER_MS(6, 8, 4000.0f),
            EQ_FILTER_MS(7, 8, 10000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x16_ms_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_16ms, 32),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_m", " Mid", " M"),
            CHANNEL_ANALYSIS("_s", " Side", " S"),
            EQ_MS_PORTS,
            EQ_FILTER_MS(0, 16, 16.0f),
            EQ_FILTER_MS(1, 16, 25.0f),
            EQ_FILTER_MS(2, 16, 40.0f),
            EQ_FILTER_MS(3, 16, 63.0f),
            EQ_FILTER_MS(4, 16, 100.0f),
            EQ_FILTER_MS(5, 16, 160.0f),
            EQ_FILTER_MS(6, 16, 250.0f),
            EQ_FILTER_MS(7, 16, 400.0f),
            EQ_FILTER_MS(8, 16, 630.0f),
            EQ_FILTER_MS(9, 16, 1000.0f),
            EQ_FILTER_MS(10, 16, 1600.0f),
            EQ_FILTER_MS(11, 16, 2500.0f),
            EQ_FILTER_MS(12, 16, 4000.0f),
            EQ_FILTER_MS(13, 16, 6300.0f),
            EQ_FILTER_MS(14, 16, 10000.0f),
            EQ_FILTER_MS(15, 16, 16000.0f),

            PORTS_END
        };

        static const port_t para_equalizer_x32_ms_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            EQ_COMMON(filter_select_32ms, 64),
            EQ_COMMUNICATION_STEREO,
            CHANNEL_ANALYSIS("_m", " Mid", " M"),
            CHANNEL_ANALYSIS("_s", " Side", " S"),
            EQ_MS_PORTS,
            EQ_FILTER_MS(0, 32, 16.0f),
            EQ_FILTER_MS(1, 32, 20.0f),
            EQ_FILTER_MS(2, 32, 25.0f),
            EQ_FILTER_MS(3, 32, 31.5f),
            EQ_FILTER_MS(4, 32, 40.0f),
            EQ_FILTER_MS(5, 32, 50.0f),
            EQ_FILTER_MS(6, 32, 63.0f),
            EQ_FILTER_MS(7, 32, 80.0f),
            EQ_FILTER_MS(8, 32, 100.0f),
            EQ_FILTER_MS(9, 32, 125.0f),
            EQ_FILTER_MS(10, 32, 160.0f),
            EQ_FILTER_MS(11, 32, 200.0f),
            EQ_FILTER_MS(12, 32, 250.0f),
            EQ_FILTER_MS(13, 32, 315.0f),
            EQ_FILTER_MS(14, 32, 400.0f),
            EQ_FILTER_MS(15, 32, 500.0f),
            EQ_FILTER_MS(16, 32, 630.0f),
            EQ_FILTER_MS(17, 32, 800.0f),
            EQ_FILTER_MS(18, 32, 1000.0f),
            EQ_FILTER_MS(19, 32, 1250.0f),
            EQ_FILTER_MS(20, 32, 1600.0f),
            EQ_FILTER_MS(21, 32, 2000.0f),
            EQ_FILTER_MS(22, 32, 2500.0f),
            EQ_FILTER_MS(23, 32, 3150.0f),
            EQ_FILTER_MS(24, 32, 4000.0f),
            EQ_FILTER_MS(25, 32, 5000.0f),
            EQ_FILTER_MS(26, 32, 6300.0f),
            EQ_FILTER_MS(27, 32, 8000.0f),
            EQ_FILTER_MS(28, 32, 10000.0f),
            EQ_FILTER_MS(29, 32, 12500.0f),
            EQ_FILTER_MS(30, 32, 16000.0f),
            EQ_FILTER_MS(31, 32, 20000.0f),

            PORTS_END
        };

        const meta::bundle_t para_equalizer_bundle =
        {
            "para_equalizer",
            "Parametric Equalizer",
            B_EQUALIZERS,
            "TfpJPsiouuU",
            "This plugin allows one to perform parametric equalization of input signal.\nUp to 16 or 32 different filters are simultaneously available for processing."
        };

        const meta::plugin_t para_equalizer_x8_mono =
        {
            "Parametrischer Entzerrer x8 Mono",
            "Parametric Equalizer x8 Mono",
            "Parametric Equalizer x8 Mono",
            "PE8M",
            &developers::v_sadovnikov,
            "para_equalizer_x8_mono",
            {
                LSP_LV2_URI("para_equalizer_x8_mono"),
                LSP_LV2UI_URI("para_equalizer_x8_mono"),
                "dh38",
                LSP_VST3_UID("pe8m    dh38"),
                LSP_VST3UI_UID("pe8m    dh38"),
                LSP_LADSPA_PARA_EQUALIZER_X8 + 0,
                LSP_LADSPA_URI("para_equalizer_x8_mono"),
                LSP_CLAP_URI("para_equalizer_x8_mono"),
                LSP_GST_UID("para_equalizer_x8_mono"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_mono,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x8_mono_ports,
            "equalizer/parametric/mono.xml",
            "equalizer/parametric/mono",
            mono_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x16_mono =
        {
            "Parametrischer Entzerrer x16 Mono",
            "Parametric Equalizer x16 Mono",
            "Parametric Equalizer x16 Mono",
            "PE16M",
            &developers::v_sadovnikov,
            "para_equalizer_x16_mono",
            {
                LSP_LV2_URI("para_equalizer_x16_mono"),
                LSP_LV2UI_URI("para_equalizer_x16_mono"),
                "dh3y",
                LSP_VST3_UID("pe16m   dh3y"),
                LSP_VST3UI_UID("pe16m   dh3y"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 0,
                LSP_LADSPA_URI("para_equalizer_x16_mono"),
                LSP_CLAP_URI("para_equalizer_x16_mono"),
                LSP_GST_UID("para_equalizer_x16_mono"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_mono,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x16_mono_ports,
            "equalizer/parametric/mono.xml",
            "equalizer/parametric/mono",
            mono_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x32_mono =
        {
            "Parametrischer Entzerrer x32 Mono",
            "Parametric Equalizer x32 Mono",
            "Parametric Equalizer x32 Mono",
            "PE32M",
            &developers::v_sadovnikov,
            "para_equalizer_x32_mono",
            {
                LSP_LV2_URI("para_equalizer_x32_mono"),
                LSP_LV2UI_URI("para_equalizer_x32_mono"),
                "i0px",
                LSP_VST3_UID("pe32m   i0px"),
                LSP_VST3UI_UID("pe32m   i0px"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 1,
                LSP_LADSPA_URI("para_equalizer_x32_mono"),
                LSP_CLAP_URI("para_equalizer_x32_mono"),
                LSP_GST_UID("para_equalizer_x32_mono"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_mono,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x32_mono_ports,
            "equalizer/parametric/mono.xml",
            "equalizer/parametric/mono",
            mono_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x8_stereo =
        {
            "Parametrischer Entzerrer x8 Stereo",
            "Parametric Equalizer x8 Stereo",
            "Parametric Equalizer x8 Stereo",
            "PE8S",
            &developers::v_sadovnikov,
            "para_equalizer_x8_stereo",
            {
                LSP_LV2_URI("para_equalizer_x8_stereo"),
                LSP_LV2UI_URI("para_equalizer_x8_stereo"),
                "a5e8",
                LSP_VST3_UID("pe8s    a5e8"),
                LSP_VST3UI_UID("pe8s    a5e8"),
                LSP_LADSPA_PARA_EQUALIZER_X8 + 1,
                LSP_LADSPA_URI("para_equalizer_x8_stereo"),
                LSP_CLAP_URI("para_equalizer_x8_stereo"),
                LSP_GST_UID("para_equalizer_x8_stereo"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x8_stereo_ports,
            "equalizer/parametric/stereo.xml",
            "equalizer/parametric/stereo",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x16_stereo =
        {
            "Parametrischer Entzerrer x16 Stereo",
            "Parametric Equalizer x16 Stereo",
            "Parametric Equalizer x16 Stereo",
            "PE16S",
            &developers::v_sadovnikov,
            "para_equalizer_x16_stereo",
            {
                LSP_LV2_URI("para_equalizer_x16_stereo"),
                LSP_LV2UI_URI("para_equalizer_x16_stereo"),
                "a5er",
                LSP_VST3_UID("pe16s   a5er"),
                LSP_VST3UI_UID("pe16s   a5er"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 2,
                LSP_LADSPA_URI("para_equalizer_x16_stereo"),
                LSP_CLAP_URI("para_equalizer_x16_stereo"),
                LSP_GST_UID("para_equalizer_x16_stereo"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x16_stereo_ports,
            "equalizer/parametric/stereo.xml",
            "equalizer/parametric/stereo",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x32_stereo =
        {
            "Parametrischer Entzerrer x32 Stereo",
            "Parametric Equalizer x32 Stereo",
            "Parametric Equalizer x32 Stereo",
            "PE32S",
            &developers::v_sadovnikov,
            "para_equalizer_x32_stereo",
            {
                LSP_LV2_URI("para_equalizer_x32_stereo"),
                LSP_LV2UI_URI("para_equalizer_x32_stereo"),
                "s2nz",
                LSP_VST3_UID("pe32s   s2nz"),
                LSP_VST3UI_UID("pe32s   s2nz"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 3,
                LSP_LADSPA_URI("para_equalizer_x32_stereo"),
                LSP_CLAP_URI("para_equalizer_x32_stereo"),
                LSP_GST_UID("para_equalizer_x32_stereo"),
            },
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

        const meta::plugin_t para_equalizer_x8_lr =
        {
            "Parametrischer Entzerrer x8 LeftRight",
            "Parametric Equalizer x8 LeftRight",
            "Parametric Equalizer x8 L/R",
            "PE8LR",
            &developers::v_sadovnikov,
            "para_equalizer_x8_lr",
            {
                LSP_LV2_URI("para_equalizer_x8_lr"),
                LSP_LV2UI_URI("para_equalizer_x8_lr"),
                "4ke8",
                LSP_VST3_UID("pe8lr   4ke8"),
                LSP_VST3UI_UID("pe8lr   4ke8"),
                LSP_LADSPA_PARA_EQUALIZER_X8 + 2,
                LSP_LADSPA_URI("para_equalizer_x8_lr"),
                LSP_CLAP_URI("para_equalizer_x8_lr"),
                LSP_GST_UID("para_equalizer_x8_lr"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x8_lr_ports,
            "equalizer/parametric/lr.xml",
            "equalizer/parametric/lr",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x16_lr =
        {
            "Parametrischer Entzerrer x16 LeftRight",
            "Parametric Equalizer x16 LeftRight",
            "Parametric Equalizer x16 L/R",
            "PE16LR",
            &developers::v_sadovnikov,
            "para_equalizer_x16_lr",
            {
                LSP_LV2_URI("para_equalizer_x16_lr"),
                LSP_LV2UI_URI("para_equalizer_x16_lr"),
                "4kef",
                LSP_VST3_UID("pe16lr  4kef"),
                LSP_VST3UI_UID("pe16lr  4kef"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 4,
                LSP_LADSPA_URI("para_equalizer_x16_lr"),
                LSP_CLAP_URI("para_equalizer_x16_lr"),
                LSP_GST_UID("para_equalizer_x16_lr"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x16_lr_ports,
            "equalizer/parametric/lr.xml",
            "equalizer/parametric/lr",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x32_lr =
        {
            "Parametrischer Entzerrer x32 LeftRight",
            "Parametric Equalizer x32 LeftRight",
            "Parametric Equalizer x32 L/R",
            "PE32LR",
            &developers::v_sadovnikov,
            "para_equalizer_x32_lr",
            {
                LSP_LV2_URI("para_equalizer_x32_lr"),
                LSP_LV2UI_URI("para_equalizer_x32_lr"),
                "ilqj",
                LSP_VST3_UID("pe32lr  ilqj"),
                LSP_VST3UI_UID("pe32lr  ilqj"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 5,
                LSP_LADSPA_URI("para_equalizer_x32_lr"),
                LSP_CLAP_URI("para_equalizer_x32_lr"),
                LSP_GST_UID("para_equalizer_x32_lr"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x32_lr_ports,
            "equalizer/parametric/lr.xml",
            "equalizer/parametric/lr",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x8_ms =
        {
            "Parametrischer Entzerrer x8 MidSide",
            "Parametric Equalizer x8 MidSide",
            "Parametric Equalizer x8 M/S",
            "PE8MS",
            &developers::v_sadovnikov,
            "para_equalizer_x8_ms",
            {
                LSP_LV2_URI("para_equalizer_x8_ms"),
                LSP_LV2UI_URI("para_equalizer_x8_ms"),
                "opj8",
                LSP_VST3_UID("pe8ms   opj8"),
                LSP_VST3UI_UID("pe8ms   opj8"),
                LSP_LADSPA_PARA_EQUALIZER_X8 + 3,
                LSP_LADSPA_URI("para_equalizer_x8_ms"),
                LSP_CLAP_URI("para_equalizer_x8_ms"),
                LSP_GST_UID("para_equalizer_x8_ms"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x8_ms_ports,
            "equalizer/parametric/ms.xml",
            "equalizer/parametric/ms",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x16_ms =
        {
            "Parametrischer Entzerrer x16 MidSide",
            "Parametric Equalizer x16 MidSide",
            "Parametric Equalizer x16 M/S",
            "PE16MS",
            &developers::v_sadovnikov,
            "para_equalizer_x16_ms",
            {
                LSP_LV2_URI("para_equalizer_x16_ms"),
                LSP_LV2UI_URI("para_equalizer_x16_ms"),
                "opjs",
                LSP_VST3_UID("pe16ms  opjs"),
                LSP_VST3UI_UID("pe16ms  opjs"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 6,
                LSP_LADSPA_URI("para_equalizer_x16_ms"),
                LSP_CLAP_URI("para_equalizer_x16_ms"),
                LSP_GST_UID("para_equalizer_x16_ms"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x16_ms_ports,
            "equalizer/parametric/ms.xml",
            "equalizer/parametric/ms",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };

        const meta::plugin_t para_equalizer_x32_ms =
        {
            "Parametrischer Entzerrer x32 MidSide",
            "Parametric Equalizer x32 MidSide",
            "Parametric Equalizer x32 M/S",
            "PE32MS",
            &developers::v_sadovnikov,
            "para_equalizer_x32_ms",
            {
                LSP_LV2_URI("para_equalizer_x32_ms"),
                LSP_LV2UI_URI("para_equalizer_x32_ms"),
                "lgz9",
                LSP_VST3_UID("pe32ms  lgz9"),
                LSP_VST3UI_UID("pe32ms  lgz9"),
                LSP_LADSPA_PARA_EQUALIZER_BASE + 7,
                LSP_LADSPA_URI("para_equalizer_x32_ms"),
                LSP_CLAP_URI("para_equalizer_x32_ms"),
                LSP_GST_UID("para_equalizer_x32_ms"),
            },
            LSP_PLUGINS_PARA_EQUALIZER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_INLINE_DISPLAY | E_DUMP_STATE,
            para_equalizer_x32_ms_ports,
            "equalizer/parametric/ms.xml",
            "equalizer/parametric/ms",
            stereo_plugin_port_groups,
            &para_equalizer_bundle
        };
    } /* namespace meta */
} /* namespace lsp */


