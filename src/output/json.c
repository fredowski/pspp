/* PSPP - a program for statistical analysis.
   Copyright (C) 2026 Johannes Weingartner

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

/* JSON output driver.

   Writes one JSON object per output item, one per line (newline-delimited
   JSON), so that a front end can consume the stream while a syntax file is
   still running.  All text is UTF-8.  Numbers are written without regard to
   the locale.  The format is documented in the PSPP manual.

   Item shapes:
     {"type":"header","version":N}   (always the first object)
     {"type":"title","level":N,"text":"..."}
     {"type":"syntax","text":"..."}
     {"type":"log","text":"..."}
     {"type":"message","severity":"error|warning|note","text":"..."}
     {"type":"chart","title":"..."}
     {"type":"image"}
     {"type":"table","label":"...","layer":[...],"title":CELL|null,
      "rows":N,"cols":N,"headRows":[top,bottom],"headCols":[left,right],
      "cells":[CELL...],"caption":CELL|null,"footnotes":[CELL...]}
   with CELL =
     {"r":row,"c":col,"rs":rowspan,"cs":colspan,"h":isHeader,
      "t":"text","n":number?,"a":"l|r|c","b":bold?,"i":italic?,
      "x":engine-generated text?,"fn":[footnote marks]?} */

#include <config.h>

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistr.h>

#include "data/file-name.h"
#include "data/file-handle-def.h"
#include "libpspp/assertion.h"
#include "libpspp/cast.h"
#include "libpspp/message.h"
#include "output/chart.h"
#include "output/driver-provider.h"
#include "output/options.h"
#include "output/output-item.h"
#include "output/pivot-output.h"
#include "output/pivot-table.h"
#include "output/table-provider.h"

#include "gl/c-snprintf.h"
#include "gl/xalloc.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

/* Version of the output format.  Increase it whenever a change would break a
   program written for the previous version. */
#define JSON_FORMAT_VERSION "1"

#define H TABLE_HORZ
#define V TABLE_VERT

struct json_driver
  {
    struct output_driver driver;
    struct file_handle *handle;
    FILE *file;
  };

static const struct output_driver_class json_driver_class;

static struct json_driver *
json_driver_cast (struct output_driver *driver)
{
  assert (driver->class == &json_driver_class);
  return UP_CAST (driver, struct json_driver, driver);
}

/* Writes S as a JSON string literal, including the quotes.  JSON text must be
   valid Unicode, but a few PSPP messages quote raw bytes from the input, so
   any byte that is not part of a well-formed UTF-8 sequence is written as
   U+FFFD (REPLACEMENT CHARACTER). */
static void
put_string (FILE *f, const char *s)
{
  const uint8_t *p = CHAR_CAST (const uint8_t *, s ? s : "");
  const uint8_t *end = p + strlen (CHAR_CAST (const char *, p));

  putc ('"', f);
  while (p < end)
    {
      if (*p < 0x80)
        {
          switch (*p)
            {
            case '"':  fputs ("\\\"", f); break;
            case '\\': fputs ("\\\\", f); break;
            case '\n': fputs ("\\n", f); break;
            case '\r': fputs ("\\r", f); break;
            case '\t': fputs ("\\t", f); break;
            default:
              if (*p < 0x20)
                fprintf (f, "\\u%04x", *p);
              else
                putc (*p, f);
            }
          p++;
        }
      else
        {
          ucs4_t uc;
          int n = u8_mbtouc (&uc, p, end - p);
          if (uc == 0xfffd)
            fputs ("\\ufffd", f);
          else
            fwrite (p, 1, n, f);
          p += n;
        }
    }
  putc ('"', f);
}

