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

#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/fmt/RoomEQWizard.h>
#include <lsp-plug.in/stdlib/string.h>

#include <private/meta/para_equalizer.h>
#include <private/ui/para_equalizer.h>

namespace lsp
{
    namespace plugins
    {
        //---------------------------------------------------------------------
        // Plugin UI factory
        static const meta::plugin_t *plugin_uis[] =
        {
            &meta::para_equalizer_x16_mono,
            &meta::para_equalizer_x16_stereo,
            &meta::para_equalizer_x16_lr,
            &meta::para_equalizer_x16_ms,
            &meta::para_equalizer_x32_mono,
            &meta::para_equalizer_x32_stereo,
            &meta::para_equalizer_x32_lr,
            &meta::para_equalizer_x32_ms
        };

        static ui::Module *ui_factory(const meta::plugin_t *meta)
        {
            return new para_equalizer_ui(meta);
        }

        static ui::Factory factory(ui_factory, plugin_uis, 8);

        //---------------------------------------------------------------------
        static const char *fmt_strings[] =
        {
            "%s_%d",
            NULL
        };

        static const char *fmt_strings_lr[] =
        {
            "%sl_%d",
            "%sr_%d",
            NULL
        };

        static const char *fmt_strings_ms[] =
        {
            "%sm_%d",
            "%ss_%d",
            NULL
        };

        para_equalizer_ui::para_equalizer_ui(const meta::plugin_t *meta): ui::Module(meta)
        {
            pRewPath        = NULL;
            pRewImport      = NULL;
            pGraph          = NULL;
            fmtStrings      = fmt_strings;
            nSplitChannels  = 1;
            nXAxisIndex     = -1;
            nYAxisIndex     = -1;

            if ((!strcmp(meta->uid, meta::para_equalizer_x16_lr.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x32_lr.uid)))
            {
                fmtStrings      = fmt_strings_lr;
                nSplitChannels  = 2;
            }
            else if ((!strcmp(meta->uid, meta::para_equalizer_x16_ms.uid)) ||
                 (!strcmp(meta->uid, meta::para_equalizer_x32_ms.uid)))
            {
                fmtStrings      = fmt_strings_ms;
                nSplitChannels  = 2;
            }
        }

        para_equalizer_ui::~para_equalizer_ui()
        {
            pRewImport = NULL;      // Will be automatically destroyed from list of widgets
        }

        status_t para_equalizer_ui::slot_start_import_rew_file(tk::Widget *sender, void *ptr, void *data)
        {
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);

            ctl::Window *wnd    = _this->wrapper()->controller();
            tk::FileDialog *dlg = _this->pRewImport;
            if (dlg == NULL)
            {
                dlg = new tk::FileDialog(_this->pDisplay);
                if (dlg == NULL)
                    return STATUS_NO_MEM;

                wnd->widgets()->add(dlg);
                _this->pRewImport  = dlg;

                dlg->init();
                dlg->mode()->set(tk::FDM_OPEN_FILE);
                dlg->title()->set("titles.import_rew_filter_settings");
                dlg->action_text()->set("actions.import");

                tk::FileFilters *f = dlg->filter();
                {
                    tk::FileMask *ffi;

                    if ((ffi = f->add()) != NULL)
                    {
                        ffi->pattern()->set("*.req|*.txt");
                        ffi->title()->set("files.roomeqwizard.all");
                        ffi->extensions()->set("");
                    }
                    if ((ffi = f->add()) != NULL)
                    {
                        ffi->pattern()->set("*.req");
                        ffi->title()->set("files.roomeqwizard.req");
                        ffi->extensions()->set("");
                    }
                    if ((ffi = f->add()) != NULL)
                    {
                        ffi->pattern()->set("*.txt");
                        ffi->title()->set("files.roomeqwizard.txt");
                        ffi->extensions()->set("");
                    }
                    if ((ffi = f->add()) != NULL)
                    {
                        ffi->pattern()->set("*");
                        ffi->title()->set("files.all");
                        ffi->extensions()->set("");
                    }
                }
                dlg->slots()->bind(tk::SLOT_SUBMIT, slot_call_import_rew_file, ptr);
                dlg->slots()->bind(tk::SLOT_SHOW, slot_fetch_rew_path, _this);
                dlg->slots()->bind(tk::SLOT_HIDE, slot_commit_rew_path, _this);
            }

