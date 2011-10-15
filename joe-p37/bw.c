/*
 *	Edit buffer window generation
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

/* Display modes */
int dspasis = 0;
int marking = 0;

static P *bwgetto(P *p, P *cur, P *top, long int line)
{

	if (p == NULL) {
		P *best = cur;
		long dist = MAXLONG;
		long d;

		d = (line >= cur->line ? line - cur->line : cur->line - line);
		if (d < dist) {
			dist = d;
			best = cur;
		}
		d = (line >= top->line ? line - top->line : top->line - line);
		if (d < dist) {
			dist = d;
			best = top;
		}
		p = pdup(best, USTR "bwgetto");
		p_goto_bol(p);
	}
	while (line > p->line)
		if (!pnextl(p))
			break;
	if (line < p->line) {
		while (line < p->line)
			pprevl(p);
		p_goto_bol(p);
	}
	return p;
}

/* Scroll window to follow cursor */

int mid = 0;

/* For hex */

void bwfllwh(BW *bw)
{
	/* Top must be a muliple of 16 bytes */
	if (bw->top->byte%16) {
		pbkwd(bw->top,bw->top->byte%16);
	}

	/* Move backward */
	if (bw->cursor->byte < bw->top->byte) {
		long new_top = bw->cursor->byte/16;
		if (mid) {
			if (new_top >= bw->h / 2)
				new_top -= bw->h / 2;
			else
				new_top = 0;
		}
		if (bw->top->byte/16 - new_top < bw->h)
			nscrldn(bw->t->t, bw->y, bw->y + bw->h, (int) (bw->top->byte/16 - new_top));
		else
			msetI(bw->t->t->updtab + bw->y, 1, bw->h);
		pgoto(bw->top,new_top*16);
	}

	/* Move forward */
	if (bw->cursor->byte >= bw->top->byte+(bw->h*16)) {
		long new_top;
		if (mid) {
			new_top = bw->cursor->byte/16 - bw->h / 2;
		} else {
			new_top = bw->cursor->byte/16 - (bw->h - 1);
		}
		if (new_top - bw->top->byte/16 < bw->h)
			nscrlup(bw->t->t, bw->y, bw->y + bw->h, (int) (new_top - bw->top->byte/16));
		else {
			msetI(bw->t->t->updtab + bw->y, 1, bw->h);
		}
		pgoto(bw->top, new_top*16);
	}

	/* Adjust scroll offset */
	if (bw->cursor->byte%16+60 < bw->offset) {
		bw->offset = bw->cursor->byte%16+60;
		msetI(bw->t->t->updtab + bw->y, 1, bw->h);
	} else if (bw->cursor->byte%16+60 >= bw->offset + bw->w) {
		bw->offset = bw->cursor->byte%16+60 - (bw->w - 1);
		msetI(bw->t->t->updtab + bw->y, 1, bw->h);
	}
}

/* For text */

void bwfllwt(BW *bw)
{
	P *newtop;

	if (!pisbol(bw->top)) {
		p_goto_bol(bw->top);
	}

	if (bw->cursor->line < bw->top->line) {
		newtop = pdup(bw->cursor, USTR "bwfllwt");
		p_goto_bol(newtop);
		if (mid) {
			if (newtop->line >= bw->h / 2)
				pline(newtop, newtop->line - bw->h / 2);
			else
				pset(newtop, newtop->b->bof);
		}
		if (bw->top->line - newtop->line < bw->h)
			nscrldn(bw->t->t, bw->y, bw->y + bw->h, (int) (bw->top->line - newtop->line));
		else {
			msetI(bw->t->t->updtab + bw->y, 1, bw->h);
		}
		pset(bw->top, newtop);
		prm(newtop);
	} else if (bw->cursor->line >= bw->top->line + bw->h) {
		/* newtop = pdup(bw->top); */
		/* bwgetto() creates newtop */
		if (mid)
			newtop = bwgetto(NULL, bw->cursor, bw->top, bw->cursor->line - bw->h / 2);
		else
			newtop = bwgetto(NULL, bw->cursor, bw->top, bw->cursor->line - (bw->h - 1));
		if (newtop->line - bw->top->line < bw->h)
			nscrlup(bw->t->t, bw->y, bw->y + bw->h, (int) (newtop->line - bw->top->line));
		else {
			msetI(bw->t->t->updtab + bw->y, 1, bw->h);
		}
		pset(bw->top, newtop);
		prm(newtop);
	}

/* Adjust column */
	if (bw->cursor->xcol < bw->offset) {
		long target = bw->cursor->xcol;
		if (target < 5)
			target = 0;
		else {
			target -= 5;
			target -= (target % 5);
		}
		bw->offset = target;
		msetI(bw->t->t->updtab + bw->y, 1, bw->h);
	}
	if (bw->cursor->xcol >= bw->offset + bw->w) {
		bw->offset = bw->cursor->xcol - (bw->w - 1);
		msetI(bw->t->t->updtab + bw->y, 1, bw->h);
	}
}