static void
put_cell (struct json_driver *json, const struct pivot_table *pt,
          const struct table_cell *cell, int row_offset, bool header)
{
  FILE *f = json->file;

  struct string body = DS_EMPTY_INITIALIZER;
  bool numeric = pivot_value_format_body (cell->value, pt, &body);

  enum table_halign halign = table_halign_interpret (cell->cell_style->halign,
                                                     numeric);
  const char *a = (halign == TABLE_HALIGN_RIGHT ? "r"
                   : halign == TABLE_HALIGN_CENTER ? "c" : "l");

  fprintf (f, "{\"r\":%d,\"c\":%d,\"rs\":%d,\"cs\":%d,\"h\":%s,\"a\":\"%s\"",
           cell->d[V][0] + row_offset, cell->d[H][0],
           table_cell_rowspan (cell), table_cell_colspan (cell),
           header ? "true" : "false", a);

  const char *s = ds_cstr (&body);
  s += strspn (s, CC_SPACES);
  fputs (",\"t\":", f);
  put_string (f, s);

  if (cell->value->type == PIVOT_VALUE_NUMERIC)
    {
      double x = cell->value->numeric.x;
      if (isfinite (x))
        {
          /* Not fprintf, which would use the locale's decimal point. */
          char buf[64];
          c_snprintf (buf, sizeof buf, "%.17g", x);
          fprintf (f, ",\"n\":%s", buf);
        }
    }
  /* "x":true marks text that PSPP itself generated (titles, headings,
     statistic names).  Variable names, labels and data values are user data
     and must not be translated by a front end. */
  if (cell->value->type == PIVOT_VALUE_TEXT
      || cell->value->type == PIVOT_VALUE_TEMPLATE)
    fputs (",\"x\":true", f);
  if (cell->font_style->bold)
    fputs (",\"b\":true", f);
  if (cell->font_style->italic)
    fputs (",\"i\":true", f);

  const struct pivot_value_ex *ex = pivot_value_ex (cell->value);
  if (ex->n_footnotes > 0)
    {
      fputs (",\"fn\":[", f);
      bool first = true;
      for (size_t i = 0; i < ex->n_footnotes; i++)
        {
          const struct pivot_footnote *fnote
            = pt->footnotes[ex->footnote_indexes[i]];
          if (fnote->show)
            {
              char *marker = pivot_footnote_marker_string (fnote, pt);
              if (!first)
                putc (',', f);
              put_string (f, marker);
              free (marker);
              first = false;
            }
        }
      putc (']', f);
    }

  putc ('}', f);
  ds_destroy (&body);
}

/* Writes a single-cell table (title, caption, one footnote line) as a CELL, or
   null if TABLE is null. */
static void
put_single (struct json_driver *json, const struct pivot_table *pt,
            const struct table *table, size_t y)
{
  if (!table)
    {
      fputs ("null", json->file);
      return;
    }
  struct table_cell cell;
  table_get_cell (table, 0, y, &cell);
  put_cell (json, pt, &cell, 0, false);
}

static void
json_output_table_layer (struct json_driver *json, const struct output_item *item,
                         const size_t *layer_indexes)
{
  FILE *f = json->file;
  const struct pivot_table *pt = item->table;

  struct table *title, *layers, *body, *caption, *footnotes;
  pivot_output (pt, layer_indexes, true, &title, &layers, &body,
                &caption, &footnotes, NULL, NULL);

  fputs ("{\"type\":\"table\",\"label\":", f);
  put_string (f, output_item_get_label (item));

  fputs (",\"layer\":", f);
  if (layers)
    {
      putc ('[', f);
      for (size_t y = 0; y < layers->n[V]; y++)
        {
          if (y)
            putc (',', f);
          struct table_cell cell;
          table_get_cell (layers, 0, y, &cell);
          struct string s = DS_EMPTY_INITIALIZER;
          pivot_value_format_body (cell.value, pt, &s);
          put_string (f, ds_cstr (&s));
          ds_destroy (&s);
        }
      putc (']', f);
    }
  else
    fputs ("[]", f);

  fputs (",\"title\":", f);
  put_single (json, pt, title, 0);

  fprintf (f, ",\"rows\":%d,\"cols\":%d,"
           "\"headRows\":[%d,%d],\"headCols\":[%d,%d],\"cells\":[",
           body->n[V], body->n[H],
           body->h[V][0], body->h[V][1], body->h[H][0], body->h[H][1]);

  bool first = true;
  for (int y = 0; y < body->n[V]; y++)
    for (int x = 0; x < body->n[H]; )
      {
        struct table_cell cell;
        table_get_cell (body, x, y, &cell);
        if (x == cell.d[H][0] && y == cell.d[V][0])
          {
            bool is_header = (y < body->h[V][0]
                              || y >= body->n[V] - body->h[V][1]
                              || x < body->h[H][0]
                              || x >= body->n[H] - body->h[H][1]);
            if (!first)
              putc (',', f);
            first = false;
            put_cell (json, pt, &cell, 0, is_header);
          }
        x = cell.d[H][1];
      }
  fputs ("],\"caption\":", f);
  put_single (json, pt, caption, 0);

  fputs (",\"footnotes\":[", f);
  if (footnotes)
    for (size_t y = 0; y < footnotes->n[V]; y++)
      {
        if (y)
          putc (',', f);
        put_single (json, pt, footnotes, y);
      }
  fputs ("]}\n", f);

  table_unref (title);
  table_unref (layers);
  table_unref (body);
  table_unref (caption);
  table_unref (footnotes);
}