            dlg->show(wnd->widget());

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_call_import_rew_file(tk::Widget *sender, void *ptr, void *data)
        {
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            LSPString path;
            status_t res = _this->pRewImport->selected_file()->format(&path);
            if (res == STATUS_OK)
                res = _this->import_rew_file(&path);
            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_fetch_rew_path(tk::Widget *sender, void *ptr, void *data)
        {
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if ((_this == NULL) || (_this->pRewPath == NULL))
                return STATUS_BAD_STATE;

            _this->pRewImport->path()->set_raw(_this->pRewPath->buffer<char>());
            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_commit_rew_path(tk::Widget *sender, void *ptr, void *data)
        {
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if ((_this == NULL) || (_this->pRewPath == NULL))
                return STATUS_BAD_STATE;

            LSPString path;
            if (_this->pRewImport->path()->format(&path) == STATUS_OK)
            {
                const char *u8path = path.get_utf8();
                _this->pRewPath->write(u8path, ::strlen(u8path));
                _this->pRewPath->notify_all();
            }

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_graph_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            ws::event_t *ev = static_cast<ws::event_t *>(data);
            _this->on_graph_dbl_click(ev->nLeft, ev->nTop);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::post_init()
        {
            status_t res = ui::Module::post_init();
            if (res != STATUS_OK)
                return res;

            // Find REW port
            pRewPath            = pWrapper->port(UI_CONFIG_PORT_PREFIX UI_DLG_REW_PATH_ID);

            // Add subwidgets
            ctl::Window *wnd    = pWrapper->controller();
            tk::Menu *menu      = tk::widget_cast<tk::Menu>(wnd->widgets()->find(WUID_IMPORT_MENU));
            if (menu != NULL)
            {
                tk::MenuItem *child = new tk::MenuItem(pDisplay);
                if (child == NULL)
                    return STATUS_NO_MEM;

                wnd->widgets()->add(child);
                child->init();
                child->text()->set("actions.import_rew_filter_file");
                child->slots()->bind(tk::SLOT_SUBMIT, slot_start_import_rew_file, this);
                menu->add(child);
            }

            // Bind double-click handler
            pGraph              = tk::widget_cast<tk::Graph>(wnd->widgets()->find("para_eq_graph"));
            if (pGraph != NULL)
            {
                pGraph->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_graph_dbl_click, this);
                nXAxisIndex         = find_axis("para_eq_ox");
                nYAxisIndex         = find_axis("para_eq_oy");
            }

            return STATUS_OK;
        }

        void para_equalizer_ui::on_graph_dbl_click(ssize_t x, ssize_t y)
        {
            if ((pGraph == NULL) || (nXAxisIndex < 0) || (nYAxisIndex < 0))
                return;

            float freq = 0.0f, gain = 0.0f;
            if (pGraph->xy_to_axis(nXAxisIndex, &freq, x, y) != STATUS_OK)
                return;
            if (pGraph->xy_to_axis(nYAxisIndex, &gain, x, y) != STATUS_OK)
                return;

            lsp_trace("Double click: x=%d, y=%d, freq=%.2f, gain=%.4f (%.2f db)",
                x, y, freq, gain, dspu::gain_to_db(gain));

            // Obtain which port set (left/right/mid/side) should be updated
            ui::IPort *selector = pWrapper->port("fsel");
            ssize_t channel     = (selector != NULL) ? size_t(selector->value()) % nSplitChannels : 0;
            if (channel < 0)
            {
                lsp_trace("Could not allocate channel");
                return;
            }

            // Allocate new port index
            ssize_t fid         = -1;
            for (size_t i=0; i<32; ++i)
            {
                ssize_t type = get_filter_type(i, channel);
                if (type == meta::para_equalizer_metadata::EQF_OFF)
                {
                    fid             = i;
                    break;
                }
                else if (type < 0)
                    break;
            }

            if (fid < 0)
            {
                lsp_trace("Could not allocate new equalizer band");
                return;
            }

            // Equalizer band has been allocated, do the stuff
            size_t mask         = 1 << channel;

            // Set-up parameters
            size_t filter_type =
                (freq <= 100.0f)    ? meta::para_equalizer_metadata::EQF_HIPASS     :
                (freq <= 300.0f)    ? meta::para_equalizer_metadata::EQF_LOSHELF    :
                (freq <= 7000.0f)   ? meta::para_equalizer_metadata::EQF_BELL       :
                (freq <= 15000.0f)  ? meta::para_equalizer_metadata::EQF_HISHELF    :
                                      meta::para_equalizer_metadata::EQF_LOPASS;

            float filter_quality = (filter_type == meta::para_equalizer_metadata::EQF_BELL) ? 2.0f : 0.5f;

            // Set-up filter type
            set_filter_mode(fid, mask, meta::para_equalizer_metadata::EFM_RLC_BT);
            set_filter_type(fid, mask, filter_type);
            set_filter_frequency(fid, mask, freq);
            set_filter_slope(fid, mask, 1);
            set_filter_gain(fid, mask, gain);
            set_filter_quality(fid, mask, filter_quality);
            set_filter_enabled(fid, mask, true);
            set_filter_solo(fid, mask, false);
        }

        ssize_t para_equalizer_ui::find_axis(const char *id)
        {
            if (pGraph == NULL)
                return -1;

            ctl::Window *wnd    = pWrapper->controller();
            tk::GraphAxis *axis = tk::widget_cast<tk::GraphAxis>(wnd->widgets()->find(id));
            if (axis == NULL)
                return -1;

            for (size_t i=0; ; ++i)
            {
                tk::GraphAxis *ax = pGraph->axis(i);
                if (ax == NULL)
                    break;
                else if (ax == axis)
                    return i;
            }

            return -1;
        }

        void para_equalizer_ui::set_port_value(const char *base, size_t mask, size_t id, float value)
        {
            char port_id[32];
            size_t pattern = 1 << 0;

            for (const char **fmt = fmtStrings; *fmt != NULL; ++fmt, pattern <<= 1)
            {
                if (!(mask & pattern))
                    continue;

                ::snprintf(port_id, sizeof(port_id)/sizeof(char), *fmt, base, int(id));
                ui::IPort *p = pWrapper->port(port_id);
                if (p != NULL)
                {
                    p->set_value(value);
                    p->notify_all();
                }
            }
        }

        void para_equalizer_ui::set_filter_mode(size_t id, size_t mask, size_t value)
        {
            set_port_value("fm", mask, id, value);
        }

        void para_equalizer_ui::set_filter_type(size_t id, size_t mask, size_t value)
        {
            set_port_value("ft", mask, id, value);
        }

        ssize_t para_equalizer_ui::get_filter_type(size_t id, size_t channel)
        {
            char port_id[32];
            size_t index = 0;

            for (const char **fmt = fmtStrings; *fmt != NULL; ++fmt, ++index)
            {
                if (index != channel)
                    continue;

                ::snprintf(port_id, sizeof(port_id)/sizeof(char), *fmt, "ft", int(id));
                ui::IPort *p = pWrapper->port(port_id);
                if (p == NULL)
                    return -STATUS_NOT_FOUND;

                return ssize_t(p->value());
            }

            return -STATUS_NOT_FOUND;
        }

        void para_equalizer_ui::set_filter_frequency(size_t id, size_t mask, float value)
        {
            set_port_value("f", mask, id, value);
        }

        void para_equalizer_ui::set_filter_quality(size_t id, size_t mask, float value)
        {
            set_port_value("q", mask, id, value);
        }

        void para_equalizer_ui::set_filter_gain(size_t id, size_t mask, float value)
        {
            set_port_value("g", mask, id, value);
        }

        void para_equalizer_ui::set_filter_slope(size_t id, size_t mask, size_t slope)
        {
            set_port_value("s", mask, id, slope - 1);
        }

        void para_equalizer_ui::set_filter_enabled(size_t id, size_t mask, bool enabled)
        {
            set_port_value("xm", mask, id, (enabled) ? 0.0f : 1.0f);
        }

        void para_equalizer_ui::set_filter_solo(size_t id, size_t mask, bool solo)
        {
            set_port_value("xs", mask, id, (solo) ? 1.0f : 0.0f);
        }

        status_t para_equalizer_ui::import_rew_file(const LSPString *path)
        {
            // Load settings
            room_ew::config_t *cfg = NULL;
            status_t res = room_ew::load(path, &cfg);
            if (res != STATUS_OK)
                return res;

            // Apply settings
            size_t fid = 0;
            for (size_t i=0; i<cfg->nFilters; ++i)
            {
                const room_ew::filter_t *f = &cfg->vFilters[i];

                // Perform parameter translation
                size_t mode     = meta::para_equalizer_metadata::EFM_APO_DR;
                ssize_t type    = -1;
                double gain     = 0.0;
                double quality  = M_SQRT1_2;
                double freq     = f->fc;

                switch (f->filterType)
                {
                    case room_ew::PK:
                        type    = meta::para_equalizer_metadata::EQF_BELL;
                        gain    = f->gain;
                        quality = f->Q;
                        break;
                    case room_ew::LS:
                        type    = meta::para_equalizer_metadata::EQF_LOSHELF;
                        gain    = f->gain;
                        quality = 2.0/3.0;
                        break;
                    case room_ew::HS:
                        type    = meta::para_equalizer_metadata::EQF_HISHELF;
                        gain    = f->gain;
                        quality = 2.0/3.0;
                        break;
                    case room_ew::LP:
                        type    = meta::para_equalizer_metadata::EQF_LOPASS;
                        break;
                    case room_ew::HP:
                        type    = meta::para_equalizer_metadata::EQF_HIPASS;
                        break;
                    case room_ew::LPQ:
                        type    = meta::para_equalizer_metadata::EQF_LOPASS;
                        quality = f->Q;
                        break;
                    case room_ew::HPQ:
                        type    = meta::para_equalizer_metadata::EQF_HIPASS;
                        quality = f->Q;
                        break;
                    case room_ew::LS6:
                        type    = meta::para_equalizer_metadata::EQF_LOSHELF;
                        gain    = f->gain;
                        quality = M_SQRT2 / 3.0;
                        freq    = freq * 2.0 / 3.0;
                        break;
                    case room_ew::HS6:
                        type    = meta::para_equalizer_metadata::EQF_HISHELF;
                        gain    = f->gain;
                        quality = M_SQRT2 / 3.0;
                        freq    = freq / M_SQRT1_2;
                        break;
                    case room_ew::LS12:
                        type    = meta::para_equalizer_metadata::EQF_LOSHELF;
                        gain    = f->gain;
                        freq    = freq * 3.0 / 2.0;
                        break;
                    case room_ew::HS12:
                        type    = meta::para_equalizer_metadata::EQF_HISHELF;
                        gain    = f->gain;
                        freq    = freq * M_SQRT1_2;
                        break;
                    case room_ew::NO:
                        type    = meta::para_equalizer_metadata::EQF_NOTCH;
                        quality = 100.0 / 3.0;
                        break;
                    case room_ew::AP:
                        type    = meta::para_equalizer_metadata::EQF_ALLPASS;
                        quality = 0.0;
                        break;
                    default: // Skip other filter types
                        break;
                }

                if (type < 0)
                    continue;

                // Set-up parameters
                set_filter_mode(fid, 0x03, mode);
                set_filter_type(fid, 0x03, type);
                set_filter_slope(fid, 0x03, 1);
                set_filter_frequency(fid, 0x03, freq);
                set_filter_gain(fid, 0x03, dspu::db_to_gain(gain));
                set_filter_quality(fid, 0x03, quality);
                set_filter_enabled(fid, 0x03, f->enabled);
                set_filter_solo(fid, 0x03, false);

                // Increment imported filter number
                ++fid;
            }

            // Reset state of all other filters
            for (; fid < 32; ++fid)
            {
                set_filter_type(fid, 0x03, meta::para_equalizer_metadata::EQF_OFF);
                set_filter_slope(fid, 0x03, 1);
                set_filter_gain(fid, 0x03, 1.0f);
                set_filter_quality(fid, 0x03, 0.0f);
                set_filter_enabled(fid, 0x03, true);
                set_filter_solo(fid, 0x03, false);
            }

            return STATUS_OK;
        }
    }
}