/* For either */

void bwfllw(BW *bw)
{
	if (bw->o.hex)
		bwfllwh(bw);
	else
		bwfllwt(bw);
}

/* Determine highlighting state of a particular line on the window.
   If the state is not known, it is computed and the state for all
   of the remaining lines of the window are also recalculated. */

HIGHLIGHT_STATE get_highlight_state(BW *bw, P *p, int line)
{
	HIGHLIGHT_STATE state;

	if(!bw->o.highlight || !bw->o.syntax) {
		invalidate_state(&state);
		return state;
	}

	return lattr_get(bw->db, bw->o.syntax, p, line);
}

/* Scroll a buffer window after an insert occured.  'flg' is set to 1 if
 * the first line was split
 */

void bwins(BW *bw, long int l, long int n, int flg)
{
	/* If highlighting is enabled... */
	if (bw->o.highlight && bw->o.syntax) {
		/* Invalidate cache */
		/* lattr_cut(bw->db, l + 1); */
		/* Force updates */
		if (l < bw->top->line) {
			msetI(bw->t->t->updtab + bw->y, 1, bw->h);
		} else if ((l + 1) < bw->top->line + bw->h) {
			int start = l + 1 - bw->top->line;
			int size = bw->h - start;
			msetI(bw->t->t->updtab + bw->y + start, 1, size);
		}
	}

	/* Scroll */
	if (l + flg + n < bw->top->line + bw->h && l + flg >= bw->top->line && l + flg <= bw->b->eof->line) {
		if (flg)
			bw->t->t->sary[bw->y + l - bw->top->line] = bw->t->t->li;
		nscrldn(bw->t->t, (int) (bw->y + l + flg - bw->top->line), bw->y + bw->h, (int) n);
	}

	/* Force update of lines in opened hole */
	if (l < bw->top->line + bw->h && l >= bw->top->line) {
		if (n >= bw->h - (l - bw->top->line)) {
			msetI(bw->t->t->updtab + bw->y + l - bw->top->line, 1, bw->h - (int) (l - bw->top->line));
		} else {
			msetI(bw->t->t->updtab + bw->y + l - bw->top->line, 1, (int) n + 1);
		}
	}
}

/* Scroll current windows after a delete */

void bwdel(BW *bw, long int l, long int n, int flg)
{
	/* If highlighting is enabled... */
	if (bw->o.highlight && bw->o.syntax) {
		/* lattr_cut(bw->db, l + 1); */
		if (l < bw->top->line) {
			msetI(bw->t->t->updtab + bw->y, 1, bw->h);
		} else if ((l + 1) < bw->top->line + bw->h) {
			int start = l + 1 - bw->top->line;
			int size = bw->h - start;
			msetI(bw->t->t->updtab + bw->y + start, 1, size);
		}
	}

	/* Update the line where the delete began */
	if (l < bw->top->line + bw->h && l >= bw->top->line)
		bw->t->t->updtab[bw->y + l - bw->top->line] = 1;

	/* Update the line where the delete ended */
	if (l + n < bw->top->line + bw->h && l + n >= bw->top->line)
		bw->t->t->updtab[bw->y + l + n - bw->top->line] = 1;

	if (l < bw->top->line + bw->h && (l + n >= bw->top->line + bw->h || (l + n == bw->b->eof->line && bw->b->eof->line >= bw->top->line + bw->h))) {
		if (l >= bw->top->line)
			/* Update window from l to end */
			msetI(bw->t->t->updtab + bw->y + l - bw->top->line, 1, bw->h - (int) (l - bw->top->line));
		else
			/* Update entire window */
			msetI(bw->t->t->updtab + bw->y, 1, bw->h);
	} else if (l < bw->top->line + bw->h && l + n == bw->b->eof->line && bw->b->eof->line < bw->top->line + bw->h) {
		if (l >= bw->top->line)
			/* Update window from l to end of file */
			msetI(bw->t->t->updtab + bw->y + l - bw->top->line, 1, (int) n);
		else
			/* Update from beginning of window to end of file */
			msetI(bw->t->t->updtab + bw->y, 1, (int) (bw->b->eof->line - bw->top->line));
	} else if (l + n < bw->top->line + bw->h && l + n > bw->top->line && l + n < bw->b->eof->line) {
		if (l + flg >= bw->top->line)
			nscrlup(bw->t->t, (int) (bw->y + l + flg - bw->top->line), bw->y + bw->h, (int) n);
		else
			nscrlup(bw->t->t, bw->y, bw->y + bw->h, (int) (l + n - bw->top->line));
	}
}

