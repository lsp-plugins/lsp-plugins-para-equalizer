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
        class para_equalizer_ui: public ui::Module
        {
            protected:
                typedef struct dot_t
                {
                    ui::IPort          *pType;
                    ui::IPort          *pMode;
                    ui::IPort          *pSlope;
                    ui::IPort          *pSolo;
                    ui::IPort          *pMute;
                    tk::GraphDot       *pDot;
                } dot_t;

            protected:
                ui::IPort          *pRewPath;
                tk::FileDialog     *pRewImport;
                tk::Graph          *pGraph;
                const char        **fmtStrings;
                ssize_t             nXAxisIndex;
                ssize_t             nYAxisIndex;
                ssize_t             nSplitChannels;
                size_t              nFilters;

                dot_t              *pDotCurr;       // Current dot
                tk::Menu           *wDotMenu;       // Popup menu for dot properties
                tk::MenuItem       *wFilterSolo;
                tk::MenuItem       *wFilterMute;
                lltl::parray<tk::MenuItem> vFilterTypes;
                lltl::parray<tk::MenuItem> vFilterModes;
                lltl::parray<tk::MenuItem> vFilterSlopes;
                lltl::darray<dot_t> vDots;

            protected:
                static status_t slot_start_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_call_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_fetch_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_commit_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_graph_dbl_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_menu_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_dot_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                tk::Menu       *create_menu();
                tk::MenuItem   *create_menu_item(tk::Menu *parent, const char *text);

            protected:
                status_t        import_rew_file(const LSPString *path);
                void            set_port_value(const char *base, size_t id, size_t mask, float value);
                ssize_t         find_axis(const char *id);
                void            on_graph_dbl_click(ssize_t x, ssize_t y);
                void            on_filter_dot_right_click(tk::GraphDot *dot, ssize_t x, ssize_t y);
                void            on_filter_menu_item_submit(tk::MenuItem *mi);

                void            set_filter_mode(size_t id, size_t mask, size_t value);
                void            set_filter_type(size_t id, size_t mask, size_t value);
                void            set_filter_frequency(size_t id, size_t mask, float value);
                void            set_filter_quality(size_t id, size_t mask, float value);
                void            set_filter_gain(size_t id, size_t mask, float value);
                void            set_filter_slope(size_t id, size_t mask, size_t slope);
                void            set_filter_enabled(size_t id, size_t mask, bool enabled);
                void            set_filter_solo(size_t id, size_t mask, bool solo);

                ui::IPort      *find_port(const char *fmt, const char *base, size_t id);
                tk::GraphDot   *find_dot(const char *fmt, const char *base, size_t id);

                ssize_t         get_filter_type(size_t id, size_t channel);

                dot_t          *find_dot(tk::GraphDot *dot);
                void            add_dots();
                void            create_dot_menu();
                tk::Menu       *create_submenu(tk::Menu *parent, const char *lc_key,
                    lltl::parray<tk::MenuItem> *items, const meta::port_t *port);
                void            set_menu_items_checked(lltl::parray<tk::MenuItem> *list, ui::IPort *port);
                void            on_menu_item_selected(lltl::parray<tk::MenuItem> *list, ui::IPort *port, tk::MenuItem *mi);

            public:
                explicit para_equalizer_ui(const meta::plugin_t *meta);
                virtual ~para_equalizer_ui();

                virtual status_t    post_init() override;
        };
    } // namespace plugins
} // namespace lsp

#endif /* PRIVATE_UI_PARA_EQUALIZER_H_ */
