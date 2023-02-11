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
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/tk/tk.h>

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

        static const tk::tether_t dot_menu_tether_list[] =
        {
            { tk::TF_BOTTOM | tk::TF_LEFT,  1.0f, 1.0f },
            { tk::TF_BOTTOM | tk::TF_RIGHT, -1.0f, 1.0f },
            { tk::TF_TOP | tk::TF_LEFT,  1.0f, -1.0f },
            { tk::TF_TOP | tk::TF_RIGHT, -1.0f, -1.0f }
        };

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

        template <class T>
        T *para_equalizer_ui::find_filter_widget(const char *fmt, const char *base, size_t id)
        {
            char widget_id[64];
            ::snprintf(widget_id, sizeof(widget_id)/sizeof(char), fmt, base, int(id));
            return pWrapper->controller()->widgets()->get<T>(widget_id);
        }

        para_equalizer_ui::para_equalizer_ui(const meta::plugin_t *meta): ui::Module(meta)
        {
            pRewPath        = NULL;
            pInspect        = NULL;
            pRewImport      = NULL;
            wGraph          = NULL;
            wInspectReset   = NULL;
            fmtStrings      = fmt_strings;
            nSplitChannels  = 1;
            nXAxisIndex     = -1;
            nYAxisIndex     = -1;

            pCurrDot        = NULL;
            wFilterMenu     = NULL;
            wFilterInspect  = NULL;
            wFilterSolo     = NULL;
            wFilterMute     = NULL;

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

            nFilters       = 16;
            if ((!strcmp(meta->uid, meta::para_equalizer_x32_lr.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x32_mono.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x32_ms.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x32_stereo.uid)))
                nFilters       = 32;
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

        status_t para_equalizer_ui::slot_filter_menu_submit(tk::Widget *sender, void *ptr, void *data)
        {
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if ((_this == NULL) || (_this->pCurrDot == NULL))
                return STATUS_BAD_STATE;
            tk::MenuItem *mi = tk::widget_cast<tk::MenuItem>(sender);
            if (mi == NULL)
                return STATUS_BAD_ARGUMENTS;

            _this->on_filter_menu_item_submit(mi);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_filter_dot_click(tk::Widget *sender, void *ptr, void *data)
        {
            // Process the right mouse click
            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev->nCode != ws::MCB_RIGHT)
                return STATUS_OK;

            // Fetch paramters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            // Process the event
            _this->on_filter_dot_right_click(sender, ev->nLeft, ev->nTop);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_filter_inspect_submit(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch paramters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            // Process the event
            _this->on_filter_inspect_submit(sender);

            return STATUS_OK;
        }

        para_equalizer_ui::filter_t *para_equalizer_ui::find_filter_by_inspect(tk::Widget *button)
        {
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *d = vFilters.uget(i);
                if (d->wInspect == button)
                    return d;
            }
            return NULL;
        }

        para_equalizer_ui::filter_t *para_equalizer_ui::find_filter_by_dot(tk::Widget *dot)
        {
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *d = vFilters.uget(i);
                if (d->wDot == dot)
                    return d;
            }
            return NULL;
        }

        void para_equalizer_ui::add_filters()
        {
            for (const char **fmt = fmtStrings; *fmt != NULL; ++fmt)
            {
                for (size_t port_id=0; port_id<nFilters; ++port_id)
                {
                    filter_t dot;
                    dot.wDot        = find_filter_widget<tk::GraphDot>(*fmt, "filter_dot", port_id);
                    if (dot.wDot == NULL)
                        continue;

                    dot.wInspect    = find_filter_widget<tk::Button>(*fmt, "filter_inspect", port_id);

                    dot.pType       = find_port(*fmt, "ft", port_id);
                    dot.pMode       = find_port(*fmt, "fm", port_id);
                    dot.pSlope      = find_port(*fmt, "s", port_id);
                    dot.pSolo       = find_port(*fmt, "xs", port_id);
                    dot.pMute       = find_port(*fmt, "xm", port_id);

                    if ((dot.pType == NULL) ||
                        (dot.pMode == NULL) ||
                        (dot.pSlope == NULL) ||
                        (dot.pSolo == NULL) ||
                        (dot.pMute == NULL))
                        continue;

                    dot.wDot->slots()->bind(tk::SLOT_MOUSE_CLICK, slot_filter_dot_click, this);
                    dot.wInspect->slots()->bind(tk::SLOT_SUBMIT, slot_filter_inspect_submit, this);

                    dot.pType->bind(this);
                    dot.pSolo->bind(this);
                    dot.pMute->bind(this);

                    vFilters.add(&dot);
                }
            }
        }

        tk::Menu *para_equalizer_ui::create_menu()
        {
            tk::Menu *menu = new tk::Menu(pWrapper->display());
            if (menu == NULL)
                return NULL;
            if ((menu->init() != STATUS_OK) ||
                (pWrapper->controller()->widgets()->add(menu) != STATUS_OK))
            {
                menu->destroy();
                delete menu;
                return NULL;
            }
            return menu;
        }

        tk::MenuItem *para_equalizer_ui::create_menu_item(tk::Menu *parent, const char *text)
        {
            tk::MenuItem *mi = new tk::MenuItem(pWrapper->display());
            if (mi == NULL)
                return NULL;
            if ((mi->init() != STATUS_OK) ||
                (pWrapper->controller()->widgets()->add(mi) != STATUS_OK))
            {
                mi->destroy();
                delete mi;
                return NULL;
            }
            if (parent != NULL)
            {
                if (parent->add(mi) != STATUS_OK)
                    return NULL;
            }
            mi->text()->set(text);

            return mi;
        }

        tk::Menu *para_equalizer_ui::create_submenu(tk::Menu *parent, const char *lc_key,
            lltl::parray<tk::MenuItem> *items, const meta::port_t *port)
        {
            if (port->items == NULL)
                return NULL;

            // Create menu item for the parent menu
            tk::MenuItem *root = create_menu_item(parent, lc_key);
            if (root == NULL)
                return NULL;

            // Create menu and bind to root menu item
            tk::Menu *menu = create_menu();
            if (menu == NULL)
                return NULL;
            root->menu()->set(menu);

            // Create menu items
            for (const meta::port_item_t *pi = port->items; pi->text != NULL; ++pi)
            {
                // Create submenu item
                LSPString key;
                if (!key.append_ascii("lists."))
                    return NULL;
                if (!key.append_ascii(pi->lc_key))
                    return NULL;
                tk::MenuItem *mi = create_menu_item(menu, key.get_ascii());
                if (mi == NULL)
                    return NULL;
                if (!items->add(mi))
                    return NULL;
                mi->type()->set_radio();
                mi->slots()->bind(tk::SLOT_SUBMIT, slot_filter_menu_submit, this);
            }

            return menu;
        }

        void para_equalizer_ui::create_filter_menu()
        {
            filter_t *dot = vFilters.get(0);
            if (dot == NULL)
                return;

            tk::Menu *root = create_menu();
            if (root == NULL)
                return;

            if ((create_submenu(root, "labels.filter", &vFilterTypes, dot->pType->metadata())) == NULL)
                return;
            if ((create_submenu(root, "labels.mode", &vFilterModes, dot->pMode->metadata())) == NULL)
                return;
            if ((create_submenu(root, "labels.slope", &vFilterSlopes, dot->pSlope->metadata())) == NULL)
                return;

            if ((wFilterInspect = create_menu_item(root, "labels.chan.inspect")) == NULL)
                return;
            wFilterInspect->type()->set_check();
            wFilterInspect->slots()->bind(tk::SLOT_SUBMIT, slot_filter_menu_submit, this);
            ctl::inject_style(wFilterInspect, "MenuItemChecked");

            if ((wFilterSolo = create_menu_item(root, "labels.chan.solo")) == NULL)
                return;
            wFilterSolo->type()->set_check();
            wFilterSolo->slots()->bind(tk::SLOT_SUBMIT, slot_filter_menu_submit, this);
            ctl::inject_style(wFilterSolo, "MenuItemChecked");

            if ((wFilterMute = create_menu_item(root, "labels.chan.mute")) == NULL)
                return;
            wFilterMute->type()->set_check();
            wFilterMute->slots()->bind(tk::SLOT_SUBMIT, slot_filter_menu_submit, this);
            ctl::inject_style(wFilterMute, "MenuItemChecked");

            wFilterMenu    = root;
        }

        status_t para_equalizer_ui::post_init()
        {
            status_t res = ui::Module::post_init();
            if (res != STATUS_OK)
                return res;

            // Add dot widgets
            add_filters();
            if (!vFilters.is_empty())
                create_filter_menu();

            // Find REW port
            pRewPath            = pWrapper->port(UI_CONFIG_PORT_PREFIX UI_DLG_REW_PATH_ID);
            pInspect            = pWrapper->port("insp_id");
            if (pInspect != NULL)
                pInspect->bind(this);

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
            wGraph              = wnd->widgets()->get<tk::Graph>("para_eq_graph");
            if (wGraph != NULL)
            {
                wGraph->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_graph_dbl_click, this);
                nXAxisIndex         = find_axis("para_eq_ox");
                nYAxisIndex         = find_axis("para_eq_oy");
            }

            wInspectReset       = wnd->widgets()->get<tk::Button>("filter_inspect_reset");
            if (wInspectReset != NULL)
            {
                wInspectReset->slots()->bind(tk::SLOT_SUBMIT, slot_filter_inspect_submit, this);
            }

            // Update state
            sync_filter_inspect_state();

            return STATUS_OK;
        }

        void para_equalizer_ui::set_menu_items_checked(lltl::parray<tk::MenuItem> *list, ui::IPort *port)
        {
            const meta::port_t *p = port->metadata();
            float min = 0.0f, max = 1.0f, step = 1.0f;
            meta::get_port_parameters(p, &min, &max, &step);

            ssize_t index   = (port->value() - min) / step;
            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                tk::MenuItem *mi = list->uget(i);
                mi->checked()->set(index == ssize_t(i));
            }
        }

        void para_equalizer_ui::on_filter_menu_item_selected(lltl::parray<tk::MenuItem> *list, ui::IPort *port, tk::MenuItem *mi)
        {
            ssize_t index = list->index_of(mi);
            if (index < 0)
                return;

            const meta::port_t *p = port->metadata();
            float min = 0.0f, max = 1.0f, step = 1.0f;
            meta::get_port_parameters(p, &min, &max, &step);

            port->set_value(min + index * step);
            port->notify_all();
        }

        void para_equalizer_ui::on_filter_menu_item_submit(tk::MenuItem *mi)
        {
            if (pCurrDot == NULL)
                return;
            lsp_finally { pCurrDot = NULL; };

            on_filter_menu_item_selected(&vFilterTypes, pCurrDot->pType, mi);
            on_filter_menu_item_selected(&vFilterModes, pCurrDot->pMode, mi);
            on_filter_menu_item_selected(&vFilterSlopes, pCurrDot->pSlope, mi);
            if (mi == wFilterMute)
            {
                pCurrDot->pMute->set_value((mi->checked()->get()) ? 0.0f : 1.0f);
                pCurrDot->pMute->notify_all();
            }
            if (mi == wFilterSolo)
            {
                pCurrDot->pSolo->set_value((mi->checked()->get()) ? 0.0f : 1.0f);
                pCurrDot->pSolo->notify_all();
            }
            if (mi == wFilterInspect)
                toggle_inspected_filter(pCurrDot, true);
        }

        void para_equalizer_ui::on_filter_dot_right_click(tk::Widget *dot, ssize_t x, ssize_t y)
        {
            // Is filter dot menu present?
            if (wFilterMenu == NULL)
                return;

            // Lookup for the dot
            if ((pCurrDot = find_filter_by_dot(dot)) == NULL)
                return;

            // Update state of menu elements
            set_menu_items_checked(&vFilterTypes, pCurrDot->pType);
            set_menu_items_checked(&vFilterModes, pCurrDot->pMode);
            set_menu_items_checked(&vFilterSlopes, pCurrDot->pSlope);

            if (pInspect != NULL)
            {
                ssize_t inspect = pInspect->value();
                ssize_t index   = vFilters.index_of(pCurrDot);
                wFilterInspect->checked()->set(index == inspect);
            }
            else
                wFilterInspect->checked()->set(false);
            wFilterMute->checked()->set(pCurrDot->pMute->value() >= 0.5f);
            wFilterSolo->checked()->set(pCurrDot->pSolo->value() >= 0.5f);

            // Show the dot menu
            ws::rectangle_t r;
            r.nLeft         = x;
            r.nTop          = y;
            r.nWidth        = 0;
            r.nHeight       = 0;

            tk::Window *wnd = tk::widget_cast<tk::Window>(dot->toplevel());
            if (wnd == NULL)
                return;

            if ((wnd->get_screen_rectangle(&r, &r) != STATUS_OK))
                return;

            wFilterMenu->set_tether(dot_menu_tether_list, sizeof(dot_menu_tether_list)/sizeof(dot_menu_tether_list[0]));
            wFilterMenu->show(pCurrDot->wDot->graph(), &r);
        }

        void para_equalizer_ui::on_graph_dbl_click(ssize_t x, ssize_t y)
        {
            if ((wGraph == NULL) || (nXAxisIndex < 0) || (nYAxisIndex < 0))
                return;

            float freq = 0.0f, gain = 0.0f;
            if (wGraph->xy_to_axis(nXAxisIndex, &freq, x, y) != STATUS_OK)
                return;
            if (wGraph->xy_to_axis(nYAxisIndex, &gain, x, y) != STATUS_OK)
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

        bool para_equalizer_ui::filter_inspect_can_be_enabled(filter_t *f)
        {
            if (f == NULL)
                return false;

            // The filter should not be muted
            bool has_solo   = false;
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *xf    = vFilters.uget(i);
                if (xf->pSolo->value() >= 0.5f)
                {
                    has_solo        = true;
                    break;
                }
            }

            bool mute       = f->pMute->value() >= 0.5f;
            bool solo       = f->pSolo->value() >= 0.5f;
            if ((mute) || ((has_solo) && (!solo)))
                return false;

            // The filter should be enabled
            return size_t(f->pType->value()) != meta::para_equalizer_metadata::EQF_OFF;
        }

        void para_equalizer_ui::sync_filter_inspect_state()
        {
            if (pInspect == NULL)
                return;

            ssize_t index = pInspect->value();
            filter_t *f = (index >= 0) ? vFilters.get(index) : NULL;
            select_inspected_filter(f, false);
        }

        void para_equalizer_ui::select_inspected_filter(filter_t *f, bool commit)
        {
            // Set the down state of inspect button for all filters
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *xf    = vFilters.uget(i);
                xf->wInspect->down()->set((f != NULL) ? (xf == f) : false);
            }

            // Update the inspection port
            ssize_t inspect = pInspect->value();
            ssize_t index = (f != NULL) ? vFilters.index_of(f) : -1;

            if ((commit) && (inspect != index))
            {
                pInspect->set_value(index);
                pInspect->notify_all();
                inspect     = index;
            }

            if (wInspectReset != NULL)
                wInspectReset->down()->set(inspect >= 0);
            if (pCurrDot == f)
                wFilterInspect->checked()->set((inspect >= 0) && (inspect == index));
        }

        void para_equalizer_ui::toggle_inspected_filter(filter_t *f, bool commit)
        {
            ssize_t curr    = pInspect->value();
            ssize_t index   = vFilters.index_of(f);

            if (curr == index)
                select_inspected_filter(NULL, true);
            else if (filter_inspect_can_be_enabled(f))
                select_inspected_filter(f, true);
        }

        void para_equalizer_ui::on_filter_inspect_submit(tk::Widget *button)
        {
            if (pInspect == NULL)
                return;

            // Filter inspect button submitted?
            filter_t *f     = find_filter_by_inspect(button);
            if (f != NULL)
                toggle_inspected_filter(f, true);

            // We need to reset inspection?
            if (button == wInspectReset)
                select_inspected_filter(NULL, true);
        }

        ssize_t para_equalizer_ui::find_axis(const char *id)
        {
            if (wGraph == NULL)
                return -1;

            ctl::Window *wnd    = pWrapper->controller();
            tk::GraphAxis *axis = tk::widget_cast<tk::GraphAxis>(wnd->widgets()->find(id));
            if (axis == NULL)
                return -1;

            for (size_t i=0; ; ++i)
            {
                tk::GraphAxis *ax = wGraph->axis(i);
                if (ax == NULL)
                    break;
                else if (ax == axis)
                    return i;
            }

            return -1;
        }

        ui::IPort *para_equalizer_ui::find_port(const char *fmt, const char *base, size_t id)
        {
            char port_id[32];
            ::snprintf(port_id, sizeof(port_id)/sizeof(char), fmt, base, int(id));
            return pWrapper->port(port_id);
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

        void para_equalizer_ui::notify(ui::IPort *port)
        {
            if (is_filter_inspect_port(port))
                sync_filter_inspect_state();
        }

        bool para_equalizer_ui::is_filter_inspect_port(ui::IPort *port)
        {
            if (pInspect == NULL)
                return false;

            if (port == pInspect)
                return true;

            ssize_t inspect = pInspect->value();
            if (inspect < 0)
                return false;

            filter_t *f = vFilters.get(inspect);
            if (f == NULL)
                return false;

            return (f->pType == port) ||
                (f->pSolo == port) ||
                (f->pMute == port);
        }

    } /* namespace plugins */
} /* namespace lsp */