/* Update a single line */

static int lgen(SCRN *t, int y, int *screen, int *attr, int x, int w, P *p, long int scr, long int from, long int to,HIGHLIGHT_STATE st,BW *bw, int fromto_modifiers)
        
      
            			/* Screen line address */
      				/* Window */
     				/* Buffer pointer */
         			/* Starting column to display */
              			/* Range for marked block */
              			/* fromto_modifiers: BOLD or INVERSE. Modifiers to apply between from and to. */
{
	int ox = x;
	int tach;
	int done = 1;
	long col = 0;
	long byte = p->byte;
	unsigned char *bp;	/* Buffer pointer, 0 if not set */
	int amnt;		/* Amount left in this segment of the buffer */
	int c, ta, c1 = 0;
	unsigned char bc;
	int ungetit = -1;

	struct utf8_sm utf8_sm;

        int *syn = 0;
        P *tmp;
        int idx=0;
        int atr = BG_COLOR(bg_text); 

	utf8_init(&utf8_sm);  /* Sets utf8_sm.state = 0. */

	/* assert(y < t->li); */
	/* y == t->li - 1 might happen here. */

	if(st.state!=-1) {
		tmp=pdup(p, USTR "lgen");
		p_goto_bol(tmp);
		parse(bw->o.syntax,tmp,st);
		syn = attr_buf;
		prm(tmp);
	}

/* Initialize bp and amnt from p */
	if (p->ofst >= p->hdr->hole) {
		bp = p->ptr + p->hdr->ehole + p->ofst - p->hdr->hole;
		amnt = SEGSIZ - p->hdr->ehole - (p->ofst - p->hdr->hole);
	} else {
		bp = p->ptr + p->ofst;
		amnt = p->hdr->hole - p->ofst;
	}

	if (col == scr)
		goto loop;
      lp:			/* Display next character */
	if (amnt)
		do {
			if (ungetit== -1)
				bc = *bp++;
			else {
				bc = ungetit;
				ungetit = -1;
			}
			if(st.state!=-1) {
				atr = syn[idx++];
				if (!((atr & BG_VALUE) >> BG_SHIFT))
					atr |= BG_COLOR(bg_text);
			}
			if (p->b->o.crlf && bc == '\r') {
				++byte;
				if (!--amnt) {
				      pppl:
					if (bp == p->ptr + SEGSIZ) {
						if (pnext(p)) {
							bp = p->ptr;
							amnt = p->hdr->hole;
						} else
							goto nnnl;
					} else {
						bp = p->ptr + p->hdr->ehole;
						amnt = SEGSIZ - p->hdr->ehole;
						if (!amnt)
							goto pppl;
					}
				}
				if (*bp == '\n') {
					++bp;
					++byte;
					++amnt;
					goto eobl;
				}
			      nnnl:
				--byte;
				++amnt;
			}
			if (square)
				if (bc == '\t') {
					long tcol = col + p->b->o.tab - col % p->b->o.tab;

					if (tcol > from && tcol <= to)
						c1 = fromto_modifiers;
					else
						c1 = 0;
				} else if (col >= from && col < to)
					c1 = fromto_modifiers;
				else
					c1 = 0;
			else if (byte >= from && byte < to)
				c1 = fromto_modifiers;
			else
				c1 = 0;
			++byte;
			if (bc == '\t') {
				ta = p->b->o.tab - col % p->b->o.tab;
				if (ta + col > scr) {
					ta -= scr - col;
					tach = ' ';
					goto dota;
				}
				if ((col += ta) == scr) {
					--amnt;
					goto loop;
				}
			} else if (bc == '\n')
				goto eobl;
			else {
				int wid = 1;
				if (p->b->o.charmap->type) {
					c = utf8_decode(&utf8_sm,bc);

					if (c>=0) /* Normal decoded character */
						wid = joe_wcwidth(1,c);
					else if(c== -1) /* Character taken */
						wid = -1;
					else if(c== -2) { /* Incomplete sequence (FIXME: do something better here) */
						wid = 1;
						ungetit = c;
						++amnt;
						--byte;
					}
					else if(c== -3) /* Control character 128-191, 254, 255 */
						wid = 1;
				} else {
					wid = 1;
				}

				if(wid>0) {
					col += wid;
					if (col == scr) {
						--amnt;
						goto loop;
					} else if (col > scr) {
						ta = col - scr;
						tach = '<';
						goto dota;
					}
				} else
					--idx;	/* Get highlighting character again.. */
			}
		} while (--amnt);
	if (bp == p->ptr + SEGSIZ) {
		if (pnext(p)) {
			bp = p->ptr;
			amnt = p->hdr->hole;
			goto lp;
		}
	} else {
		bp = p->ptr + p->hdr->ehole;
		amnt = SEGSIZ - p->hdr->ehole;
		goto lp;
	}
	goto eof;

      loop:			/* Display next character */
	if (amnt)
		do {
			if (ungetit== -1)
				bc = *bp++;
			else {
				bc = ungetit;
				ungetit = -1;
			}
			if(st.state!=-1) {
				atr = syn[idx++];
				if (!(atr & BG_MASK))
					atr |= BG_COLOR(bg_text);
			}
			if (p->b->o.crlf && bc == '\r') {
				++byte;
				if (!--amnt) {
				      ppl:
					if (bp == p->ptr + SEGSIZ) {
						if (pnext(p)) {
							bp = p->ptr;
							amnt = p->hdr->hole;
						} else
							goto nnl;
					} else {
						bp = p->ptr + p->hdr->ehole;
						amnt = SEGSIZ - p->hdr->ehole;
						if (!amnt)
							goto ppl;
					}
				}
				if (*bp == '\n') {
					++bp;
					++byte;
					++amnt;
					goto eobl;
				}
			      nnl:
				--byte;
				++amnt;
			}
			if (square)
				if (bc == '\t') {
					long tcol = scr + x - ox + p->b->o.tab - (scr + x - ox) % p->b->o.tab;

					if (tcol > from && tcol <= to)
						c1 = fromto_modifiers;
					else
						c1 = 0;
				} else if (scr + x - ox >= from && scr + x - ox < to)
					c1 = fromto_modifiers;
				else
					c1 = 0;
			else if (byte >= from && byte < to)
				c1 = fromto_modifiers;
			else
				c1 = 0;
			++byte;
			if (bc == '\t') {
				ta = p->b->o.tab - ((x - ox + scr) % p->b->o.tab);
				tach = ' ';
			      dota:
				do {
					outatr(bw->b->o.charmap, t, screen + x, attr + x, x, y, tach, c1|atr);
					if (ifhave)
						goto bye;
					if (++x == w)
						goto eosl;
				} while (--ta);
			} else if (bc == '\n')
				goto eobl;
			else {
				int wid = -1;
				int utf8_char;
				if (p->b->o.charmap->type) { /* UTF-8 */

					utf8_char = utf8_decode(&utf8_sm,bc);
					if (utf8_char >= 0) { /* Normal decoded character */
						wid = joe_wcwidth(1,utf8_char);
					} else if(utf8_char== -1) { /* Character taken */
						wid = -1;
					} else if(utf8_char== -2) { /* Incomplete sequence (FIXME: do something better here) */
						ungetit = bc;
						++amnt;
						--byte;
						utf8_char = '$';
						wid = 1;
						c1 ^= UNDERLINE;
					} else if(utf8_char== -3) { /* Invalid UTF-8 start character 128-191, 254, 255 */
						/* Show as control character */
						wid = 1;
						utf8_char = '&';
						c1 ^= UNDERLINE;
					}
				} else { /* Regular */
					utf8_char = bc;
					wid = 1;
				}

				if(wid>=0) {
					if (x+wid > w) {
						/* If multibyte character hits right most column, don't display it */
						/* TODO(pts): Display the beginning of it, e.g. `<DFF' instead of `<<<' for U+DFF0. */
						while (x < w) {
							outatr(bw->b->o.charmap, t, screen + x, attr + x, x, y, '>', (c1^UNDERLINE)|atr);
							x++;
						}
					} else {
						outatr(bw->b->o.charmap, t, screen + x, attr + x, x, y, utf8_char, c1|atr);
						x += wid;
					}
				} else
					--idx;

				if (ifhave)
					goto bye;
				if (x >= w)
					goto eosl;
			}
		} while (--amnt);
	if (bp == p->ptr + SEGSIZ) {
		if (pnext(p)) {
			bp = p->ptr;
			amnt = p->hdr->hole;
			goto loop;
		}
	} else {
		bp = p->ptr + p->hdr->ehole;
		amnt = SEGSIZ - p->hdr->ehole;
		goto loop;
	}
	goto eof;

      eobl:			/* End of buffer line found.  Erase to end of screen line */
        if (utf8_sm.state) {  /* Unfinished UTF-8 sequence at EOL. */
        	c1 ^= UNDERLINE;
		outatr(bw->b->o.charmap, t, screen + x, attr + x, x, y, '$', c1|atr);
		x++;
	}
	++p->line;
      eof:
	if (x != w)
		done = eraeol(t, x, y, BG_COLOR(bg_text));
	else
		done = 0;

/* Set p to bp/amnt */
      bye:
	if (bp - p->ptr <= p->hdr->hole)
		p->ofst = bp - p->ptr;
	else
		p->ofst = bp - p->ptr - (p->hdr->ehole - p->hdr->hole);
	p->byte = byte;
	return done;

      eosl:
	if (bp - p->ptr <= p->hdr->hole)
		p->ofst = bp - p->ptr;
	else
		p->ofst = bp - p->ptr - (p->hdr->ehole - p->hdr->hole);
	p->byte = byte;
	pnextl(p);
	return 0;
}

