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

        static ui::Factory factory(plugin_uis, 8);

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
            pRewImport  = NULL;
            pRewPath    = NULL;
            fmtStrings  = fmt_strings;

            if (::strstr(meta->lv2_urid, "_lr") != NULL)
                fmtStrings      = fmt_strings_lr;
            else if (::strstr(meta->lv2_urid, "_ms") != NULL)
                fmtStrings      = fmt_strings_ms;
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

            return STATUS_OK;
        }

        void para_equalizer_ui::set_port_value(const char *base, size_t id, float value)
        {
            char port_id[32];

            for (const char **fmt = fmtStrings; *fmt != NULL; ++fmt)
            {
                ::snprintf(port_id, sizeof(port_id)/sizeof(char), *fmt, base, int(id));
                ui::IPort *p = pWrapper->port(port_id);
                if (p != NULL)
                {
                    p->set_value(value);
                    p->notify_all();
                }
            }
        }

        void para_equalizer_ui::set_filter_mode(size_t id, size_t value)
        {
            set_port_value("fm", id, value);
        }

        void para_equalizer_ui::set_filter_type(size_t id, size_t value)
        {
            set_port_value("ft", id, value);
        }

        void para_equalizer_ui::set_filter_frequency(size_t id, double value)
        {
            set_port_value("f", id, value);
        }

        void para_equalizer_ui::set_filter_quality(size_t id, double value)
        {
            set_port_value("q", id, value);
        }

        void para_equalizer_ui::set_filter_gain(size_t id, double value)
        {
            double gain = expf(0.05 * value * M_LN10);
            set_port_value("g", id, gain);
        }

        void para_equalizer_ui::set_filter_slope(size_t id, size_t slope)
        {
            set_port_value("s", id, slope - 1);
        }

        void para_equalizer_ui::set_filter_enabled(size_t id, bool enabled)
        {
            set_port_value("xm", id, (enabled) ? 0.0f : 1.0f);
        }

        void para_equalizer_ui::set_filter_solo(size_t id, bool solo)
        {
            set_port_value("xs", id, (solo) ? 1.0f : 0.0f);
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
                set_filter_mode(fid, mode);
                set_filter_type(fid, type);
                set_filter_slope(fid, 1);
                set_filter_frequency(fid, freq);
                set_filter_gain(fid, gain);
                set_filter_quality(fid, quality);
                set_filter_enabled(fid, f->enabled);
                set_filter_solo(fid, false);

                // Increment imported filter number
                ++fid;
            }

            // Reset state of all other filters
            for (; fid < 32; ++fid)
            {
                set_filter_type(fid, meta::para_equalizer_metadata::EQF_OFF);
                set_filter_slope(fid, 1);
                set_filter_gain(fid, 1.0f);
                set_filter_quality(fid, 0.0f);
                set_filter_enabled(fid, true);
                set_filter_solo(fid, false);
            }

            return STATUS_OK;
        }
    }
}