static void
json_submit__ (struct output_driver *driver, const struct output_item *item,
               int level)
{
  struct json_driver *json = json_driver_cast (driver);
  FILE *f = json->file;

  switch (item->type)
    {
    case OUTPUT_ITEM_GROUP:
      for (size_t i = 0; i < item->group.n_children; i++)
        json_submit__ (driver, item->group.children[i], level + 1);
      break;

    case OUTPUT_ITEM_CHART:
      {
        const char *title = chart_get_title (item->chart);
        fputs ("{\"type\":\"chart\",\"title\":", f);
        put_string (f, title);
        fputs ("}\n", f);
      }
      break;

    case OUTPUT_ITEM_IMAGE:
      fputs ("{\"type\":\"image\"}\n", f);
      break;

    case OUTPUT_ITEM_MESSAGE:
      {
        char *s = msg_to_string (item->message);
        const char *sev = (item->message->severity == MSG_S_ERROR ? "error"
                           : item->message->severity == MSG_S_WARNING
                           ? "warning" : "note");
        fprintf (f, "{\"type\":\"message\",\"severity\":\"%s\",\"text\":", sev);
        put_string (f, s);
        fputs ("}\n", f);
        free (s);
      }
      break;

    case OUTPUT_ITEM_PAGE_BREAK:
      break;

    case OUTPUT_ITEM_TABLE:
      {
        size_t *layer_indexes;
        PIVOT_OUTPUT_FOR_EACH_LAYER (layer_indexes, item->table, true)
          json_output_table_layer (json, item, layer_indexes);
      }
      break;

    case OUTPUT_ITEM_TEXT:
      {
        char *s = text_item_get_plain_text (item);
        switch (item->text.subtype)
          {
          case TEXT_ITEM_PAGE_TITLE:
            break;
          case TEXT_ITEM_TITLE:
            fprintf (f, "{\"type\":\"title\",\"level\":%d,\"text\":", level);
            put_string (f, s);
            fputs ("}\n", f);
            break;
          case TEXT_ITEM_SYNTAX:
            fputs ("{\"type\":\"syntax\",\"text\":", f);
            put_string (f, s);
            fputs ("}\n", f);
            break;
          case TEXT_ITEM_LOG:
            fputs ("{\"type\":\"log\",\"text\":", f);
            put_string (f, s);
            fputs ("}\n", f);
            break;
          }
        free (s);
      }
      break;
    }
}

static void
json_submit (struct output_driver *driver, const struct output_item *item)
{
  json_submit__ (driver, item, 1);
  fflush (json_driver_cast (driver)->file);
}

static void
json_flush (struct output_driver *driver)
{
  fflush (json_driver_cast (driver)->file);
}

static void
json_destroy (struct output_driver *driver)
{
  struct json_driver *json = json_driver_cast (driver);

  if (json->file != NULL)
    fn_close (json->handle, json->file);
  fh_unref (json->handle);
  free (json);
}

static struct output_driver *
json_create (struct file_handle *fh, enum settings_output_devices device_type,
             struct driver_options *o UNUSED)
{
  FILE *file = fn_open (fh, "w");
  if (!file)
    {
      msg_error (errno, _("error opening output file `%s'"),
                 fh_get_file_name (fh));
      return NULL;
    }

  /* The first object in the stream tells readers which version of the
     format follows.  See "JSON Output Options" in the manual. */
  fputs ("{\"type\":\"header\",\"version\":" JSON_FORMAT_VERSION "}\n", file);

  struct json_driver *json = xmalloc (sizeof *json);
  *json = (struct json_driver) {
    .driver = {
      .class = &json_driver_class,
      .name = xstrdup (fh_get_file_name (fh)),
      .device_type = device_type
    },
    .handle = fh,
    .file = file,
  };
  return &json->driver;
}

const struct output_driver_factory json_driver_factory =
  { "json", "pspp.json", json_create };

static const struct output_driver_class json_driver_class =
  {
    .name = "json",
    .destroy = json_destroy,
    .submit = json_submit,
    .flush = json_flush,
    .handles_groups = true,
  };
