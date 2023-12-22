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
                    para_equalizer_ui  *pUI;
                    ws::rectangle_t     sRect;          // The overall rectangle over the grid

                    bool                bMouseIn;       // Mouse is over filter indicator

                    ui::IPort          *pType;
                    ui::IPort          *pMode;
                    ui::IPort          *pSlope;
                    ui::IPort          *pFreq;
                    ui::IPort          *pSolo;
                    ui::IPort          *pMute;
                    ui::IPort          *pQuality;
                    ui::IPort          *pGain;

                    tk::Widget         *wGrid;          // Grid associated with the filter
                    tk::GraphDot       *wDot;           // Graph dot for editing
                    tk::GraphText      *wNote;          // Text with note and frequency
                    tk::Button         *wInspect;       // Inspect button
                    tk::Button         *wSolo;          // Solo button
                    tk::Button         *wMute;          // Mute button
                    tk::ComboBox       *wType;          // Filter type
                    tk::ComboBox       *wMode;          // Filter mode
                    tk::ComboBox       *wSlope;         // Filter slope
                    tk::Knob           *wGain;          // Gain button
                    tk::Knob           *wFreq;          // Frequency button
                    tk::Knob           *wQuality;       // Quality button
                } filter_t;

            protected:
                ui::IPort          *pRewPath;
                ui::IPort          *pRewFileType;
                ui::IPort          *pInspect;           // Inspected filter index
                ui::IPort          *pAutoInspect;       // Automatically inspect the filter
                ui::IPort          *pSelector;          // Filter selector
                ui::IPort          *pCurrentFilter;     // Current filter
                tk::FileDialog     *pRewImport;
                tk::Graph          *wGraph;
                tk::Button         *wInspectReset;      // Inspect reset button
                tk::Timer           sEditTimer;         // Edit timer
                const char        **fmtStrings;
                ssize_t             nXAxisIndex;
                ssize_t             nYAxisIndex;
                ssize_t             nSplitChannels;
                size_t              nFilters;
                size_t              nCurrentFilter;

                filter_t           *pCurrDot;           // Current filter associated with dot
                filter_t           *pCurrNote;          // Current filter note
                tk::Menu           *wFilterMenu;        // Popup menu for filter properties
                tk::MenuItem       *wFilterInspect;
                tk::MenuItem       *wFilterSolo;
                tk::MenuItem       *wFilterMute;
                tk::MenuItem       *wFilterSwitch;      // Switch filter between Left/Right and Mid/Side
                lltl::parray<tk::MenuItem> vFilterTypes;
                lltl::parray<tk::MenuItem> vFilterModes;
                lltl::parray<tk::MenuItem> vFilterSlopes;
                lltl::darray<filter_t> vFilters;
                lltl::parray<tk::Widget> vFilterGrids;   // List of filter grids

            protected:
                static status_t slot_start_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_call_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_fetch_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_commit_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_graph_dbl_click(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_filter_menu_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_dot_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_dot_mouse_down(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_dot_mouse_scroll(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_dot_mouse_dbl_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_inspect_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_begin_edit(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_change(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_end_edit(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_edit_timer(ws::timestamp_t sched, ws::timestamp_t time, void *arg);
                static status_t slot_filter_mouse_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_filter_mouse_out(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_main_grid_realized(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_main_grid_mouse_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_main_grid_mouse_out(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_main_grid_mouse_move(tk::Widget *sender, void *ptr, void *data);

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

                void            on_filter_dot_right_click(tk::Widget *sender, ssize_t x, ssize_t y);
                void            on_filter_dot_mouse_down(tk::Widget *sender, ssize_t x, ssize_t y);
                void            on_filter_dot_mouse_scroll(tk::Widget *sender, const ws::event_t *ev);
                void            on_filter_menu_item_submit(tk::MenuItem *mi);
                void            on_filter_menu_item_selected(lltl::parray<tk::MenuItem> *list, ui::IPort *port, tk::MenuItem *mi);
                void            on_filter_inspect_submit(tk::Widget *button);
                void            on_begin_filter_edit(tk::Widget *w);
                void            on_filter_change(tk::Widget *w);
                void            on_filter_edit_timer();
                void            on_end_filter_edit(tk::Widget *w);
                void            on_filter_mouse_in(filter_t *f);
                void            on_filter_mouse_out();
                void            on_filter_mouse_dbl_click(tk::Widget *sender);

                void            on_main_grid_realized(tk::Widget *w);
                void            on_main_grid_mouse_in(tk::Widget *w, ssize_t x, ssize_t y);
                void            on_main_grid_mouse_out(tk::Widget *w, ssize_t x, ssize_t y);
                void            on_main_grid_mouse_move(tk::Widget *w, ssize_t x, ssize_t y);

                void            set_current_filter(size_t id);
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

                filter_t       *find_filter_by_widget(tk::Widget *widget);
                filter_t       *find_filter_by_rect(tk::Widget *grid, ssize_t x, ssize_t y);
                filter_t       *find_filter_by_mute(ui::IPort *port);
                tk::Widget     *find_filter_grid(filter_t *f);
                void            add_filters();
                void            create_filter_menu();
                void            select_inspected_filter(filter_t *f, bool commit);
                void            toggle_inspected_filter(filter_t *f, bool commit);
                bool            filter_inspect_can_be_enabled(filter_t *f);
                void            sync_filter_inspect_state();
                bool            is_filter_inspect_port(ui::IPort *port);
                void            bind_filter_edit(tk::Widget *w);
                void            update_filter_note_text();
                filter_t       *find_switchable_filter(filter_t *filter);
                void            transfer_port_value(ui::IPort *dst, ui::IPort *src);

            public:
                explicit para_equalizer_ui(const meta::plugin_t *meta);
                virtual ~para_equalizer_ui() override;

                virtual status_t    init(ui::IWrapper *wrapper, tk::Display *dpy) override;

                virtual status_t    post_init() override;
                virtual status_t    pre_destroy() override;

                virtual void        notify(ui::IPort *port, size_t flags) override;
        };
    } // namespace plugins
} // namespace lsp

#endif /* PRIVATE_UI_PARA_EQUALIZER_H_ */