static void gennum(BW *bw, int *screen, int *attr, SCRN *t, int y, int *comp)
{
	unsigned char buf[12];
	int z;
	int lin = bw->top->line + y - bw->y;

	if (lin <= bw->b->eof->line)
		joe_snprintf_1(buf, sizeof(buf), "%9ld ", bw->top->line + y - bw->y + 1);
	else {
		int x;
		for (x = 0; x != LINCOLS; ++x)
			buf[x] = ' ';
		buf[x] = 0;
	}
	for (z = 0; buf[z]; ++z) {
		outatr(bw->b->o.charmap, t, screen + z, attr + z, z, y, buf[z], BG_COLOR(bg_text)); 
		if (ifhave)
			return;
		comp[z] = buf[z];
	}
}

void bwgenh(BW *bw)
{
	int *screen;
	int *attr;
	P *q = pdup(bw->top, USTR "bwgenh");
	int bot = bw->h + bw->y;
	int y;
	SCRN *t = bw->t->t;
	int flg = 0;
	long from;
	long to;
	int dosquare = 0;

	from = to = 0;

	if (markv(0) && markk->b == bw->b)
		if (square) {
			from = markb->xcol;
			to = markk->xcol;
			dosquare = 1;
		} else {
			from = markb->byte;
			to = markk->byte;
		}
	else if (marking && bw == (BW *)maint->curwin->object && markb && markb->b == bw->b && bw->cursor->byte != markb->byte && !from) {
		if (square) {
			from = long_min(bw->cursor->xcol, markb->xcol);
			to = long_max(bw->cursor->xcol, markb->xcol);
			dosquare = 1;
		} else {
			from = long_min(bw->cursor->byte, markb->byte);
			to = long_max(bw->cursor->byte, markb->byte);
		}
	}

	if (marking && bw == (BW *)maint->curwin->object)
		msetI(t->updtab + bw->y, 1, bw->h);

	if (dosquare) {
		from = 0;
		to = 0;
	}

	y=bw->y;
	attr = t->attr + y*bw->t->w;
	for (screen = t->scrn + y * bw->t->w; y != bot; ++y, (screen += bw->t->w), (attr += bw->t->w)) {
		unsigned char txt[80];
		int fmt[80];
		unsigned char bf[16];
		int x;
		memset(txt,' ',76);
		msetI(fmt,BG_COLOR(bg_text),76);
		txt[76]=0;
		if (!flg) {
#if SIZEOF_LONG_LONG && SIZEOF_LONG_LONG == SIZEOF_OFF_T
			sprintf((char *)bf,"%8llx ",q->byte);
#else
			sprintf((char *)bf,"%8lx ",q->byte);
#endif
			memcpy(txt,bf,9);
			for (x=0; x!=8; ++x) {
				int c;
				if (q->byte==bw->cursor->byte && !flg) {
					fmt[10+x*3] |= INVERSE;
					fmt[10+x*3+1] |= INVERSE;
				}
				if (q->byte>=from && q->byte<to && !flg) {
					fmt[10+x*3] |= UNDERLINE;
					fmt[10+x*3+1] |= UNDERLINE;
					fmt[60+x] |= INVERSE;
				}
				c = pgetb(q);
				if (c >= 0) {
					sprintf((char *)bf,"%2.2x",c);
					txt[10+x*3] = bf[0];
					txt[10+x*3+1] = bf[1];
					if (c >= 0x20 && c <= 0x7E)
						txt[60+x] = c;
					else
						txt[60+x] = '.';
				} else
					flg = 1;
			}
			for (x=8; x!=16; ++x) {
				int c;
				if (q->byte==bw->cursor->byte && !flg) {
					fmt[11+x*3] |= INVERSE;
					fmt[11+x*3+1] |= INVERSE;
				}
				if (q->byte>=from && q->byte<to && !flg) {
					fmt[11+x*3] |= UNDERLINE;
					fmt[11+x*3+1] |= UNDERLINE;
					fmt[60+x] |= INVERSE;
				}
				c = pgetb(q);
				if (c >= 0) {
					sprintf((char *)bf,"%2.2x",c);
					txt[11+x*3] = bf[0];
					txt[11+x*3+1] = bf[1];
					if (c >= 0x20 && c <= 0x7E)
						txt[60+x] = c;
					else
						txt[60+x] = '.';
				} else
					flg = 1;
			}
		}
		genfield(t, screen, attr, 0, y, bw->offset, txt, 76, 0, bw->w, 1, fmt);
	}
	prm(q);
}

