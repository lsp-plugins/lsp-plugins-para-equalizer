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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/fmt/RoomEQWizard.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/locale.h>
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
            &meta::para_equalizer_x8_mono,
            &meta::para_equalizer_x8_stereo,
            &meta::para_equalizer_x8_lr,
            &meta::para_equalizer_x8_ms,
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

        static ui::Factory factory(ui_factory, plugin_uis, 12);

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

        static const char *note_names[] =
        {
            "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"
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
            pRewFileType    = NULL;
            pInspect        = NULL;
            pAutoInspect    = NULL;
            pSelector       = NULL;
            pQuantize       = NULL;
            pRewImport      = NULL;
            wGraph          = NULL;
            wInspectReset   = NULL;
            fmtStrings      = fmt_strings;
            nSplitChannels  = 1;
            nXAxisIndex     = -1;
            nYAxisIndex     = -1;

            pCurrDot        = NULL;
            pCurrNote       = NULL;
            wFilterMenu     = NULL;
            wFilterInspect  = NULL;
            wFilterSolo     = NULL;
            wFilterMute     = NULL;
            wFilterSwitch   = NULL;

            if ((!strcmp(meta->uid, meta::para_equalizer_x8_lr.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x16_lr.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x32_lr.uid)))
            {
                fmtStrings      = fmt_strings_lr;
                nSplitChannels  = 2;
            }
            else if (
                    (!strcmp(meta->uid, meta::para_equalizer_x8_ms.uid)) ||
                    (!strcmp(meta->uid, meta::para_equalizer_x16_ms.uid)) ||
                 (!strcmp(meta->uid, meta::para_equalizer_x32_ms.uid)))
            {
                fmtStrings      = fmt_strings_ms;
                nSplitChannels  = 2;
            }

            nFilters        = 8;
            if ((!strcmp(meta->uid, meta::para_equalizer_x16_lr.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x16_mono.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x16_ms.uid)) ||
                (!strcmp(meta->uid, meta::para_equalizer_x16_stereo.uid)))
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
            if (_this == NULL)
                return STATUS_BAD_STATE;

            if (_this->pRewPath != NULL)
                _this->pRewImport->path()->set_raw(_this->pRewPath->buffer<char>());
            if (_this->pRewFileType != NULL)
            {
                size_t filter = _this->pRewFileType->value();
                _this->pRewImport->selected_filter()->set(filter);
            }

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_commit_rew_path(tk::Widget *sender, void *ptr, void *data)
        {
            para_equalizer_ui *self = static_cast<para_equalizer_ui *>(ptr);
            if (self == NULL)
                return STATUS_BAD_STATE;

            if (self->pRewPath != NULL)
                self->pRewPath->begin_edit();
            if (self->pRewFileType != NULL)
                self->pRewFileType->begin_edit();

            if (self->pRewPath != NULL)
            {
                LSPString path;
                if (self->pRewImport->path()->format(&path) == STATUS_OK)
                {
                    const char *u8path = path.get_utf8();

                    self->pRewPath->write(u8path, ::strlen(u8path));
                    self->pRewPath->notify_all(ui::PORT_USER_EDIT);
                }
            }
            if (self->pRewFileType != NULL)
            {
                self->pRewFileType->set_value(self->pRewImport->selected_filter()->get());
                self->pRewFileType->notify_all(ui::PORT_USER_EDIT);
            }

            if (self->pRewPath != NULL)
                self->pRewPath->end_edit();
            if (self->pRewFileType != NULL)
                self->pRewFileType->end_edit();

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

        status_t para_equalizer_ui::slot_filter_edit_timer(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
        {
            // Fetch paramters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(arg);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            // Process the event
            _this->on_filter_edit_timer();

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
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            // Process the event
            _this->on_filter_inspect_submit(sender);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_filter_begin_edit(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            _this->on_begin_filter_edit(sender);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_filter_change(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            _this->on_filter_change(sender);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_filter_end_edit(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            _this->on_end_filter_edit(sender);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_filter_mouse_in(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            filter_t *f = static_cast<filter_t *>(ptr);
            if ((f == NULL) || (f->pUI == NULL))
                return STATUS_BAD_STATE;

            f->pUI->on_filter_mouse_in(f);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_filter_mouse_out(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            filter_t *f = static_cast<filter_t *>(ptr);
            if ((f == NULL) || (f->pUI == NULL))
                return STATUS_BAD_STATE;

            f->pUI->on_filter_mouse_out();

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_main_grid_realized(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            _this->on_main_grid_realized(sender);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_main_grid_mouse_in(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;
            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_BAD_STATE;

            _this->on_main_grid_mouse_in(sender, ev->nLeft, ev->nTop);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_main_grid_mouse_out(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;
            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_BAD_STATE;

            _this->on_main_grid_mouse_out(sender, ev->nLeft, ev->nTop);

            return STATUS_OK;
        }

        status_t para_equalizer_ui::slot_main_grid_mouse_move(tk::Widget *sender, void *ptr, void *data)
        {
            // Fetch parameters
            para_equalizer_ui *_this = static_cast<para_equalizer_ui *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;
            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_BAD_STATE;

            _this->on_main_grid_mouse_move(sender, ev->nLeft, ev->nTop);

            return STATUS_OK;
        }

        para_equalizer_ui::filter_t *para_equalizer_ui::find_filter_by_widget(tk::Widget *widget)
        {
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *d = vFilters.uget(i);
                if ((d->wDot == widget) ||
                    (d->wNote == widget) ||
                    (d->wInspect == widget) ||
                    (d->wSolo == widget) ||
                    (d->wMute == widget) ||
                    (d->wType == widget) ||
                    (d->wMode == widget) ||
                    (d->wSlope == widget) ||
                    (d->wGain == widget) ||
                    (d->wFreq == widget) ||
                    (d->wQuality == widget))
                    return d;
            }
            return NULL;
        }

        para_equalizer_ui::filter_t *para_equalizer_ui::find_filter_by_rect(tk::Widget *grid, ssize_t x, ssize_t y)
        {
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *d = vFilters.uget(i);
                if (d->wGrid != grid)
                    continue;
                if (tk::Position::inside(&d->sRect, x, y))
                    return d;
            }
            return NULL;
        }

        void para_equalizer_ui::on_begin_filter_edit(tk::Widget *w)
        {
            if (pInspect == NULL)
                return;

            // Cancel the previously launched timer
            pCurrDot = NULL;
            sEditTimer.cancel();

            // Find the filter
            filter_t *f = find_filter_by_widget(w);
            if (f == NULL)
                return;

            // Check the auto-inspect mode
            if (pAutoInspect->value() < 0.5f)
                return;

            // Remember the selected filter and launch the timer
            pCurrDot    = f;
            sEditTimer.launch(1, 0, 200);
        }

        void para_equalizer_ui::on_filter_edit_timer()
        {
            if ((pInspect == NULL) || (pCurrDot == NULL))
                return;
            select_inspected_filter(pCurrDot, true);
        }

        void para_equalizer_ui::on_filter_change(tk::Widget *w)
        {
            if ((pCurrDot == NULL) || (pInspect == NULL))
                return;

            // Cancel the previously launched timer
            sEditTimer.cancel();

            // Check the auto-inspect mode
            if (pAutoInspect->value() < 0.5f)
                return;
            select_inspected_filter(pCurrDot, true);
        }

        void para_equalizer_ui::on_end_filter_edit(tk::Widget *w)
        {
            // Cancel the timer
            sEditTimer.cancel();

            // Exit if there is nothing to edit
            if (pCurrDot == NULL)
                return;
            lsp_finally { pCurrDot = NULL; };

            // Clear the inspected filter
            select_inspected_filter(NULL, true);
        }

        void para_equalizer_ui::on_filter_mouse_in(filter_t *f)
        {
            pCurrNote   = (f->pMute->value() >= 0.5) ? NULL : f;
            f->bMouseIn = true;
            update_filter_note_text();
        }

        void para_equalizer_ui::on_filter_mouse_out()
        {
            pCurrNote = NULL;
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *f = vFilters.uget(i);
                if (f != NULL)
                    f->bMouseIn = false;
            }
            update_filter_note_text();
        }

        bool para_equalizer_ui::quantize_note(note_t *out, float freq)
        {
            // Process filter note
            const float note_full = dspu::frequency_to_note(freq);
            if (note_full == dspu::NOTE_OUT_OF_RANGE)
                return false;

            const float center  = note_full + 0.5f;
            out->nNumber        = int32_t(center);
            out->nCents         = (center - float(out->nNumber)) * 100 - 50;

            return true;
        }

        float para_equalizer_ui::quantize_frequency(float value, void *context)
        {
            para_equalizer_ui *self = static_cast<para_equalizer_ui *>(context);
            if (self == NULL)
                return value;
            const bool quantize     = (self->pQuantize != NULL) ? self->pQuantize->value() > 0.5f : false;
            if (!quantize)
                return value;

            note_t note;
            if (!quantize_note(&note, value))
                return value;

            return dspu::midi_note_to_frequency(note.nNumber);
        }

        void para_equalizer_ui::update_filter_note_text()
        {
            // Determine which frequency/note to show: of inspected filter or of selected filter
            ssize_t inspect = (pInspect != NULL) ? ssize_t(pInspect->value()) : -1;
            filter_t *f = (inspect >= 0) ? vFilters.uget(inspect) : NULL;
            if (f == NULL)
                f = pCurrNote;

            // Commit current filter pointer and update note text
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *xf = vFilters.uget(i);
                if (xf != NULL)
                    xf->wNote->visibility()->set(xf == f);
            }

            // Check that we have the widget to display
            if ((f == NULL) || (f->wNote == NULL))
                return;

            // Get the frequency
            float freq = (f->pFreq != NULL) ? f->pFreq->value() : -1.0f;
            if (freq < 0.0f)
            {
                f->wNote->visibility()->set(false);
                return;
            }

            // Get the gain
            float gain = (f->pGain != NULL) ? f->pGain->value() : -1.0f;
            if (gain < 0.0f)
            {
                f->wNote->visibility()->set(false);
                return;
            }

            // Check that filter is enabled
            ssize_t type = (f->pType != NULL) ? ssize_t(f->pType->value()) : meta::para_equalizer_metadata::EQF_OFF;
            if (type == meta::para_equalizer_metadata::EQF_OFF)
            {
                f->wNote->visibility()->set(false);
                return;
            }
            size_t filter_index = vFilters.index_of(f);

            // Update the note name displayed in the text
            {
                // Fill the parameters
                expr::Parameters params;
                tk::prop::String lc_string;
                LSPString text;
                lc_string.bind(f->wNote->style(), pDisplay->dictionary());
                SET_LOCALE_SCOPED(LC_NUMERIC, "C");

                // Frequency and gain
                params.set_float("frequency", freq);
                params.set_float("gain", dspu::gain_to_db(gain));

                // Filter number and audio channel
                text.set_ascii(f->pType->id());
                if (text.starts_with_ascii("ftm_"))
                    lc_string.set("lists.filters.index.mid_id");
                else if (text.starts_with_ascii("fts_"))
                    lc_string.set("lists.filters.index.side_id");
                else if (text.starts_with_ascii("ftl_"))
                    lc_string.set("lists.filters.index.left_id");
                else if (text.starts_with_ascii("ftr_"))
                    lc_string.set("lists.filters.index.right_id");
                else
                    lc_string.set("lists.filters.index.filter_id");
                lc_string.params()->set_int("id", ((filter_index % nFilters)+1) );
                lc_string.format(&text);
                params.set_string("filter", &text);
                lc_string.params()->clear();

                // Process filter type
                text.fmt_ascii("lists.%s", f->pType->metadata()->items[type].lc_key);
                lc_string.set(&text);
                lc_string.format(&text);
                params.set_string("filter_type", &text);

                // Process filter note
                note_t note;
                if (quantize_note(&note, freq))
                {
                    // Note name
                    text.fmt_ascii("lists.notes.names.%s", note_names[note.nNumber % 12]);
                    lc_string.set(&text);
                    lc_string.format(&text);
                    params.set_string("note", &text);

                    // Octave number
                    params.set_int("octave", ssize_t(note.nNumber / 12) - 1);

                    // Cents
                    if (note.nCents < 0)
                        text.fmt_ascii(" - %02d", int(-note.nCents));
                    else
                        text.fmt_ascii(" + %02d", int(note.nCents));
                    params.set_string("cents", &text);

                    f->wNote->text()->set("lists.para_eq.display.full", &params);
                }
                else
                    f->wNote->text()->set("lists.para_eq.display.unknown", &params);
            }
        }

        void para_equalizer_ui::bind_filter_edit(tk::Widget *w)
        {
            if (w == NULL)
                return;
            w->slots()->bind(tk::SLOT_BEGIN_EDIT, slot_filter_begin_edit, this);
            w->slots()->bind(tk::SLOT_CHANGE, slot_filter_change, this);
            w->slots()->bind(tk::SLOT_SUBMIT, slot_filter_change, this);
            w->slots()->bind(tk::SLOT_END_EDIT, slot_filter_end_edit, this);
        }

        tk::Widget *para_equalizer_ui::find_filter_grid(filter_t *f)
        {
            tk::Widget *list[] =
            {
                f->wNote,
                f->wInspect,
                f->wSolo,
                f->wMute,
                f->wType,
                f->wSlope,
                f->wGain,
                f->wFreq,
                f->wQuality
            };

            for (size_t i=0, n=vFilterGrids.size(); i<n; ++i)
            {
                tk::Widget *g = vFilterGrids.uget(i);

                for (size_t j=0, m=sizeof(list)/sizeof(list[0]); j<m; ++j)
                {
                    tk::Widget *w = list[j];
                    if ((w != NULL) && (w->has_parent(g)))
                        return g;
                }
            }

            return NULL;
        }

        void para_equalizer_ui::add_filters()
        {
            for (const char **fmt = fmtStrings; *fmt != NULL; ++fmt)
            {
                for (size_t port_id=0; port_id<nFilters; ++port_id)
                {
                    filter_t f;

                    f.pUI           = this;

                    f.sRect.nLeft   = 0;
                    f.sRect.nTop    = 0;
                    f.sRect.nWidth  = 0;
                    f.sRect.nHeight = 0;

                    f.bMouseIn      = false;

                    f.wDot          = find_filter_widget<tk::GraphDot>(*fmt, "filter_dot", port_id);
                    f.wNote         = find_filter_widget<tk::GraphText>(*fmt, "filter_note", port_id);
                    f.wInspect      = find_filter_widget<tk::Button>(*fmt, "filter_inspect", port_id);
                    f.wSolo         = find_filter_widget<tk::Button>(*fmt, "filter_solo", port_id);
                    f.wMute         = find_filter_widget<tk::Button>(*fmt, "filter_mute", port_id);
                    f.wType         = find_filter_widget<tk::ComboBox>(*fmt, "filter_type", port_id);
                    f.wMode         = find_filter_widget<tk::ComboBox>(*fmt, "filter_mode", port_id);
                    f.wSlope        = find_filter_widget<tk::ComboBox>(*fmt, "filter_slope", port_id);
                    f.wGain         = find_filter_widget<tk::Knob>(*fmt, "filter_gain", port_id);
                    f.wFreq         = find_filter_widget<tk::Knob>(*fmt, "filter_freq", port_id);
                    f.wQuality      = find_filter_widget<tk::Knob>(*fmt, "filter_q", port_id);
                    f.wGrid         = find_filter_grid(&f);

                    f.pType         = find_port(*fmt, "ft", port_id);
                    f.pMode         = find_port(*fmt, "fm", port_id);
                    f.pSlope        = find_port(*fmt, "s", port_id);
                    f.pFreq         = find_port(*fmt, "f", port_id);
                    f.pSolo         = find_port(*fmt, "xs", port_id);
                    f.pMute         = find_port(*fmt, "xm", port_id);
                    f.pGain         = find_port(*fmt, "g", port_id);
                    f.pQuality      = find_port(*fmt, "q", port_id);

                    if (f.wDot != NULL)
                    {
                        f.wDot->slots()->bind(tk::SLOT_MOUSE_CLICK, slot_filter_dot_click, this);
                        f.wDot->hvalue()->set_transform(quantize_frequency, this);
                    }
                    if (f.wInspect != NULL)
                        f.wInspect->slots()->bind(tk::SLOT_SUBMIT, slot_filter_inspect_submit, this);

                    bind_filter_edit(f.wDot);
                    bind_filter_edit(f.wInspect);
                    bind_filter_edit(f.wSolo);
                    bind_filter_edit(f.wMute);
                    bind_filter_edit(f.wType);
                    bind_filter_edit(f.wMode);
                    bind_filter_edit(f.wSlope);
                    bind_filter_edit(f.wGain);
                    bind_filter_edit(f.wFreq);
                    bind_filter_edit(f.wQuality);

                    if (f.pType != NULL)
                        f.pType->bind(this);
                    if (f.pFreq != NULL)
                        f.pFreq->bind(this);
                    if (f.pSolo != NULL)
                        f.pSolo->bind(this);
                    if (f.pMute != NULL)
                        f.pMute->bind(this);

                    vFilters.add(&f);
                }
            }

            // Bind events
            size_t index = 0;
            for (const char **fmt = fmtStrings; *fmt != NULL; ++fmt)
            {
                for (size_t port_id=0; port_id<nFilters; ++port_id)
                {
                    filter_t *f = vFilters.uget(index++);
                    if (f == NULL)
                        return;

                    if (f->wDot != NULL)
                    {
                        f->wDot->slots()->bind(tk::SLOT_MOUSE_IN, slot_filter_mouse_in, f);
                        f->wDot->slots()->bind(tk::SLOT_MOUSE_OUT, slot_filter_mouse_out, f);
                    }

                    // Get all filter-related widgets
                    LSPString grp_name;
                    grp_name.fmt_utf8(*fmt, "grp_filter", int(port_id));
                    lltl::parray<tk::Widget> all_widgets;
                    pWrapper->controller()->widgets()->query_group(&grp_name, &all_widgets);
                    for (size_t i=0, n=all_widgets.size(); i<n; ++i)
                    {
                        tk::Widget *w = all_widgets.uget(i);
                        if (w != NULL)
                        {
                            w->slots()->bind(tk::SLOT_MOUSE_IN, slot_filter_mouse_in, f);
                            w->slots()->bind(tk::SLOT_MOUSE_OUT, slot_filter_mouse_out, f);
                        }
                    }
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

            if ((wFilterSolo = create_menu_item(root, "labels.chan.solo")) == NULL)
                return;
            wFilterSolo->type()->set_check();
            wFilterSolo->slots()->bind(tk::SLOT_SUBMIT, slot_filter_menu_submit, this);

            if ((wFilterMute = create_menu_item(root, "labels.chan.mute")) == NULL)
                return;

            wFilterMute->type()->set_check();
            wFilterMute->slots()->bind(tk::SLOT_SUBMIT, slot_filter_menu_submit, this);

            if ((wFilterSwitch = create_menu_item(root, "")) == NULL)
                return;

            wFilterSwitch->slots()->bind(tk::SLOT_SUBMIT, slot_filter_menu_submit, this);

            wFilterMenu    = root;
        }

        status_t para_equalizer_ui::post_init()
        {
            status_t res = ui::Module::post_init();
            if (res != STATUS_OK)
                return res;

            // Find main filter grids
            pWrapper->controller()->widgets()->query_group("filters", &vFilterGrids);
            for (size_t i=0, n=vFilterGrids.size(); i<n; ++i)
            {
                tk::Widget *g = vFilterGrids.uget(i);
                g->slots()->bind(tk::SLOT_REALIZED, slot_main_grid_realized, this);
                g->slots()->bind(tk::SLOT_MOUSE_IN, slot_main_grid_mouse_in, this);
                g->slots()->bind(tk::SLOT_MOUSE_OUT, slot_main_grid_mouse_out, this);
                g->slots()->bind(tk::SLOT_MOUSE_MOVE, slot_main_grid_mouse_move, this);
            }

            // Add filter widgets
            add_filters();
            if (!vFilters.is_empty())
                create_filter_menu();

            // Find REW port
            pRewPath            = pWrapper->port(REW_PATH_PORT);
            pRewFileType        = pWrapper->port(REW_FTYPE_PORT);
            pInspect            = pWrapper->port("insp_id");
            if (pInspect != NULL)
                pInspect->bind(this);
            pAutoInspect        = pWrapper->port("insp_on");
            if (pAutoInspect != NULL)
                pAutoInspect->bind(this);
            pSelector           = pWrapper->port("fsel");
            pQuantize           = pWrapper->port("quant");

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
                wInspectReset->slots()->bind(tk::SLOT_SUBMIT, slot_filter_inspect_submit, this);

            // Bind the timer
            sEditTimer.bind(pDisplay);
            sEditTimer.set_handler(slot_filter_edit_timer, this);

            // Update state
            sync_filter_inspect_state();

            return STATUS_OK;
        }

        status_t para_equalizer_ui::pre_destroy()
        {
            // Cancel the edit timer
            sEditTimer.cancel();

            // Force the inspect mode to be turned off
            if (pInspect != NULL)
            {
                pInspect->begin_edit();
                pInspect->set_value(-1.0f);
                pInspect->notify_all(ui::PORT_USER_EDIT);
                pInspect->end_edit();
            }

            return ui::Module::pre_destroy();
        }

        void para_equalizer_ui::set_menu_items_checked(lltl::parray<tk::MenuItem> *list, ui::IPort *port)
        {
            if (port == NULL)
                return;

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
            if (port == NULL)
                return;

            ssize_t index = list->index_of(mi);
            if (index < 0)
                return;

            const meta::port_t *p = port->metadata();
            float min = 0.0f, max = 1.0f, step = 1.0f;
            meta::get_port_parameters(p, &min, &max, &step);

            port->set_value(min + index * step);
            port->notify_all(ui::PORT_USER_EDIT);
        }

        void para_equalizer_ui::transfer_port_value(ui::IPort *dst, ui::IPort *src)
        {
            if ((src == NULL) || (dst == NULL))
                return;

            src->begin_edit();
            dst->begin_edit();

            dst->set_value(src->value());
            src->set_default();

            dst->notify_all(ui::PORT_USER_EDIT);
            src->notify_all(ui::PORT_USER_EDIT);

            src->end_edit();
            dst->end_edit();
        }

        void para_equalizer_ui::on_filter_menu_item_submit(tk::MenuItem *mi)
        {
            if (pCurrDot == NULL)
                return;
            lsp_finally { pCurrDot = NULL; };

            if (pCurrDot->pType != NULL)
                pCurrDot->pType->begin_edit();
            if (pCurrDot->pMode != NULL)
                pCurrDot->pMode->begin_edit();
            if (pCurrDot->pSlope != NULL)
                pCurrDot->pSlope->begin_edit();
            if (pCurrDot->pMute != NULL)
                pCurrDot->pMute->begin_edit();
            if (pCurrDot->pSolo != NULL)
                pCurrDot->pSolo->begin_edit();
            if (pSelector != NULL)
                pCurrDot->pSolo->begin_edit();

            on_filter_menu_item_selected(&vFilterTypes, pCurrDot->pType, mi);
            on_filter_menu_item_selected(&vFilterModes, pCurrDot->pMode, mi);
            on_filter_menu_item_selected(&vFilterSlopes, pCurrDot->pSlope, mi);
            if ((mi == wFilterMute) && (pCurrDot->pMute != NULL))
            {
                pCurrDot->pMute->set_value((mi->checked()->get()) ? 0.0f : 1.0f);
                pCurrDot->pMute->notify_all(ui::PORT_USER_EDIT);
            }
            if ((mi == wFilterSolo) && (pCurrDot->pSolo != NULL))
            {
                pCurrDot->pSolo->set_value((mi->checked()->get()) ? 0.0f : 1.0f);
                pCurrDot->pSolo->notify_all(ui::PORT_USER_EDIT);
            }
            if (mi == wFilterSwitch)
            {
                filter_t *alt_f     = find_switchable_filter(pCurrDot);

                // Set-up alternate filter and disable current filter
                transfer_port_value(alt_f->pMode, pCurrDot->pMode);
                transfer_port_value(alt_f->pSlope, pCurrDot->pSlope);
                transfer_port_value(alt_f->pFreq, pCurrDot->pFreq);
                transfer_port_value(alt_f->pSolo, pCurrDot->pSolo);
                transfer_port_value(alt_f->pMute, pCurrDot->pMute);
                transfer_port_value(alt_f->pQuality, pCurrDot->pQuality);
                transfer_port_value(alt_f->pGain, pCurrDot->pGain);
                transfer_port_value(alt_f->pType, pCurrDot->pType);

                // Apply selector to filter
                ssize_t filter_index = vFilters.index_of(alt_f);
                if ((filter_index >= 0) && (pSelector != NULL))
                {
                    size_t group    = filter_index / nFilters;
                    size_t subgroup = (filter_index % nFilters) / 8;
                    pSelector->set_value(subgroup * 2 + group);
                    pSelector->notify_all(ui::PORT_USER_EDIT);
                }

                // Update current filter
                pCurrDot            = alt_f;
            }
            if (mi == wFilterInspect)
                toggle_inspected_filter(pCurrDot, true);

            if (pCurrDot->pType != NULL)
                pCurrDot->pType->end_edit();
            if (pCurrDot->pMode != NULL)
                pCurrDot->pMode->end_edit();
            if (pCurrDot->pSlope != NULL)
                pCurrDot->pSlope->end_edit();
            if (pCurrDot->pMute != NULL)
                pCurrDot->pMute->end_edit();
            if (pCurrDot->pSolo != NULL)
                pCurrDot->pSolo->end_edit();
            if (pSelector != NULL)
                pCurrDot->pSolo->end_edit();
        }

        para_equalizer_ui::filter_t *para_equalizer_ui::find_switchable_filter(filter_t *filter)
        {
            // We can switch filter between Left/Right and Left/Side only when it is possible
            if (nSplitChannels < 2)
                return NULL;

            ssize_t filter_index = vFilters.index_of(filter);
            if (filter_index < 0)
                return NULL;

            size_t offset       = filter_index % nFilters;
            size_t begin        = ((filter_index / nFilters) > 0) ? 0 : nFilters;

            // Seeking an alternate filter in the whole alternate filter group (using round-robin)
            for (size_t i=0; i<nFilters; ++i)
            {
                size_t index    = begin + (offset + i) % nFilters;
                filter_t *alt_f = vFilters.uget(index);
                if ((alt_f == NULL) || (alt_f->pType == NULL))
                    continue;

                if (ssize_t(alt_f->pType->value()) == meta::para_equalizer_metadata::EQF_OFF)
                    return alt_f;
            }

            return NULL;
        }

        void para_equalizer_ui::on_filter_dot_right_click(tk::Widget *dot, ssize_t x, ssize_t y)
        {
            // Is filter dot menu present?
            if (wFilterMenu == NULL)
                return;

            // Lookup for the dot
            if ((pCurrDot = find_filter_by_widget(dot)) == NULL)
                return;
            if (pCurrDot->wDot == NULL)
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

            if (find_switchable_filter(pCurrDot) != NULL)
            {
                LSPString id_type;
                id_type.set_ascii(pCurrDot->pType->id());

                wFilterSwitch->visibility()->set(true);
                if (id_type.starts_with_ascii("ftm_"))
                    wFilterSwitch->text()->set_key("actions.filters.switch.to_side");
                else if (id_type.starts_with_ascii("fts_"))
                    wFilterSwitch->text()->set_key("actions.filters.switch.to_mid");
                else if (id_type.starts_with_ascii("ftl_"))
                    wFilterSwitch->text()->set_key("actions.filters.switch.to_right");
                else if (id_type.starts_with_ascii("ftr_"))
                    wFilterSwitch->text()->set_key("actions.filters.switch.to_left");
                else
                    wFilterSwitch->visibility()->set(false);
            }
            else
                wFilterSwitch->visibility()->set(false);

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
            ssize_t channel     = (pSelector != NULL) ? size_t(pSelector->value()) % nSplitChannels : 0;
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
                (freq <= 20.0f)    ? meta::para_equalizer_metadata::EQF_HIPASS     :
                (freq <= 50.0f)    ? meta::para_equalizer_metadata::EQF_LOSHELF    :
                (freq <= 5000.0f)   ? meta::para_equalizer_metadata::EQF_BELL       :
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
                if ((xf->pSolo != NULL) && (xf->pSolo->value() >= 0.5f))
                {
                    has_solo        = true;
                    break;
                }
            }

            bool mute       = (f->pMute != NULL) ? f->pMute->value() >= 0.5f : false;
            bool solo       = (f->pSolo != NULL) ? f->pSolo->value() >= 0.5f : false;
            if ((mute) || ((has_solo) && (!solo)))
                return false;

            // The filter should be enabled
            size_t type     = (f->pType != NULL) ? size_t(f->pType->value()) : meta::para_equalizer_metadata::EQF_OFF;
            return type != meta::para_equalizer_metadata::EQF_OFF;
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
            bool auto_inspect = (pAutoInspect != NULL) && (pAutoInspect->value() >= 0.5f);

            // Set the down state of inspect button for all filters
            for (size_t i=0, n=vFilters.size(); i<n; ++i)
            {
                filter_t *xf    = vFilters.uget(i);
                if (xf->wInspect != NULL)
                    xf->wInspect->down()->set((f != NULL) ? (xf == f) : false);
            }

            // Update the inspection port
            ssize_t inspect = (pInspect != NULL) ? ssize_t(pInspect->value()) : -1;
            ssize_t index = (f != NULL) ? vFilters.index_of(f) : -1;

            if ((pInspect != NULL) && (commit) && (inspect != index))
            {
                pInspect->begin_edit();
                pInspect->set_value(index);
                pInspect->notify_all(ui::PORT_USER_EDIT);
                pInspect->end_edit();
                inspect     = index;
            }

            if (wInspectReset != NULL)
                wInspectReset->down()->set((!auto_inspect) && (inspect >= 0));
            if ((pCurrDot == f) && (wFilterInspect != NULL))
                wFilterInspect->checked()->set((inspect >= 0) && (inspect == index));

            // Make the frequency and note visible
            update_filter_note_text();
        }

        void para_equalizer_ui::toggle_inspected_filter(filter_t *f, bool commit)
        {
            if (pInspect == NULL)
            {
                select_inspected_filter(NULL, true);
                return;
            }

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
            if ((pAutoInspect != NULL) && (pAutoInspect->value() >= 0.5f))
            {
                select_inspected_filter(NULL, true);
                return;
            }

            // Filter inspect button submitted?
            filter_t *f     = find_filter_by_widget(button);
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
                    p->notify_all(ui::PORT_USER_EDIT);
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

        void para_equalizer_ui::notify(ui::IPort *port, size_t flags)
        {
            if (is_filter_inspect_port(port))
            {
                if ((port == pAutoInspect) && (pAutoInspect->value() >= 0.5f))
                    select_inspected_filter(NULL, true);
                else
                    sync_filter_inspect_state();
            }
            if (pCurrNote != NULL)
            {
                if ((port == pCurrNote->pFreq) || (port == pCurrNote->pType))
                    update_filter_note_text();
            }

            filter_t *f = find_filter_by_mute(port);
            if (f != NULL)
            {
                if (port->value() >= 0.5)
                {
                    if (pCurrNote == f)
                    {
                        pCurrNote = NULL;
                        update_filter_note_text();
                    }
                }
                else
                {
                    if (f->bMouseIn)
                    {
                        pCurrNote = f;
                        update_filter_note_text();
                    }
                }
            }
        }

        bool para_equalizer_ui::is_filter_inspect_port(ui::IPort *port)
        {
            if (pInspect == NULL)
                return false;

            if ((port == pInspect) || (port == pAutoInspect))
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

        para_equalizer_ui::filter_t *para_equalizer_ui::find_filter_by_mute(ui::IPort *port)
        {
            for (size_t i = 0, n = vFilters.size(); i < n; ++i)
            {
                filter_t *f = vFilters.uget(i);
                if (f == NULL)
                    continue;
                if (f->pMute == port)
                    return f;
            }
            return NULL;
        }

        void para_equalizer_ui::on_main_grid_realized(tk::Widget *w)
        {
            // Bind events
            size_t index = 0;
            for (const char **fmt = fmtStrings; *fmt != NULL; ++fmt)
            {
                for (size_t port_id=0; port_id<nFilters; ++port_id)
                {
                    filter_t *f = vFilters.uget(index++);
                    if ((f == NULL) || (f->wGrid != w))
                        continue;

                    // Get all filter-related widgets
                    LSPString grp_name;
                    grp_name.fmt_utf8(*fmt, "grp_filter", int(port_id));
                    lltl::parray<tk::Widget> all_widgets;
                    pWrapper->controller()->widgets()->query_group(&grp_name, &all_widgets);

                    // Estimate the surrounding rectangle size
                    ws::rectangle_t r;
                    ssize_t min_x = 0, max_x = 0;
                    ssize_t min_y = 0, max_y = 0;
                    size_t processed = 0;
                    for (size_t i=0, n=all_widgets.size(); i<n; ++i)
                    {
                        tk::Widget *w = all_widgets.uget(i);
                        if (w != NULL)
                        {
                            w->get_padded_rectangle(&r);
                            if (processed++ > 0)
                            {
                                min_x = lsp_min(min_x, r.nLeft);
                                min_y = lsp_min(min_y, r.nTop);
                                max_x = lsp_max(max_x, r.nLeft + r.nWidth);
                                max_y = lsp_max(max_y, r.nTop + r.nHeight);
                            }
                            else
                            {
                                min_x = r.nLeft;
                                min_y = r.nTop;
                                max_x = r.nLeft + r.nWidth;
                                max_y = r.nTop + r.nHeight;
                            }
                        }
                    }

                    // Update allocation rectangle
                    f->sRect.nLeft      = min_x;
                    f->sRect.nTop       = min_y;
                    f->sRect.nWidth     = max_x - min_x;
                    f->sRect.nHeight    = max_y - min_y;
                }
            }
        }

        void para_equalizer_ui::on_main_grid_mouse_in(tk::Widget *w, ssize_t x, ssize_t y)
        {
            filter_t *f = find_filter_by_rect(w, x, y);
            if (f != NULL)
                on_filter_mouse_in(f);
            else
                on_filter_mouse_out();
        }

        void para_equalizer_ui::on_main_grid_mouse_out(tk::Widget *w, ssize_t x, ssize_t y)
        {
            on_filter_mouse_out();
        }

        void para_equalizer_ui::on_main_grid_mouse_move(tk::Widget *w, ssize_t x, ssize_t y)
        {
            filter_t *f = find_filter_by_rect(w, x, y);
            if (f != NULL)
                on_filter_mouse_in(f);
            else
                on_filter_mouse_out();
        }

    } /* namespace plugins */
} /* namespace lsp */


