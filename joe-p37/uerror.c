/*
 *	Compiler error handler
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

/* Error database */

typedef struct error ERROR;

struct error {
	LINK(ERROR) link;	/* Linked list of errors */
	long line;		/* Target line number */
	long org;		/* Original target line number */
	unsigned char *file;	/* VS. Target file name */
	long src;		/* Error-file line number */
	unsigned char *msg;	/* The message */
} errors = { { &errors, &errors}, 0, 0, 0, 0, 0 };
ERROR *errptr = &errors;	/* Current error row */

B *errbuf = NULL;
P *errp = NULL;

P *get_error_srcp(void) {
	return errptr == &errors ? NULL : errp;  /* errp can also be NULL. */
}  

/* Function which allows stepping through all error buffers,
   for multi-file search and replace.  Give it a buffer.  It finds next
   buffer in error list.  Look at 'berror' for error information. */

/* This is made to work like bafter: it does not increment refcount of buffer */

B *beafter(B *b)
{
	ERROR *e;
	unsigned char *name = b->name;
	if (!name) name = USTR "";
	for (e = errors.link.next; e != &errors; e = e->link.next)
		if (!zcmp(name, e->file))
			break;
	if (e == &errors) {
		/* Given buffer is not in list?  Return first buffer in list. */
		e = errors.link.next;
	}
	while (e != &errors && !zcmp(name, e->file))
		e = e->link.next;
	berror = 0;
	if (e != &errors) {
		B *b = bfind_incref(e->file);
		/* bfind_incref() bumps refcount, so we have to unbump it. */
		if (b->count == 1)
			b->orphan = 1; /* Oops */
		else
			--b->count;
		return b;
	}
	return 0;
}

/* Insert and delete notices */

void inserr(unsigned char *name, long int where, long int n, int bol)
{
	ERROR *e;

	if (!n)
		return;

	if (name) {
		for (e = errors.link.next; e != &errors; e = e->link.next) {
			if (!zcmp(e->file, name)) {
				if (e->line > where)
					e->line += n;
				else if (e->line == where && bol)
					e->line += n;
			}
		}
	}
}

void delerr(unsigned char *name, long int where, long int n)
{
	ERROR *e;

	if (!n)
		return;

	if (name) {
		for (e = errors.link.next; e != &errors; e = e->link.next) {
			if (!zcmp(e->file, name)) {
				if (e->line > where + n)
					e->line -= n;
				else if (e->line > where)
					e->line = where;
			}
		}
	}
}

/* Abort notice */

void abrerr(unsigned char *name)
{
	ERROR *e;

	if (name)
		for (e = errors.link.next; e != &errors; e = e->link.next)
			if (!zcmp(e->file, name))
				e->line = e->org;
}

/* Save notice */

void saverr(unsigned char *name)
{
	ERROR *e;

	if (name)
		for (e = errors.link.next; e != &errors; e = e->link.next)
			if (!zcmp(e->file, name))
				e->org = e->line;
}

/* Pool of free error nodes */
ERROR errnodes = { {&errnodes, &errnodes}, 0, 0, 0, 0, 0 };

/* Free an error node */

static void freeerr(ERROR *n)
{
	vsrm(n->file);
	vsrm(n->msg);
	enquef(ERROR, link, &errnodes, n);
}

/* Free all errors */

static void freeall(void)
{
	while (!qempty(ERROR, link, &errors))
		freeerr(deque_f(ERROR, link, errors.link.next));
	errptr = &errors;
}

/* Parse error messages into database */

/* From joe's joe 2.9 */

/* First word (allowing ., /, _ and -) with a . is the file name.  Next number
   is line number.  Then there should be a ':' */

static void parseone(struct charmap *map,unsigned char *s,unsigned char **rtn_name,long *rtn_line)
{
	int x, y, flg;
	unsigned char *name = NULL;
	long line = -1;

	y=0;
	flg=0;

	do {
		/* Skip to first word */
		for (x = y; s[x] && !(joe_isalnum_(map,s[x]) || s[x] == '.' || s[x] == '/'); ++x) ;

		/* Skip to end of first word */
		for (y = x; joe_isalnum_(map,s[y]) || s[y] == '.' || s[y] == '/' || s[y]=='-'; ++y)
			if (s[y] == '.')
				flg = 1;
	} while (!flg && x!=y);

	/* Save file name */
	if (x != y)
		name = vsncpy(NULL, 0, s + x, y - x);

	/* Skip to first number */
	for (x = y; s[x] && (s[x] < '0' || s[x] > '9'); ++x) ;

	/* Skip to end of first number */
	for (y = x; s[y] >= '0' && s[y] <= '9'; ++y) ;

	/* Save line number */
	if (x != y)
		sscanf((char *)(s + x), "%ld", &line);
	if (line != -1)
		--line;

	/* Look for ':' */
	flg = 0;
	while (s[y]) {
	/* Allow : anywhere on line: works for MIPS C compiler */
/*
	for (y = 0; s[y];)
*/
		if (s[y]==':') {
			flg = 1;
			break;
		}
		++y;
	}

	if (!flg)
		line = -1;

	*rtn_name = name;
	*rtn_line = line;
}