static void gen_lines(BW *bw, int y, int yend, const int linums, const int dosquare, const int fromto_modifiers, const long fromline, const long from, const long toline, const long to) {
	int *attr;
	int *screen;
	long from0, to0;
	P *p = NULL;
	SCRN *t = bw->t->t;

	y += bw->y;
	yend += bw->y;
	attr = t->attr + y * bw->t->w;
	screen = t->scrn + y * bw->t->w;
	for (; y < yend; ++y, (screen += bw->t->w), (attr += bw->t->w)) {
		if (ifhave && !linums)
			break;
		if (linums)
			gennum(bw, screen, attr, t, y, t->compose);
		if (t->updtab[y]) {
			p = bwgetto(p, bw->cursor, bw->top, bw->top->line + y - bw->y);
			if (!dosquare || (bw->top->line + y - bw->y >= fromline && bw->top->line + y - bw->y <= toline)) {
				from0 = from; to0 = to;
			} else {
				from0 = to0 = 0;
			}
			t->updtab[y] = lgen(t, y, screen, attr, bw->x, bw->x + bw->w, p, bw->offset, from0, to0, get_highlight_state(bw,p,bw->top->line+y-bw->y), bw, fromto_modifiers);
		}
	}
	if (p != NULL)
		prm(p);
}

