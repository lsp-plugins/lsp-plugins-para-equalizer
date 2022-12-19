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
                ui::IPort          *pRewPath;
                tk::FileDialog     *pRewImport;
                tk::Graph          *pGraph;
                const char        **fmtStrings;
                ssize_t             nXAxisIndex;
                ssize_t             nYAxisIndex;
                ssize_t             nSplitChannels;

            protected:
                static status_t slot_start_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_call_import_rew_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_fetch_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_commit_rew_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_graph_dbl_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                status_t    import_rew_file(const LSPString *path);
                void        set_port_value(const char *base, size_t id, size_t mask, float value);
                ssize_t     find_axis(const char *id);
                void        on_graph_dbl_click(ssize_t x, ssize_t y);

                void        set_filter_mode(size_t id, size_t mask, size_t value);
                void        set_filter_type(size_t id, size_t mask, size_t value);
                void        set_filter_frequency(size_t id, size_t mask, float value);
                void        set_filter_quality(size_t id, size_t mask, float value);
                void        set_filter_gain(size_t id, size_t mask, float value);
                void        set_filter_slope(size_t id, size_t mask, size_t slope);
                void        set_filter_enabled(size_t id, size_t mask, bool enabled);
                void        set_filter_solo(size_t id, size_t mask, bool solo);

                ssize_t     get_filter_type(size_t id, size_t channel);

            public:
                explicit para_equalizer_ui(const meta::plugin_t *meta);
                virtual ~para_equalizer_ui();

                virtual status_t    post_init() override;
        };
    } // namespace plugins
} // namespace lsp

#endif /* PRIVATE_UI_PARA_EQUALIZER_H_ */
