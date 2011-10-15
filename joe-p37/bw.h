/*
 *	Edit buffer window generation
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#ifndef _JOE_BW_H
#define _JOE_BW_H 1

/* A buffer window: there are several kinds, depending on what is in 'object' (usually a TW) */

struct bw {
	W	*parent;
	B	*b;
	P	*top;
	P	*cursor;
	long	offset;
	Screen	*t;
	int	h, w, x, y;

	OPTIONS	o;
	void	*object;

	int	linums;
	int	top_changed;	/* Top changed */
	struct lattr_db *db;	/* line attribute database */
};

extern int dspasis;	/* Display characters above 127 as-is */
extern int mid;		/* Controls how window scrolls: when set, scroll window enough so that line with cursor becomes centered */

void bwfllw PARAMS((BW *bw));
void bwfllwt PARAMS((BW *bw));
void bwfllwh PARAMS((BW *bw));
void bwins PARAMS((BW *bw, long int l, long int n, int flg));
void bwdel PARAMS((BW *bw, long int l, long int n, int flg));
void bwgen PARAMS((BW *bw, int linums));
void bwgenh PARAMS((BW *bw));
BW *bwmk PARAMS((W *window, B *b, int prompt));
void bwmove PARAMS((BW *bw, int x, int y));
void bwresz PARAMS((BW *bw, int wi, int he));
void bwrm PARAMS((BW *bw));
int ustat PARAMS((BW *bw));
int ucrawll PARAMS((BW *bw));
int ucrawlr PARAMS((BW *bw));
void orphit PARAMS((BW *bw));

extern int marking;	/* Anchored block marking mode */

void save_file_pos PARAMS((FILE *f));
void load_file_pos PARAMS((FILE *f));
long get_file_pos PARAMS((unsigned char *name));
void set_file_pos PARAMS((unsigned char *name, long pos));

extern int restore_file_pos;

void set_file_pos_all PARAMS((Screen *t));

#endif