void bwgen(BW *bw, int linums)
{
	int dosquare = 0;
	long from, to;
	long fromline, toline;
	P *error_srcp;
	int fromto_modifiers = INVERSE;

	/* Set w.db to correct value */
	if (bw->o.highlight && bw->o.syntax && (!bw->db || bw->db->syn != bw->o.syntax))
		bw->db = find_lattr_db(bw->b, bw->o.syntax);

	fromline = toline = from = to = 0;

	if (bw->b == errbuf &&
	    (!markb || !markk || markb->b != errbuf || markk->b != errbuf ||
	     markb->byte >= markk->byte) &&
	    NULL != (error_srcp = get_error_srcp())) {
		P *tmp = pdup(error_srcp, USTR "error_srcp_copy");
		p_goto_bol(tmp);
		from = tmp->byte;
		pnextl(tmp);
		to = tmp->byte;
		prm(tmp);
		fromto_modifiers = BOLD;
	} else if (markv(0) && markk->b == bw->b)
		if (square) {
			from = markb->xcol;
			to = markk->xcol;
			dosquare = 1;
			fromline = markb->line;
			toline = markk->line;
		} else {
			from = markb->byte;
			to = markk->byte;
		}
	else if (marking && bw == (BW *)maint->curwin->object && markb && markb->b == bw->b && bw->cursor->byte != markb->byte && !from) {
		if (square) {
			from = long_min(bw->cursor->xcol, markb->xcol);
			to = long_max(bw->cursor->xcol, markb->xcol);
			fromline = long_min(bw->cursor->line, markb->line);
			toline = long_max(bw->cursor->line, markb->line);
			dosquare = 1;
		} else {
			from = long_min(bw->cursor->byte, markb->byte);
			to = long_max(bw->cursor->byte, markb->byte);
		}
	}

	if (marking && bw == (BW *)maint->curwin->object)
		msetI(bw->t->t->updtab + bw->y, 1, bw->h);

	/* Generate from cursor to bottom of bw first, and then from top of bw to cursor. */
	gen_lines(bw, bw->cursor->line - bw->top->line, bw->h, linums, dosquare, fromto_modifiers, fromline, from, toline, to);
	gen_lines(bw, 0, bw->cursor->line - bw->top->line,     linums, dosquare, fromto_modifiers, fromline, from, toline, to);
}

