/*
** Copyright (C) 2004 Lars Helmer <lasso@spacecentre.se>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/

/* hashstring.c -- calculates simple hash-values from strings given as
   parameters */

/* Version 0.1.0 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char** args)
{
  int i, j, hash;

  for (i = 1; i < argc; i++)
  {
    hash = 0;
    for (j = 0; j < (int)strlen (args[i]); j++)
      hash += args[i][j];
    printf ("%s: %d\n", args[i], hash);
  }
  
  return EXIT_SUCCESS;
}
