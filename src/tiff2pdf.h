//C-  -*- C -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License, either version 2 of the license,
//C- or (at your option) any later version. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

// $Id$

#ifndef TIFF2PDF_H
#define TIFF2PDF_H

#if AUTOCONF
# include "config.h"
#endif

#if HAVE_TIFF
# include <stdio.h>
# include <stdlib.h>
# include "tiffio.h"

# ifdef __cplusplus
extern "C"
# endif
int tiff2pdf(TIFF *input, FILE *output, int argc, const char **argv);

#endif /* HAVE_TIFF */
#endif /* TIFF2PDF_H */
