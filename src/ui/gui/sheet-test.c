/* This is a file */

#include <config.h>

#include <gtk/gtk.h>

#include "psppire-spreadsheet-model.h"

#include "data/gnumeric-reader.h"
#include "data/spreadsheet-reader.h"
#include "data/casereader.h"
#include "data/case.h"

#define N 10

#if 0
static GtkListStore *
make_store ()
  {
    int i;
    GtkTreeIter iter;
    
    GtkListStore * list_store  = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);

    for (i = 0; i < N; ++i)
      {
	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter,
			    0, N - i,
			    1, "xxx", 
			    -1);
      }
    return list_store;
  }
#endif

struct spreadsheet *sp = NULL;
GtkWidget *combo_box;

static void
on_clicked (GtkButton *button, gpointer data)
{
  struct ccase *c;
  gint x = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
  struct casereader *reader ;
  struct spreadsheet_read_info ri;
  struct spreadsheet_read_options opts;

  g_print( "%s %d\n", __FUNCTION__, x);

  opts.sheet_index = x + 1;
  opts.cell_range = NULL;
  opts.sheet_name = NULL;

  ri.read_names = TRUE;
  ri.asw = -1;

  reader = gnumeric_make_reader (sp, &ri, &opts);
  for (;
           (c = casereader_read (reader)) != NULL; case_unref (c))
    {
      const double val = case_data_idx (c, 0)->f;
      printf ("%g\n", val);
    }
}

int
main (int argc, char *argv[] )
{
  GtkWidget *window;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *treeview;

  GtkTreeModel *tm;
  GtkWidget *button;

  gtk_init (&argc, &argv);
    
  if ( argc < 2)
    g_error ("Usage: prog file\n");

  sp = gnumeric_probe (argv[1], true);
  
  if (sp == NULL)
    {
      g_error ("%s is not a gnumeric file\n", argv[1]);
      return 0;
    }

  tm = psppire_spreadsheet_model_new (sp);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  hbox = gtk_hbox_new (FALSE, 5);
  vbox = gtk_vbox_new (FALSE, 5);

  button = gtk_button_new_with_label ("Test reader");
  g_signal_connect (button, "clicked", G_CALLBACK (on_clicked), NULL);
   
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  
  combo_box = gtk_combo_box_new();

  {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
				    "text", 0,
				    NULL);
  }

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), tm);

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);

  treeview = gtk_tree_view_new_with_model (tm);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
					       0, "sheet name",
					       gtk_cell_renderer_text_new (),
					       "text", 0,
					       NULL);


  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
					       1, "range",
					       gtk_cell_renderer_text_new (),
					       "text", 1,
					       NULL);


  gtk_box_pack_start (GTK_BOX (hbox), treeview, TRUE, TRUE, 5);

  gtk_box_pack_start (GTK_BOX (vbox), combo_box, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 5);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  g_signal_connect (window, "destroy", gtk_main_quit, 0);

  gtk_widget_show_all (window);

  gtk_main ();

  gnumeric_destroy (sp);
    
  return 0;
}