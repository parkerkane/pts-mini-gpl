/*
 *	Fast block move/copy subroutines
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
/* This module requires ALIGNED and SIZEOF_INT to be defined correctly */

#include "types.h"

#define BITS 8

#if SIZEOF_INT == 8
#  define SHFT 3
#elif SIZEOF_INT == 4
#  define SHFT 2
#elif SIZEOF_INT == 2
#  define SHFT 1
#endif

/* Set 'sz' 'int's beginning at 'd' to the value 'c' */
/* Returns address of block.  Does nothing if 'sz' equals zero */

int *msetI(void *dest, int c, int sz)
{
	int	*d = dest;
	int	*orgd = dest;

	while (sz >= 16) {
		d[0] = c;
		d[1] = c;
		d[2] = c;
		d[3] = c;
		d[4] = c;
		d[5] = c;
		d[6] = c;
		d[7] = c;
		d[8] = c;
		d[9] = c;
		d[10] = c;
		d[11] = c;
		d[12] = c;
		d[13] = c;
		d[14] = c;
		d[15] = c;
		d += 16;
		sz -= 16;
	}
	switch (sz) {
	case 15:	d[14] = c;
	case 14:	d[13] = c;
	case 13:	d[12] = c;
	case 12:	d[11] = c;
	case 11:	d[10] = c;
	case 10:	d[9] = c;
	case 9:		d[8] = c;
	case 8:		d[7] = c;
	case 7:		d[6] = c;
	case 6:		d[5] = c;
	case 5:		d[4] = c;
	case 4:		d[3] = c;
	case 3:		d[2] = c;
	case 2:		d[1] = c;
	case 1:		d[0] = c;
	case 0:		/* do nothing */;
	}
	return orgd;
}

/* Set 'sz' 'int's beginning at 'd' to the value 'c' */
/* Returns address of block.  Does nothing if 'sz' equals zero */

void **msetP(void **d, void *c, int sz)
{
	void	**orgd = d;

	while (sz >= 16) {
		d[0] = c;
		d[1] = c;
		d[2] = c;
		d[3] = c;
		d[4] = c;
		d[5] = c;
		d[6] = c;
		d[7] = c;
		d[8] = c;
		d[9] = c;
		d[10] = c;
		d[11] = c;
		d[12] = c;
		d[13] = c;
		d[14] = c;
		d[15] = c;
		d += 16;
		sz -= 16;
	}
	switch (sz) {
	case 15:	d[14] = c;
	case 14:	d[13] = c;
	case 13:	d[12] = c;
	case 12:	d[11] = c;
	case 11:	d[10] = c;
	case 10:	d[9] = c;
	case 9:		d[8] = c;
	case 8:		d[7] = c;
	case 7:		d[6] = c;
	case 6:		d[5] = c;
	case 5:		d[4] = c;
	case 4:		d[3] = c;
	case 3:		d[2] = c;
	case 2:		d[1] = c;
	case 1:		d[0] = c;
	case 0:		/* do nothing */;
	}
	return orgd;
}

/* Utility to count number of lines within a segment */

int mcnt(register unsigned char *blk, register unsigned char c, int size)
{
	register int nlines = 0;

	while (size >= 16) {
		if (blk[0] == c) ++nlines;
		if (blk[1] == c) ++nlines;
		if (blk[2] == c) ++nlines;
		if (blk[3] == c) ++nlines;
		if (blk[4] == c) ++nlines;
		if (blk[5] == c) ++nlines;
		if (blk[6] == c) ++nlines;
		if (blk[7] == c) ++nlines;
		if (blk[8] == c) ++nlines;
		if (blk[9] == c) ++nlines;
		if (blk[10] == c) ++nlines;
		if (blk[11] == c) ++nlines;
		if (blk[12] == c) ++nlines;
		if (blk[13] == c) ++nlines;
		if (blk[14] == c) ++nlines;
		if (blk[15] == c) ++nlines;
		blk += 16;
		size -= 16;
	}
	switch (size) {
	case 15:	if (blk[14] == c) ++nlines;
	case 14:	if (blk[13] == c) ++nlines;
	case 13:	if (blk[12] == c) ++nlines;
	case 12:	if (blk[11] == c) ++nlines;
	case 11:	if (blk[10] == c) ++nlines;
	case 10:	if (blk[9] == c) ++nlines;
	case 9:		if (blk[8] == c) ++nlines;
	case 8:		if (blk[7] == c) ++nlines;
	case 7:		if (blk[6] == c) ++nlines;
	case 6:		if (blk[5] == c) ++nlines;
	case 5:		if (blk[4] == c) ++nlines;
	case 4:		if (blk[3] == c) ++nlines;
	case 3:		if (blk[2] == c) ++nlines;
	case 2:		if (blk[1] == c) ++nlines;
	case 1:		if (blk[0] == c) ++nlines;
	case 0:		/* do nothing */;
	}
	return nlines;
}