void bwmove(BW *bw, int x, int y)
{
	bw->x = x;
	bw->y = y;
}

void bwresz(BW *bw, int wi, int he)
{
	if (he > bw->h && bw->y != -1) {
		msetI(bw->t->t->updtab + bw->y + bw->h, 1, he - bw->h);
	}
	bw->w = wi;
	bw->h = he;
}

BW *bwmk(W *window, B *b, int prompt)
{
	BW *bw = (BW *) joe_malloc(sizeof(BW));

	bw->parent = window;
	bw->b = b;
	if (prompt || (!window->y && staen)) {
		bw->y = window->y;
		bw->h = window->h;
	} else {
		bw->y = window->y + 1;
		bw->h = window->h - 1;
	}
	if (b->oldcur) {
		bw->top = b->oldtop;
		b->oldtop = NULL;
		bw->top->owner = NULL;
		bw->cursor = b->oldcur;
		b->oldcur = NULL;
		bw->cursor->owner = NULL;
	} else {
		bw->top = pdup(b->bof, USTR "bwmk");
		bw->cursor = pdup(b->bof, USTR "bwmk");
	}
	bw->t = window->t;
	bw->object = NULL;
	bw->offset = 0;
	bw->o = bw->b->o;
	if (bw->o.linums) {
		bw->x = window->x + LINCOLS;
		bw->w = window->w - LINCOLS;
	} else {
		bw->x = window->x;
		bw->w = window->w;
	}
	if (window == window->main) {
		rmkbd(window->kbd);
		window->kbd = mkkbd(kmap_getcontext(bw->o.context));
	}
	bw->top->xcol = 0;
	bw->cursor->xcol = 0;
	bw->linums = 0;
	bw->top_changed = 1;
	bw->linums = 0;
	bw->db = 0;
	return bw;
}

/* Database of last file positions */

#define MAX_FILE_POS 20 /* Maximum number of file positions we track */

static struct file_pos {
	LINK(struct file_pos) link;
	unsigned char *name;
	long line;
} file_pos = { { &file_pos, &file_pos }, 0, 0 };

static int file_pos_count;

struct file_pos *find_file_pos(unsigned char *name)
{
	struct file_pos *p;
	for (p = file_pos.link.next; p != &file_pos; p = p->link.next)
		if (!zcmp(p->name, name)) {
			promote(struct file_pos,link,&file_pos,p);
			return p;
		}
	p = (struct file_pos *)malloc(sizeof(struct file_pos));
	p->name = zdup(name);
	p->line = 0;
	enquef(struct file_pos,link,&file_pos,p);
	if (++file_pos_count == MAX_FILE_POS) {
		free(deque_f(struct file_pos,link,file_pos.link.prev));
		--file_pos_count;
	}
	return p;
}

int restore_file_pos;

long get_file_pos(unsigned char *name)
{
	if (name && restore_file_pos) {
		struct file_pos *p = find_file_pos(name);
		return p->line;
	} else {
		return 0;
	}
}

void set_file_pos(unsigned char *name, long pos)
{
	if (name) {
		struct file_pos *p = find_file_pos(name);
		p->line = pos;
	}
}

