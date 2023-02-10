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

#ifndef PRIVATE_UI_PARA_EQUALIZER_H_
#define PRIVATE_UI_PARA_EQUALIZER_H_

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/lltl/darray.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * UI for Parametric Equalizer plugin series
         */
        class para_equalizer_ui: public ui::Module, public ui::IPortListener
        {
            protected:
                typedef struct filter_t
                {
                    ui::IPort          *pType;
                    ui::IPort          *pMode;
                    ui::IPort          *pSlope;
                    ui::IPort          *pSolo;
                    ui::IPort          *pMute;
                    tk::GraphDot       *wDot;
                    tk::Button         *wInspect;
                } dot_t;

            protected:
                ui::IPort          *pRewPath;
                ui::IPort          *pInspect;           // Inspected filter index
                tk::FileDialog     *pRewImport;
                tk::Graph          *wGraph;
                tk::Button         *wInspectReset;      // Inspect reset button
                const char        **fmtStrings;
                ssize_t             nXAxisIndex;
                ssize_t             nYAxisIndex;
                ssize_t             nSplitChannels;
                size_t              nFilters;

                filter_t           *pCurrDot;           // Current filter associated with dot
                tk::Menu           *wFilterMenu;        // Popup menu for filter properties
                tk::MenuItem       *wFilterInspect;
                tk::MenuItem       *wFilterSolo;
                tk::MenuItem       *wFilterMute;
                lltl::parray<tk::MenuItem> vFilterTypes;
                lltl::parray<tk::MenuItem> vFilterModes;
                lltl::parray<tk::MenuItem> vFilterSlopes;
                lltl::darray<filter_t> vFilters;

            protected:
                static status_t slot_start_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_call_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_fetch_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_commit_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_graph_dbl_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_menu_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_dot_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_inspect_submit(tk::Widget *sender, void *ptr, void *data);

            protected:
                tk::Menu       *create_menu();
                tk::MenuItem   *create_menu_item(tk::Menu *parent, const char *text);
                tk::Menu       *create_submenu(tk::Menu *parent, const char *lc_key,
                    lltl::parray<tk::MenuItem> *items, const meta::port_t *port);
                void            set_menu_items_checked(lltl::parray<tk::MenuItem> *list, ui::IPort *port);

                template <class T>
                T              *find_filter_widget(const char *fmt, const char *base, size_t id);

            protected:
                status_t        import_rew_file(const LSPString *path);
                void            set_port_value(const char *base, size_t id, size_t mask, float value);
                ssize_t         find_axis(const char *id);

                void            on_graph_dbl_click(ssize_t x, ssize_t y);

                void            on_filter_dot_right_click(tk::Widget *dot, ssize_t x, ssize_t y);
                void            on_filter_menu_item_submit(tk::MenuItem *mi);
                void            on_filter_menu_item_selected(lltl::parray<tk::MenuItem> *list, ui::IPort *port, tk::MenuItem *mi);
                void            on_filter_inspect_submit(tk::Widget *button);

                void            set_filter_mode(size_t id, size_t mask, size_t value);
                void            set_filter_type(size_t id, size_t mask, size_t value);
                void            set_filter_frequency(size_t id, size_t mask, float value);
                void            set_filter_quality(size_t id, size_t mask, float value);
                void            set_filter_gain(size_t id, size_t mask, float value);
                void            set_filter_slope(size_t id, size_t mask, size_t slope);
                void            set_filter_enabled(size_t id, size_t mask, bool enabled);
                void            set_filter_solo(size_t id, size_t mask, bool solo);

                ui::IPort      *find_port(const char *fmt, const char *base, size_t id);

                ssize_t         get_filter_type(size_t id, size_t channel);

                filter_t       *find_filter_by_dot(tk::Widget *dot);
                filter_t       *find_filter_by_inspect(tk::Widget *button);
                void            add_filters();
                void            create_filter_menu();
                void            select_inspected_filter(filter_t *f, bool commit);
                void            toggle_inspected_filter(filter_t *f, bool commit);
                bool            filter_inspect_can_be_enabled(filter_t *f);
                void            sync_filter_inspect_state();
                bool            is_filter_inspect_port(ui::IPort *port);

            public:
                explicit para_equalizer_ui(const meta::plugin_t *meta);
                virtual ~para_equalizer_ui() override;

                virtual status_t    post_init() override;

                virtual void        notify(ui::IPort *port) override;
        };
    } // namespace plugins
} // namespace lsp

#endif /* PRIVATE_UI_PARA_EQUALIZER_H_ */