/* Parser for file name lists from grep, find and ls.
 *
 * filename
 * filename:*
 * filename:line-number:*
 */

void parseone_grep(struct charmap *map,unsigned char *s,unsigned char **rtn_name,long *rtn_line)
{
	int y;
	unsigned char *name = NULL;
	long line = -1;

	/* Skip to first : or end of line */
	for (y = 0;s[y] && s[y] != ':';++y);
	if (y) {
		/* This should be the file name */
		name = vsncpy(NULL,0,s,y);
		line = 0;
		if (s[y] == ':') {
			/* Maybe there's a line number */
			++y;
			while (s[y] >= '0' && s[y] <= '9')
				line = line * 10 + (s[y++] - '0');
			--line;
			if (line < 0 || s[y] != ':') {
				/* Line number is only valid if there's a second : */
				line = 0;
			}
		}
	}

	*rtn_name = name;
	*rtn_line = line;
}

static int parseit(struct charmap *map,unsigned char *s, long int row,
  void (*parseone)(struct charmap *map, unsigned char *s, unsigned char **rtn_name, long *rtn_line))
{
	unsigned char *name = NULL;  /* VS */
	long line = -1;
	ERROR *err;

	parseone(map,s,&name,&line);

	if (name) {
		if (line != -1) {
			/* We have an error */
			err = (ERROR *) alitem(&errnodes, sizeof(ERROR));
			err->file = name;
			err->org = err->line = line;
			err->src = row;
			err->msg = vsncpy(NULL, 0, sc("\\i"));
			err->msg = vsncpy(sv(err->msg), sv(s));
			enqueb(ERROR, link, &errors, err);
			return 1;
		} else
			vsrm(name);
	}
	return 0;
}

/* Parse the error output contained in a buffer */

static long parserr(B *b)
{
	P *p = pdup(b->bof, USTR "parserr");
	P *q = pdup(p, USTR "parserr");
	long nerrs = 0;

	if (b->scratch && b->name[0] == '*') {
		/* "* Build Log *", "* Grep Log *" */
	 	b->changed = 0;
	}
	freeall();
	if (errbuf != b) {
		if (errp != NULL) {
			prm(errp);
			errp = NULL;
		}
		errbuf = b;
	}
	do {
		unsigned char *s;

		pset(q, p);
		p_goto_eol(p);
		s = brvs(q, (int) (p->byte - q->byte));
		if (s) {
			nerrs += parseit(b->o.charmap, s, q->line, (b->parseone ? b->parseone : parseone));
			vsrm(s);
		}
	} while (pgetc(p) != NO_MORE_DATA);
	prm(p);
	prm(q);
	return nerrs;
}

BW *find_a_good_bw(B *b)
{
	W *w;
	BW *bw = 0;
	/* Find lowest window with buffer */
	if ((w = maint->topwin) != NULL) {
		do {
			if ((w->watom->what&TYPETW) && ((BW *)w->object)->b==b && w->y>=0)
				bw = (BW *)w->object;
			w = w->link.next;
		} while (w != maint->topwin);
	}
	if (bw)
		return bw;
	/* Otherwise just find lowest window */
	if ((w = maint->topwin) != NULL) {
		do {
			if ((w->watom->what&TYPETW) && w->y>=0)
				bw = (BW *)w->object;
			w = w->link.next;
		} while (w != maint->topwin);
	}
	return bw;
}

static void show_messages_found_count(BW *bw, int n) {
	if (n)
		joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("%d messages found")), n);
	else
		joe_snprintf_0(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("No messages found")));
	msgnw(bw->parent, msgbuf);
}

void parserrb(B *b)
{
	BW *bw;
	bw = find_a_good_bw(b);
	show_messages_found_count(bw, parserr(bw->b));
}

int uparserr(BW *bw)
{
	show_messages_found_count(bw, parserr(bw->b));
	return 0;
}

