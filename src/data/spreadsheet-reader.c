/* PSPP - a program for statistical analysis.
   Copyright (C) 2007, 2009, 2010, 2011, 2013, 2020 Free Software Foundation, Inc.

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

#include <config.h>

#include "spreadsheet-reader.h"

#include <libpspp/assertion.h>
#include "gnumeric-reader.h"
#include "ods-reader.h"

#include <libpspp/str.h>
#include <stdio.h>
#include <string.h>
#include <gl/xalloc.h>
#include <gl/c-xvasprintf.h>
#include <stdlib.h>

struct spreadsheet *
spreadsheet_ref (struct spreadsheet *s)
{
  s->ref_cnt++;
  return s;
}

void
spreadsheet_unref (struct spreadsheet *s)
{
  if (--s->ref_cnt == 0)
    s->destroy (s);
}


struct casereader *
spreadsheet_make_reader (struct spreadsheet *s,
                         const struct spreadsheet_read_options *opts)
{
  return s->make_reader (s, opts);
}

const char *
spreadsheet_get_sheet_name (struct spreadsheet *s, int n)
{
  return s->get_sheet_name (s, n);
}


char *
spreadsheet_get_sheet_range (struct spreadsheet *s, int n)
{
  return s->get_sheet_range (s, n);
}

int
spreadsheet_get_sheet_n_sheets (struct spreadsheet *s)
{
  return s->get_sheet_n_sheets (s);
}

unsigned int
spreadsheet_get_sheet_n_rows (struct spreadsheet *s, int n)
{
  return s->get_sheet_n_rows (s, n);
}

unsigned int
spreadsheet_get_sheet_n_columns (struct spreadsheet *s, int n)
{
  return s->get_sheet_n_columns (s, n);
}

char *
spreadsheet_get_cell (struct spreadsheet *s, int n, int row, int column)
{
  return s->get_sheet_cell (s, n, row, column);
}


#define RADIX 26

static void
reverse (char *s, int len)
{
  int i;
  for (i = 0; i < len / 2; ++i)
    {
      char tmp = s[len - i - 1];
      s[len - i -1] = s[i];
      s[i] = tmp;
    }
}


/* Convert a string, which is an integer encoded in base26
   IE, A=0, B=1, ... Z=25 to the integer it represents.
   ... except that in this scheme, digits with an exponent
   greater than 1 are implicitly incremented by 1, so
   AA  = 0 + 1*26, AB = 1 + 1*26,
   ABC = 2 + 2*26 + 1*26^2 ....
   On error, this function returns -1
*/
int
ps26_to_int (const char *str)
{
  int i;
  int multiplier = 1;
  int result = 0;
  int len = strlen (str);

  for (i = len - 1 ; i >= 0; --i)
    {
      char c = str[i];
      if (c < 'A' || c > 'Z')
	return -1;
      int mantissa = (c - 'A');

      assert (mantissa >= 0);
      assert (mantissa < RADIX);

      if (i != len - 1)
	mantissa++;

      result += mantissa * multiplier;
      multiplier *= RADIX;
    }

  return result;
}

/* Convert an integer, which must be non-negative,
   to pseudo base 26.
   The caller must free the return value when no longer required.  */
char *
int_to_ps26 (int i)
{
  char *ret = NULL;

  int lower = 0;
  long long int base = RADIX;
  int exp = 1;

  if (i < 0)
    return NULL;

  while (i > lower + base - 1)
    {
      lower += base;
      base *= RADIX;
      assert (base > 0);
      exp++;
    }

  i -= lower;
  i += base;

  ret = xmalloc (exp + 1);

  exp = 0;
  do
    {
      ret[exp++] = (i % RADIX) + 'A';
      i /= RADIX;
    }
  while (i > 1);

  ret[exp]='\0';

  reverse (ret, exp);
  return ret;
}


char *
create_cell_ref (int col0, int row0)
{
  char *cs0 ;
  char *s ;

  if (col0 < 0) return NULL;
  if (row0 < 0) return NULL;

  cs0 = int_to_ps26 (col0);
  s =  c_xasprintf ("%s%d", cs0, row0 + 1);

  free (cs0);

  return s;
}

char *
create_cell_range (int col0, int row0, int coli, int rowi)
{
  char *s0 = create_cell_ref (col0, row0);
  char *si = create_cell_ref (coli, rowi);

  char *s =  c_xasprintf ("%s:%s", s0, si);

  free (s0);
  free (si);

  return s;
}


/* Convert a cell reference in the form "A1:B2", to
   integers.  A1 means column zero, row zero.
   B1 means column 1 row 0. AA1 means column 26, row 0.
*/
bool
convert_cell_ref (const char *ref,
		  int *col0, int *row0,
		  int *coli, int *rowi)
{
  char startcol[5];
  char stopcol [5];

  int startrow;
  int stoprow;

  int n = sscanf (ref, "%4[a-zA-Z]%d:%4[a-zA-Z]%d",
	      startcol, &startrow,
	      stopcol, &stoprow);
  if (n != 4)
    return false;

  str_uppercase (startcol);
  *col0 = ps26_to_int (startcol);
  str_uppercase (stopcol);
  *coli = ps26_to_int (stopcol);
  *row0 = startrow - 1;
  *rowi = stoprow - 1 ;

  return true;
}