void save_file_pos(FILE *f)
{
	struct file_pos *p;
	for (p = file_pos.link.prev; p != &file_pos; p = p->link.prev) {
		fprintf(f,"	%ld ",p->line);
		emit_string(f,p->name,zlen(p->name));
		fprintf(f,"\n");
	}
	fprintf(f,"done\n");
}

void load_file_pos(FILE *f)
{
	unsigned char buf[1024];
	while (fgets((char *)buf,sizeof(buf)-1,f) && zcmp(buf,USTR "done\n")) {
		unsigned char *p = buf;
		long pos;
		unsigned char name[1024];
		parse_ws(&p,'#');
		if (!parse_long(&p, &pos)) {
			parse_ws(&p, '#');
			if (parse_string(&p, name, sizeof(name)) > 0) {
				set_file_pos(name, pos);
			}
		}
	}
}

/* Save file position for all windows */

void set_file_pos_all(Screen *t)
{
	/* Step through all windows */
	W *wx = t->topwin;
	do {
		if (wx->watom == &watomtw) {
			BW *bw = wx->object;
			set_file_pos(bw->b->name, bw->cursor->line);
		}
		wx = wx->link.next;
	} while(wx != t->topwin);
	/* Set through orphaned buffers */
	set_file_pos_orphaned();
}

void bwrm(BW *bw)
{
	if (bw->b == errbuf && bw->b->count == 1) {
		/* Do not lose message buffer */
		orphit(bw);
	}
	set_file_pos(bw->b->name,bw->cursor->line);
	prm(bw->top);
	prm(bw->cursor);
	brm(bw->b);
	joe_free(bw);
}

int ustat(BW *bw)
{
	static unsigned char buf[160];
	unsigned char bf1[100];
	unsigned char bf2[100];
	P pbak;
	int c;
	int bytelen;

	pbak = *bw->cursor;
	c = pgetc(bw->cursor);
	bytelen = bw->cursor->byte - pbak.byte;
	*bw->cursor = pbak;

#if SIZEOF_LONG_LONG && SIZEOF_LONG_LONG == SIZEOF_OFF_T
		joe_snprintf_1(bf1, sizeof(bf1), "%lld", bw->cursor->byte);
		joe_snprintf_1(bf2, sizeof(bf2), "%llx", bw->cursor->byte);
#else
		joe_snprintf_1(bf1, sizeof(bf1), "%ld", bw->cursor->byte);
		joe_snprintf_1(bf2, sizeof(bf2), "%lx", bw->cursor->byte);
#endif

	if (c == NO_MORE_DATA)
		joe_snprintf_4(buf, sizeof(buf), joe_gettext(_("** Line %ld  Col %ld  Offset %s(0x%s) **")), bw->cursor->line + 1, piscol(bw->cursor) + 1, bf1, bf2);
	else
		joe_snprintf_10(buf, sizeof(buf), joe_gettext(_("** Line %ld  Col %ld  Offset %s(0x%s)  %s %d(0%o/0x%X) Width %d/%d **")), bw->cursor->line + 1, piscol(bw->cursor) + 1, bf1, bf2, bw->b->o.charmap->name, c, c, c, joe_wcwidth(bw->o.charmap->type,c), bytelen);
	msgnw(bw->parent, buf);
	return 0;
}

int ucrawlr(BW *bw)
{
	int amnt = bw->w / 2;

	pcol(bw->cursor, bw->cursor->xcol + amnt);
	bw->cursor->xcol += amnt;
	bw->offset += amnt;
	updall();
	return 0;
}

int ucrawll(BW *bw)
{
	int amnt = bw->w / 2;
	int curamnt = bw->w / 2;

	if (amnt > bw->offset) {
		amnt = bw->offset;
		curamnt = bw->offset;
	}
	if (!bw->offset)
		curamnt = bw->cursor->xcol;
	if (!curamnt)
		return -1;
	pcol(bw->cursor, bw->cursor->xcol - curamnt);
	bw->cursor->xcol -= curamnt;
	bw->offset -= amnt;
	updall();
	return 0;
}

/* If we are about to call bwrm, and b->count is 1, and orphan mode
 * is set, call this. */

void orphit(BW *bw)
{
	++bw->b->count; /* Assumes bwrm() is about to be called. */
	bw->b->orphan = 1;
	pdupown(bw->cursor, &bw->b->oldcur, USTR "orphit");
	pdupown(bw->top, &bw->b->oldtop, USTR "orphit");
}
