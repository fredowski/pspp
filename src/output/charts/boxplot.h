/* PSPP - a program for statistical analysis.
   Copyright (C) 2004, 2009, 2011 Free Software Foundation, Inc.

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

#ifndef OUTPUT_CHARTS_BOXPLOT_H
#define OUTPUT_CHARTS_BOXPLOT_H 1

#include <stddef.h>
#include "output/chart.h"

/* Box-whiskers plot. */
struct boxplot
  {
    struct chart chart;

    /* Data. */
    struct boxplot_box *boxes;
    size_t n_boxes, boxes_allocated;

    /* Derived from data and convenient for plotting. */
    double y_min;               /* Minimum Y coordinate of extremum. */
    double y_max;               /* Maximum Y coordinate of extremum. */
  };

/* One box within a box-whiskers plot. */
struct boxplot_box
  {
    struct box_whisker *bw;
    char *label;                /* Variable name or factor label. */
  };

struct boxplot *boxplot_create (double y_min, double y_max, const char *title);
void boxplot_add_box (struct boxplot *,
                      struct box_whisker *, const char *label);

/* This boilerplate for boxplot, a subclass of chart, was
   autogenerated by mk-class-boilerplate. */

#include <assert.h>
#include "libpspp/cast.h"

extern const struct chart_class boxplot_class;

/* Returns true if SUPER is a boxplot, otherwise false. */
static inline bool
is_boxplot (const struct chart *super)
{
  return super->class == &boxplot_class;
}

/* Returns SUPER converted to boxplot.  SUPER must be a boxplot, as
   reported by is_boxplot. */
static inline struct boxplot *
to_boxplot (const struct chart *super)
{
  assert (is_boxplot (super));
  return UP_CAST (super, struct boxplot, chart);
}

/* Returns INSTANCE converted to chart. */
static inline struct chart *
boxplot_super (const struct boxplot *instance)
{
  return CONST_CAST (struct chart *, &instance->chart);
}

/* Increments INSTANCE's reference count and returns INSTANCE. */
static inline struct boxplot *
boxplot_ref (const struct boxplot *instance)
{
  return to_boxplot (chart_ref (&instance->chart));
}

/* Decrements INSTANCE's reference count, then destroys INSTANCE if
   the reference count is now zero. */
static inline void
boxplot_unref (struct boxplot *instance)
{
  chart_unref (&instance->chart);
}

/* Returns true if INSTANCE's reference count is greater than 1,
   false otherwise. */
static inline bool
boxplot_is_shared (const struct boxplot *instance)
{
  return chart_is_shared (&instance->chart);
}

static inline void
boxplot_submit (struct boxplot *instance)
{
  chart_submit (&instance->chart);
}

#endif /* output/charts/boxplot.h */