int jump_to_file_line(BW *bw,unsigned char *file,int line,unsigned char *msg)
{
	int omid;
	if (!bw->b->name || zcmp(file, bw->b->name)) {
		if (doswitch(bw, vsdup(file), NULL, NULL))
			return -1;
		bw = (BW *) maint->curwin->object;
	}
	omid = mid;
	mid = 1;
	pline(bw->cursor, line);
	dofollows();
	mid = omid;
	bw->cursor->xcol = piscol(bw->cursor);
	msgnw(bw->parent, msg);
	return 0;
}

/* Show current message */

int ucurrent_msg(BW *bw)
{
	if (errptr != &errors) {
		msgnw(bw->parent, errptr->msg);
		return 0;
	} else {
		msgnw(bw->parent, joe_gettext(_("No messages")));
		return -1;
	}
}

/* Find line in error database: return pointer to message */

static ERROR *srcherr(unsigned char *file, long line)
{
	ERROR *p;
	for (p = errors.link.next; p != &errors; p=p->link.next) {
		if (!zcmp(p->file,file) && p->org == line) {
			return p;
		}
	}
	return NULL;
}

int ujump(BW *bw)
{
	int rtn = -1;
	P *p = pdup(bw->cursor, USTR "ujump");
	P *q = pdup(p, USTR "ujump");
	unsigned char *s;
	p_goto_bol(p);
	p_goto_eol(q);
	s = brvs(p, (int) (q->byte - p->byte));
	prm(p);
	prm(q);
	if (s) {
		unsigned char *name = NULL;
		long line = -1;
		if (bw->b->parseone)
			bw->b->parseone(bw->b->o.charmap,s,&name,&line);
		else
			parseone(bw->b->o.charmap,s,&name,&line);
		if (name && line != -1) {
			ERROR *p = srcherr(name, line);
			if (p != NULL) {
				errptr = p;
				setline(errbuf, errptr->src);
				name = p->file;
				line = p->line;
			}
			uprevvw((BASE *)bw);
			rtn = jump_to_file_line(maint->curwin->object, name, line, NULL /* errptr->msg */);
			vsrm(name);
		}
		vsrm(s);
	}
	return rtn;
}

static int jump_to_error(BW *bw, ERROR *new_errptr) {
	W *w;
	if (new_errptr == NULL) {
		msgnw(bw->parent, joe_gettext(_("No more errors")));
		return -1;
	}
	errptr = new_errptr;
	/* This moves the cursor to the beginning of line errptr->src,
	 * and it also scrolls the window.
	 */
	setline(errbuf, errptr->src);

	/* Set errp to the beginning of line in errbuf. */
	w = maint->curwin;
	do {
		if (w->watom->what == TYPETW) {
			BW *bw2 = w->object;
			if (bw2->b == errbuf) {
				if (errp != NULL) {
					pset(errp, bw2->cursor);
				} else {
					errp = pdup(bw2->cursor, USTR "errp");
				}
				pgetc(errp);  /* Move to next char to let the user type above. */
				if (piseol(errp))  /* Undo the move for empty lines. */
				  p_goto_bol(errp);
				break;
			}
		}
	} while ((w = w->link.next) != maint->curwin);

	if (bw->b == errbuf) {
		uprevvw((BASE *)bw);
		bw = (BW*) maint->curwin->object;
	}
	return jump_to_file_line(bw,errptr->file,errptr->line,NULL /* errptr->msg */);
}

/* Reparse errbuf to errors if errbuf is a scratch buffer and it has changed
 * since the last parse.
 */
static void reparserr(void) {
	if (errbuf == NULL || !errbuf->scratch || !errbuf->changed ||
	    errbuf->name[0] != '*') {
		return;
	}
	if (errptr != &errors) {  /* Keep the original error position. */
		unsigned char *name = vsncpy(NULL, 0, sv(errptr->file));
		long line = errptr->line;
		ERROR *p;
		parserr(errbuf);
		if (NULL != (p = srcherr(name, line))) {
			errptr = p;
			setline(errbuf, errptr->src);
		}
		vsrm(name);
	} else {
		parserr(errbuf);
	}
}

/* Makes the current buffer the error buffer, possibly reparsing errors (if
 * it was a scratch buffer). Does not show the message count.
 */
int uthiserrbuf(BW *bw)
{
	if (errbuf != bw->b) {
		errbuf = bw->b;
		freeall();
	}
	reparserr();
	return 0;
}

int unxterr(BW *bw)
{
	ERROR *new_errptr;
	reparserr();
	new_errptr = errptr->link.next;
	return jump_to_error(bw, new_errptr == &errors ? NULL : new_errptr);
}

int uprverr(BW *bw)
{
	ERROR *new_errptr;
	reparserr();
	new_errptr = errptr->link.prev;
	return jump_to_error(bw, new_errptr == &errors ? NULL : new_errptr);
}
