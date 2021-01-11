/* PSPP - a program for statistical analysis.
   Copyright (C) 2009, 2011 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef OUTPUT_ITEM_PROVIDER_H
#define OUTPUT_ITEM_PROVIDER_H 1

#include "libpspp/str.h"
#include "output/output-item.h"

/* Class structure for an output item.

   This structure must be provided by an output_item subclass to initialize an
   instance of output_item. */
struct output_item_class
  {
    /* Returns the localized label to use for ITEM.  This is only called when
       ITEM does not have an explicitly set label, that is, when 'item->label'
       is NULL. */
    const char *(*get_label) (const struct output_item *item);

    /* Destroys and frees ITEM.  Called when output_item_unref() drops ITEM's
       reference count to 0. */
    void (*destroy) (struct output_item *item);
  };

#define OUTPUT_ITEM_INITIALIZER(CLASS) { .class = CLASS, .ref_cnt = 1 }
#define OUTPUT_ITEM_CLONE_INITIALIZER(SRC)                      \
  {                                                             \
    .class = (SRC)->class,                                      \
    .ref_cnt = 1,                                               \
    .label = xstrdup_if_nonnull ((SRC)->label),                 \
    .command_name = xstrdup_if_nonnull ((SRC)->command_name),   \
  }

#endif /* output/output-item-provider.h */
