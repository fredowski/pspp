/* PSPPIRE - a graphical user interface for PSPP.
   Copyright (C) 2008, 2010, 2011, 2012, 2013, 2014, 2016  Free Software Foundation

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


#ifndef __PSPPIRE_DATA_WINDOW_H__
#define __PSPPIRE_DATA_WINDOW_H__

/* PsppireDataWindow is a top-level window for editing a PSPP dataset.

   PsppireDataWindow contains a PsppireDataEditor.

   PsppireDataWindow's own functionality basically amounts to managing menus
   and toolbars.  In addition to maintaining some menu itema and toolbar items
   of its own, it merges in menu and toolbar items provided by its child
   PsppireDataEditor (based on the "ui-manager" property of PsppireDataEditor).
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "libpspp/ll.h"
#include "ui/gui/psppire-window.h"
#include "ui/gui/psppire-data-editor.h"

G_BEGIN_DECLS

#define PSPPIRE_DATA_WINDOW_TYPE            (psppire_data_window_get_type ())
#define PSPPIRE_DATA_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PSPPIRE_DATA_WINDOW_TYPE, PsppireDataWindow))
#define PSPPIRE_DATA_WINDOW_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class), \
    PSPPIRE_DATA_WINDOW_TYPE, PsppireData_WindowClass))
#define PSPPIRE_IS_DATA_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
    PSPPIRE_DATA_WINDOW_TYPE))
#define PSPPIRE_IS_DATA_WINDOW_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), \
    PSPPIRE_DATA_WINDOW_TYPE))


typedef struct _PsppireDataWindow       PsppireDataWindow;
typedef struct _PsppireDataWindowClass  PsppireDataWindowClass;


enum PsppireDataWindowFormat {
  PSPPIRE_DATA_WINDOW_SAV,
  PSPPIRE_DATA_WINDOW_ZSAV,
  PSPPIRE_DATA_WINDOW_POR
};

struct _PsppireDataWindow
{
  PsppireWindow parent;

  gboolean dispose_has_run;

  /* <private> */
  PsppireDataEditor *data_editor;
  GtkBuilder *builder;

  PsppireDict *dict;
  struct dataset *dataset;
  PsppireDataStore *data_store;

  enum PsppireDataWindowFormat format;

  struct ll ll;                 /* In global 'all_data_windows' list. */
  unsigned long int lazy_serial;
  unsigned int dataset_seqno;

  GtkToolItem *ti_value_labels_button;

  GtkToolItem *ti_jump_to_variable;
  GtkToolItem *ti_insert_variable;
  GtkToolItem *ti_jump_to_case;
  GtkToolItem *ti_insert_case;
  GtkToolItem *ti_find;

  GtkWidget *mi_go_to_case;
  GtkWidget *mi_insert_case;
  GtkWidget *mi_find;
  GtkWidget *mi_find_separator;
  GtkWidget *mi_options;

  GtkWidget *mi_edit_separator ;
  GtkWidget *mi_cut;
  GtkWidget *mi_copy;
  GtkWidget *mi_paste;
  GtkWidget *mi_clear_variables;
  GtkWidget *mi_clear_cases;
  GtkWidget *mi_insert_var;
};

struct _PsppireDataWindowClass
{
  PsppireWindowClass parent_class;
};

extern struct session *the_session;
extern struct ll_list all_data_windows;

GType      psppire_data_window_get_type        (void);
GtkWidget* psppire_data_window_new             (struct dataset *);

PsppireDataWindow *psppire_default_data_window (void);
void psppire_data_window_set_default (PsppireDataWindow *);
void psppire_data_window_undefault (PsppireDataWindow *);

PsppireDataWindow *psppire_data_window_for_dataset (struct dataset *);
PsppireDataWindow *psppire_data_window_for_data_store (PsppireDataStore *);

bool psppire_data_window_is_empty (PsppireDataWindow *);
GtkWindow * create_data_window (void);
GtkWindow * open_data_window (PsppireWindow *victim, const char *file_name,
                       const char *encoding, gpointer hint);

G_END_DECLS

#endif /* __PSPPIRE_DATA_WINDOW_H__ */
