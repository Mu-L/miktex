/* 
Copyright (c) 2008-2011 jerome DOT laurens AT u-bourgogne DOT fr

This file is part of the SyncTeX package.

Latest Revision: Wed Aug 22 07:20:29 UTC 2011

License:
--------
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE

Except as contained in this notice, the name of the copyright holder  
shall not be used in advertising or otherwise to promote the sale,  
use or other dealings in this Software without prior written  
authorization from the copyright holder.
Acknowledgments:
----------------
The author received useful remarks from the pdfTeX developers, especially Hahn The Thanh,
and significant help from XeTeX developer Jonathan Kew

Nota Bene:
----------
If you include or use a significant part of the synctex package into a software,
I would appreciate to be listed as contributor and see "SyncTeX" highlighted.

Version 1
Thu Jun 19 09:39:21 UTC 2008

*/

#if defined(MIKTEX)
#  define C4PEXTERN extern
#  include "miktex-xetex.h"
#else
#  include "xetexd.h"
#endif
/* this will define XeTeX, which we can use in later conditionals */

#if !defined(MIKTEX)
#  include <xetexdir/xetexextra.h>
#endif

/* We observe nopdfoutput in order to determine whether output mode is
 * pdf or xdv. */
#  define SYNCTEX_OFFSET_IS_PDF (nopdfoutput==0)
#  define SYNCTEX_OUTPUT (nopdfoutput!=0?"xdv":"pdf")

#define SYNCTEX_CURH ((nopdfoutput==0)?(curh+4736287):curh)
#define SYNCTEX_CURV ((nopdfoutput==0)?(curv+4736287):curv)

/*  WARNING:
    The definition below must be in sync with their eponym declarations in synctex-xetex.ch1
*/
#  define synchronization_field_size 1

/* in XeTeX, "halfword" fields are at least 32 bits, so we'll use those for
 * tag and line so that the sync field size is only one memory_word. */
#if defined(MIKTEX)
#  define SYNCTEX_TAG_MODEL(NODE,TYPE)\
                mem[NODE+TYPE##_node_size-synchronization_field_size].c4p_P2.hh.c4p_P1.lh
#  define SYNCTEX_LINE_MODEL(NODE,TYPE)\
                mem[NODE+TYPE##_node_size-synchronization_field_size].c4p_P2.hh.rh
#else
#  define SYNCTEX_TAG_MODEL(NODE,TYPE)\
                mem[NODE+TYPE##_node_size-synchronization_field_size].hh.lhfield
#  define SYNCTEX_LINE_MODEL(NODE,TYPE)\
                mem[NODE+TYPE##_node_size-synchronization_field_size].hh.rh
#endif