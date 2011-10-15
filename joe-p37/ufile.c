/*
 * 	User file operations
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

#include <sys/time.h>

struct my_utimbuf {
  time_t actime;            /* Access time.  */
  time_t modtime;           /* Modification time.  */
};

static int my_utime(const char *file, const struct my_utimbuf *times) {
	struct timeval timevals[2];

	if (times != NULL) {
		timevals[0].tv_usec = 0L;
		timevals[1].tv_usec = 0L;
		timevals[0].tv_sec = (long int) times->actime;
		timevals[1].tv_sec = (long int) times->modtime;
	}
	return utimes(file, times ? timevals : NULL);
}

/* for utime.h in /usr/src/uClibc-0.9.31/linux/utime.h */
#undef  libc_hidden_proto
#undef  libc_hidden_proto
#define libc_hidden_proto(x)

#ifdef HAVE_UTIME_H
#include <utime.h>
#else
#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#endif
#endif

#ifdef WITH_SELINUX
int copy_security_context(const char *from_file, const char *to_file);
#endif

int orphan;
unsigned char *backpath = NULL;		/* Place to store backup files */
B *filehist = NULL;	/* History of file names */
int nobackups = 0;
int exask = 0;

/* Ending message generator */
/**** message which is shown after closing joe (CTRL+x; CTRL+k) *****/
void genexmsg(BW *bw, int saved, unsigned char *name)
{
	unsigned char *s;

	if (bw->b->name && bw->b->name[0]) {
		s = bw->b->name;
	} else {
		s = joe_gettext(_("(Unnamed)"));
	}

	if (name) {
		if (saved) {
			joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("File %s saved")), name);
		} else {
			joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("File %s not saved")), name);
		}
	} else if (bw->b->changed && bw->b->count == 1) {
		joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("File %s not saved")), s);
	} else if (saved) {
		joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("File %s saved")), s);
	} else {
		joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("File %s not changed so no update needed")), s);
	}
	if (exmsg)
		vsrm(exmsg);

	exmsg = vsncpy(NULL,0,sz(msgbuf));
	msgnw(bw->parent, msgbuf);
}

/* For ^X ^C */
void genexmsgmulti(BW *bw, int saved, int skipped)
{
	if (saved)
		if (skipped)
			joe_snprintf_0(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("Some files have not been saved.")));
		else
			joe_snprintf_0(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("All modified files have been saved.")));
	else
		joe_snprintf_0(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("No modified files, so no updates needed.")));

	msgnw(bw->parent, msgbuf);

	exmsg = vsncpy(NULL,0,sz(msgbuf));
}


/* Shell escape */

int ushell(BW *bw)
{
	nescape(bw->parent->t->t);
	ttsusp();
	nreturn(bw->parent->t->t);
	return 0;
}

/* Execute shell command */

static int dosys(BW *bw, unsigned char *s, void *object, int *notify)
{
	int rtn;

	nescape(bw->parent->t->t);
	rtn=ttshell(s);
	nreturn(bw->parent->t->t);

	if (notify)
		*notify = 1;
	vsrm(s);
	return rtn;
}

int usys(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("System (^C to abort): ")), NULL, dosys, NULL, NULL, NULL, NULL, NULL, bw->b->o.charmap, 1))
		return 0;
	else
		return -1;
}

/* Copy a file */

static int cp(unsigned char *from, unsigned char *to)
{
	int f, g, amnt;
	struct stat sbuf;

#ifdef HAVE_UTIME
#ifdef NeXT
	time_t utbuf[2];
#else
	struct my_utimbuf utbuf;
#endif
#endif

	f = open((char *)from, O_RDONLY);
	if (f < 0) {
		return -1;
	}
	if (fstat(f, &sbuf) < 0) {
		return -1;
	}
	g = creat((char *)to, sbuf.st_mode & ~(S_ISUID | S_ISGID));
	if (g < 0) {
		close(f);
		return -1;
	}
	while ((amnt = read(f, stdbuf, stdsiz)) > 0) {
		if (amnt != joe_write(g, stdbuf, amnt)) {
			break;
		}
	}
	close(f);
	close(g);
	if (amnt) {
		return -1;
	}

#ifdef HAVE_UTIME
#ifdef NeXT
	utbuf[0] = (time_t) sbuf.st_atime;
	utbuf[1] = (time_t) sbuf.st_mtime;
#else
	utbuf.actime = sbuf.st_atime;
	utbuf.modtime = sbuf.st_mtime;
#endif
	my_utime((char *)to, &utbuf);
#endif

#ifdef WITH_SELINUX
	copy_security_context(from,to);
#endif

	return 0;
}

/* Make backup file if it needs to be made
 * Returns 0 if backup file was made or didn't need to be made
 * Returns 1 for error
 */

static int backup(BW *bw)
{
	if (!bw->b->backup && !nobackups && !bw->o.nobackup && bw->b->name && bw->b->name[0]) {
		unsigned char tmp[1024];
		unsigned char name[1024];

#ifdef __MSDOS__
		int x;

		if (backpath) {
			unsigned char *t = vsncpy(NULL,0,sz(backpath));
			t = canonical(t);
			joe_snprintf_2(name, sizeof(name), "%s/%s", t, namepart(tmp, bw->b->name));
			vsrm(t);
		} else {
			joe_snprintf_1(name, sizeof(name), "%s", bw->b->name);
		}

		for (x = zlen(name); name[--x] != '.';) {
			if (name[x] == '\\' || (name[x] == ':' && x == 1) || x == 0) {
				x = zlen(name);
				break;
			}
		}

		zcpy(name + x, USTR ".bak");

#else

		/* Create backup file name */
		unsigned char *simple_backup_suffix = (unsigned char *)getenv("SIMPLE_BACKUP_SUFFIX");
		
		if (simple_backup_suffix == NULL) {
			simple_backup_suffix = USTR "~";
		}
		if (backpath) {
			unsigned char *t = vsncpy(NULL, 0, sz(backpath));
			t = canonical(t);
			joe_snprintf_3(name, sizeof(name), "%s/%s%s", t, namepart(tmp, dequote(bw->b->name)), simple_backup_suffix);
			vsrm(t);
		} else {
			joe_snprintf_2(name, sizeof(name), "%s%s", dequote(bw->b->name), simple_backup_suffix);
		}
		
		/* Attempt to delete backup file first */
		unlink((char *)name);

#endif

		/* Copy original file to backup file */
		if (cp(dequote(bw->b->name), name)) {
			return 1;
		} else {
			bw->b->backup = 1;
			return 0;
		}
	} else {
		return 0;
	}
}

/* Write file */

/* Continuation structure */

struct savereq {
	int (*callback) ();
	unsigned char *name;
	B *first;
	int not_saved;	/* Set if a modified file was not saved */
	int rename;	/* Set if we're renaming the file during save */
	int block_save; /* Flag, if we want to save a block# */
	unsigned char *message; /* String for messages to be shown to the user */
};

struct savereq *mksavereq(int (*callback)(), unsigned char *name, B *first,int rename, int block_save)
{
	struct savereq *req = (struct savereq *) joe_malloc(sizeof(struct savereq));
	req->callback = callback;
	req->name = name;
	req->first = first;
	req->not_saved = 0;
	req->rename = rename;
	req->block_save = block_save;
	return req;
}

static void rmsavereq(struct savereq *req)
{
	vsrm(req->name);
	joe_free(req);
}

/* Check if character 'c' is in the set.
 * 'c' should be unicode if the locale is UTF-8, otherwise it's
 * an 8-bit character.  'set' should be of this format: "xxxx<>yyyy".  xxxx
 * is a list of 8-bit characters. yyyy is a list of UTF-8 characters.
 */

unsigned char *yes_key = (unsigned char *) _("|yes|yY");
unsigned char *no_key = (unsigned char *) _("|no|nN");

int yncheck(unsigned char *key_set, int c)
{
	unsigned char *set = joe_gettext(key_set);
	if (locale_map->type) {
		/* 'c' is unicode */
		while (*set) {
			if (c == utf8_decode_fwrd(&set, NULL))
				return 1;
		}
		return 0;
	} else {
		/* 'c' is 8-bit */
		while (set[0]) {
			if (set[0] == c)
				return 1;
			++set;
		}
		return 0;
	}
}

int ynchecks(unsigned char *set, unsigned char *s)
{
	if (locale_map->type)
		return yncheck(set, utf8_decode_fwrd(&s, NULL));
	else
		return yncheck(set, s[0]);
}

static int saver(BW *bw, int c, struct savereq *req, int *notify)
{
	int fl;
	if (c == NO_CODE || yncheck(no_key, c)) {
		msgnw(bw->parent, joe_gettext(_("Couldn't make backup file... file not saved")));
		if (req->callback) {
			return req->callback(bw, req, -1, notify);
		} else {
			if (notify) {
				*notify = 1;
			}
			rmsavereq(req);
			return -1;
		}
	}
	if (c != YES_CODE && !yncheck(yes_key, c)) {
		if (mkqw(bw->parent, sz(joe_gettext(_("Could not make backup file.  Save anyway (y,n,^C)? "))), saver, NULL, req, notify)) {
			return 0;
		} else {
			rmsavereq(req);
			if (notify)
				*notify = 1;
			return -1;
		}
	}
	if (bw->b->er == -1 && bw->o.msnew) {
		exmacro(bw->o.msnew,1);
		bw->b->er = -3;
	}
	if (bw->b->er == 0 && bw->o.msold) {
		exmacro(bw->o.msold,1);
	}
	if ((fl = bsave(bw->b->bof, req->name, bw->b->eof->byte, req->rename ? 2 : 1)) != 0) {
		msgnw(bw->parent, joe_gettext(msgs[-fl]));
		if (req->callback) {
			return req->callback(bw, req, -1, notify);
		} else {
			rmsavereq(req);
			if (notify) {
				*notify = 1;
			}
			return -1;
		}
	} else {
		if (req->rename && req->name[0] != '!' && req->name[0] != '>') {
			bw_unlock(bw);
			joe_free(bw->b->name);
			bw->b->name = 0;
		}
		if (!bw->b->name && req->name[0]!='!' && req->name[0]!='>')
			bw->b->name = joesep(zdup(req->name));
		if (bw->b->name && !zcmp(bw->b->name, req->name)) {
			bw_unlock(bw);
			bw->b->changed = 0;
			saverr(bw->b->name);
		}
		{
			/* Last UNDOREC which wasn't modified will be changed
			 * to modified. And because this block is
			 * executed after each 'save', there can't be more
			 * than one record which is not modified
			 *		24 Apr 2001, Marx
			 */
			UNDO *u = bw->b->undo;
			UNDOREC *rec, *rec_start;

			rec = rec_start = &u->recs;

			do {
				rec = rec->link.prev;
			} while (rec != rec_start && rec->changed);
			if(rec->changed == 0)
				rec->changed = 1;

		}
		genexmsg(bw, 1, req->name);
		if (req->callback) {
			return req->callback(bw, req, 0, notify);
		} else {
			rmsavereq(req);
			return 0;
		}
	}
}

static int dosave(BW *bw, struct savereq *req, int *notify)
{
	if (req->block_save)
	{
		if (notify)
			*notify = 1;
		if (markv(1)) {
			if (square) {
				int fl;
				int ret = 0;
				B *tmp = pextrect(markb,
						  markk->line - markb->line + 1,
						  markk->xcol);
						  
				if ((fl = bsave(tmp->bof, req->name, tmp->eof->byte, 0)) != 0) {
					msgnw(bw->parent, joe_gettext(msgs[-fl]));
					ret = -1;
				}
				brm(tmp);
				if (!ret) {
					joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("Block written to file %s")), req->name);
					msgnw(bw->parent, msgbuf);
				}
				if (lightoff)
					unmark(bw);
				vsrm(req->name);
				return ret;
			} else {
				int fl;
				int ret = 0;

				if ((fl = bsave(markb, req->name, markk->byte - markb->byte, 0)) != 0) {
					msgnw(bw->parent, joe_gettext(msgs[-fl]));
					ret = -1;
				}
				if (!ret) {
					joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("Block written to file %s")), req->name);
					msgnw(bw->parent, msgbuf);
				}
				if (lightoff)
					unmark(bw);
					vsrm(req->name);
				return ret;
			}
		} else {
			vsrm(req->name);
			msgnw(bw->parent, joe_gettext(_("No block")));
			return -1;
		}
	}
	else
	{
		if (backup(bw)) {
			return saver(bw, 0, req, notify);
		} else {
			return saver(bw, YES_CODE, req, notify);
		}
	}
}

static int dosave2(BW *bw, int c, struct savereq *req, int *notify)
{
	if (c == YES_CODE || yncheck(yes_key, c)) {
		return dosave(bw, req, notify);
	} else if (c == NO_CODE || yncheck(no_key, c)) {
		if (notify) {
			*notify = 1;
		}
		genexmsg(bw, 0, req->name);
		rmsavereq(req);
		return -1;
	} else if (mkqw(bw->parent, sz(req->message), dosave2, NULL, req, notify)) {
		return 0;
	} else {
		/* Should be in abort function */
		rmsavereq(req);
		return -1;
	}
}

/* Checks if file exists. */

static int dosave1(BW *bw, unsigned char *s, struct savereq *req, int *notify)
{
	int f;

	if (req->name)
		vsrm(req->name);
	req->name = s;

	if (s[0] != '!' && !(s[0] == '>' && s[1] == '>')) {
		/* It's a normal file: not a pipe or append */
		if (!bw->b->name || zcmp(s, bw->b->name)) {
			/* Newly named file or name is different than buffer */
			f = open((char *)dequote(s), O_RDONLY);
			if (f != -1) {
				close(f);
				/* char *msg = "File exists. Overwrite (y,n,^C)? ";
				   req->message = msg; */
				req->message = joe_gettext(_("File exists. Overwrite (y,n,^C)? "));
				return dosave2(bw, 0, req, notify);
			}
		}
		else {
			/* We're saving a newer version of the same file */
			if (check_mod(bw->b)) {
				req->message = joe_gettext(_("File on disk is newer. Overwrite (y,n,^C)? "));
				return dosave2(bw, 0, req, notify);
			}
		}
	}

	return dosave(bw, req, notify);
}

int xsave(BW *bw, int rename)
{
	BW *pbw;
	
	pbw = wmkpw(bw->parent, joe_gettext(_("Name of file to save (^C to abort): ")), &filehist, dosave1, USTR "Names", NULL, cmplt,
	            mksavereq(NULL,NULL,NULL, rename, 0), NULL, locale_map, bw->b->name ? 1 : 7);

	if (pbw && bw->b->name) {
		binss(pbw->cursor, bw->b->name);
		pset(pbw->cursor, pbw->b->eof);
		pbw->cursor->xcol = piscol(pbw->cursor);
	}
	if (pbw) {
		return 0;
	} else {
		return -1;
	}
}

/* User command: ^K D */

int usave(BW *bw)
{
	return xsave(bw, /*rename:*/1);
}

int usaveother(BW *bw)
{
	return xsave(bw, /*rename:*/0);
}

int usavenow(BW *bw)
{
	if (bw->b->name) {
		return dosave1(bw,vsncpy(NULL,0,sz(bw->b->name)),mksavereq(NULL,NULL,NULL,0,0),NULL);
	} else
		return usave(bw);
}

/* Write highlighted block to a file */

int ublksave(BW *bw)
{
	if (markb && markk && markb->b == markk->b && (markk->byte - markb->byte) > 0 && (!square || piscol(markk) > piscol(markb))) {
		if (wmkpw(bw->parent, joe_gettext(_("Name of file to write (^C to abort): ")), &filehist, dosave1, USTR "Names", NULL, cmplt, mksavereq(NULL, NULL, NULL, 0, 1), NULL, locale_map, 3)) {
			return 0;
		} else {
			return -1;
		}
	} else {
		return usave(bw);
	}
}

static int count_onscreen_main_windows(Screen *t) {
	int nmain = 1;
	W *m = t->curwin->main;
	W *q;
	/* TODO(pts): Similar implementation, but no ->main in unextvw(). */
	for (q = t->curwin->link.next; q != t->curwin; q = q->link.next)
		if (q->main != m) {
			if ((q->watom->what&TYPETW) && q->y >= 0)
				++nmain;
			m = q->main;
		}
	return nmain;
}

static W* find_onscreen_window(B *b, Screen *t) {
	W *w0 = t->curwin;
	W *w = w0;
	while (1) {
		if (w->y >= 0 && (w->watom->what&TYPETW) &&
		    ((BW*)w->object)->b == b) return w;
		w = w->link.prev;
		if (w == w0) return NULL;
	}
}

static W* find_offscreen_window(B *b, Screen *t) {
	W *w0 = t->curwin;
	W *w = w0;
	while (1) {
		if (w->y < 0 && (w->watom->what&TYPETW) &&
		    ((BW*)w->object)->b == b) return w;
		w = w->link.prev;
		if (w == w0) return NULL;
	}
}

/* Swap w->bw with ow->bw. w must be an onscreen, and ow an offscreen window.
 * Return 0 on success, anything else on failure.
 */
static int swap_bw_with_offscreen_window(W *w, W *ow) {
	BW *bw, *obw;
	if (w->y < 0) return -1;
	if (ow->y >= 0) return -1;
	if (!(w->watom->what&TYPETW)) return -1;
	if (!(ow->watom->what&TYPETW)) return -1;
	bw = (BW*)w->object;
	obw = (BW*)ow->object;
	w->object = obw;
	ow->object = bw;
	bw->parent = ow;
	obw->parent = w;
	wfit(w->t); /* This seems to be needed */
	wredraw(w); /* This is also needed */
	/* dofollows(); */ /* Not needed -- or is if window heights differ? */
	return 0;
}

int replace_b_in_bw(BW *bw, B *b, int do_orphan, int do_restore, int use_berror, int do_macros) {
	W *w = bw->parent;
	W *ow;
	int ret = 0;

	if (bw->b == b) {
	      easy_return:
		if (use_berror) {
			msgnwt(w, joe_gettext(msgs[-use_berror]));
			if (use_berror != -1) {
				ret = -1;
			}
		}
		return ret;
	} else if ((ow = find_offscreen_window(b, w->t)) != NULL &&
            0 == swap_bw_with_offscreen_window(w, ow)) {
        	goto easy_return;
        }

        /* Ensure that the old buffer (bw->b) is in another BW in addition to
         * bw.
         */
	if (bw->b->count == 1 && (bw->b->changed || bw->b->name)) {
		if (do_orphan) {
			orphit(bw);
		} else {
			if (uduptw(bw)) {
				brm(b);
				return -1;
			}
			bw = (BW *) w->t->curwin->object; /* the new, replaced, onscreen bw */
			w = bw->parent;
		}
	}

	if (use_berror) {
		/* TODO(pts): Check this (i.e. permission denied opening file */
		msgnwt(w, joe_gettext(msgs[-use_berror]));
		if (use_berror != -1) {
			ret = -1;
		}
#if 0  /* b is a "New file" or "Error opening file" placeholder buffer. */
		brm(b);
		return ret;
#endif
	}

        /* Replace the contents of w with b, by creating a new BW. */
	{	void *object = bw->object;
		ow = find_onscreen_window(b, w->t);
		if (ow == NULL) ow = find_offscreen_window(b, w->t);
		bwrm(bw);  /* This includes --bw->b->count. */
		/* bwmk uses b->oldpos, which usually at the BOL for a b with
		 * no corresponding bw.
		 */
		w->object = (void *) (bw = bwmk(w, b, 0));
		if (!b->orphan) {
			++b->count;
		} else {
			b->orphan = 0;
		}
		if (ow != NULL && ow != w) {
			BW *obw = (BW*)ow->object;
			/* pset code copied from uduptw */
			pset(bw->top, obw->top);
			pset(bw->cursor, obw->cursor);
			bw->cursor->xcol = obw->cursor->xcol;
		}
		wredraw(w);
		bw->object = object; /* TODO(pts): Move this above wredraw(w)? */
	}

	if (do_macros && use_berror == -1 && bw->o.mnew) {
		exmacro(bw->o.mnew,1);
	}
	if (do_macros && use_berror == 0 && bw->o.mold) {
		exmacro(bw->o.mold,1);
	}
	if (do_restore && !bw->b->scratch) {
		int omid = mid;
		/* Restore cursor line */
		pline(bw->cursor, get_file_pos(bw->b->name));
		mid = 1;
		dofollows();
		mid = omid;
	}
	return ret;
}


/* Load file to edit */

#define IF_LOADED_REUSE_KEEP 0
#define IF_LOADED_REUSE_ADJUST 1
#define IF_LOADED_RELOAD_ADJUST 2

/**
 * Load filename filename to bw, notify and then vsrm(filename).
 * @param if_loaded_policy Specifies what happens if file is already loaded
 *   into a buffer. Possible values.
 *   IF_LOADED_REUSE_KEEP: use existing buffer, keep bnext()/bprev()
 *   IF_LOADED_REUSE_ADJUST: use existing buffer, ajdust bnext()/bprev()
 *   IF_LOADED_RELOAD_ADJUST: reload from file to buffer, adjust bnext()/bprev()
 */
static int doedit2(BW *bw, int if_loaded_policy, unsigned char *filename,
                   int *notify) {
  B *b;
  int ret;
  if (notify) *notify = 1;
  /* bfind_reload() and bfind() set berror, and always use a non-NULL b */
  if (if_loaded_policy == IF_LOADED_RELOAD_ADJUST) {
    b = bfind_reload(filename);
    /* Make the buffer holding the original file read-only to prevent
     * accidental editing and saving (instead of editing the old, dirty
     * buffer).
     */
    b->rdonly = b->o.readonly = 1;
  } else {
    b = bfind(filename);
  }
  vsrm(filename);
  if (!berror && if_loaded_policy != IF_LOADED_REUSE_KEEP && bw->b != b) {
    /* Enque the new buffer b, so upbufu() will go back to the old buffer */
    deque(B, link, b);
    enqueb(B, link, bw->b, b);
  }
  ret = replace_b_in_bw(bw, b, /*do_orphan:*/orphan, /*do_restore:*/1, /*use_berror:*/berror, /*do_macros:*/1);
  brm(b);  /* Undo b->count increase done by bfind() or bfind_reload() above. */
  return ret;
}

static int doedit1(BW *bw,int c,unsigned char *s,int *notify)
{
	if (c == YES_CODE || yncheck(yes_key, c)) {
		return doedit2(bw, IF_LOADED_RELOAD_ADJUST, s, notify);
	} else if (c == NO_CODE || yncheck(no_key, c)) {
		return doedit2(bw, IF_LOADED_REUSE_ADJUST, s, notify);
	} else {
		/* FIXME: need abort handler to prevent leak */
		if (mkqw(bw->parent, sz(joe_gettext(_("Load original file as read-only from disk (y,n,^C)? "))), doedit1, NULL, s, notify))
			return 0;
		else {
			vsrm(s);
			return -1;
		}
	}
}

int doedit(BW *bw, unsigned char *s, void *obj, int *notify)
{
	B *b;

	b = bcheck_loaded(s);

	if (b && b->changed && !b->scratch) {
		/* Modified buffer exists, so ask */
		return doedit1(bw, 0, s, notify);
	}
	/* No buffer or buffer not modified -- load or just use it as is */
	/* IF_LOADED_REUSE_ADJUST and IF_LOADED_RELOAD_ADJUST
	 * have the same effect here, since the file is not loaded.
	 */
	return doedit2(bw, IF_LOADED_REUSE_ADJUST, s, notify);
}

int okrepl(BW *bw)
{
	if (bw->b->count == 1 && bw->b->changed) {
		msgnw(bw->parent, joe_gettext(_("Can't replace modified file")));
		return -1;
	} else {
		return 0;
	}
}

int uedit(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("Name of file to edit (^C to abort): ")), &filehist, doedit, USTR "Names", NULL, cmplt, NULL, NULL, locale_map,7)) {
		return 0;
	} else {
		return -1;
	}
}

int doswitch(BW *bw, unsigned char *s, void *obj, int *notify)
{
	/* utag(), nxterr() and prverr() use doswitch() to jump to the file */

	/* Try buffer, then file */
	return doedit2(bw, IF_LOADED_REUSE_KEEP, s, notify);
}

int uswitch(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("Name of buffer to edit (^C to abort): ")), &filehist, doswitch, USTR "Names", NULL, cmplt, NULL, NULL, locale_map,1)) {
		return 0;
	} else {
		return -1;
	}
}

int doscratch(BW *bw, unsigned char *s, void *obj, int *notify)
{
	B *b;
	if (notify) {
		*notify = 1;
	}
	b = bfind_scratch(s);
	vsrm(s);
	return replace_b_in_bw(bw, b, /*do_orphan:*/orphan, /*do_restore:*/1, /*use_berror:*/berror, /*do_macros:*/1);
}

int uscratch(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("Name of scratch buffer to edit (^C to abort): ")), &filehist, doscratch, USTR "Names", NULL, cmplt, NULL, NULL, locale_map, 1)) {
		return 0;
	} else {
		return -1;
	}
}

/* Load file into buffer: can result in an orphaned buffer */

static int dorepl(BW *bw, unsigned char *s, void *obj, int *notify)
{
	B *b;
	if (notify) {
		*notify = 1;
	}
	b = bfind(s);
	vsrm(s);
	return replace_b_in_bw(bw, b, /*do_orphan:*/1, /*do_restore:*/1, /*use_berror:*/berror, /*do_macros:*/1);
}

/* Switch to next buffer in the current window */

int unbuf(BW *bw)
{
	B *b;
	b = bnext();
	if (b == bw->b) {
		b = bnext();
	}
	return get_buffer_in_window(bw,b);
}

int upbuf(BW *bw)
{
	B *b;
	b = bprev();
	if (b == bw->b) {
		b = bprev();
	}
	return get_buffer_in_window(bw, b);
}

int unbufu(BW *bw)
{
	Screen *t = bw->parent->t;
	B *b0, *b;

	if (bw->b == errbuf && count_onscreen_main_windows(t) != 1)
		return -1;  /* Beep. */

	/* Move bufs just before bw->b, so bnext() will return the next
	 * buffer with respect to the current buffer (bw->b)
	 */
	deque(B, link, &bufs);
	enqueb(B, link, bw->b, &bufs);

	/* bnext() moves bufs.link.prev forward (past bufs), and returns it */
	b0 = b = bnext();
	while (b == bw->b ||
	       (!b->orphan && find_onscreen_window(b, t)) ||
	       (b->scratch && b->name[0] == '*')  /* Don't switch to `* Grep Log *', `* Build Log *' or other errbufs */
	      ) {
	  b = bnext();
	  if (b == b0) return 0; /* end of loop */
	} 

	return replace_b_in_bw(bw, b, /*do_orphan:*/orphan, /*do_restore:*/0, /*use_berror:*/0, /*do_macros:*/0);
}

int upbufu(BW *bw)
{
	Screen *t = bw->parent->t;
	B *b0, *b;

	/* !! TODO(pts): ^K q in errbuf should kill the window (and refit the
	 *    rest), not the buffer.
	 */
	if (bw->b == errbuf && count_onscreen_main_windows(t) != 1)
		return -1;  /* Beep. */

	/* Move bufs just after bw->b, so bprev() will return the previous
	 * buffer with respect to the current buffer (bw->b)
	 */
	deque(B, link, &bufs);
	enquef(B, link, bw->b, &bufs);
	
	/* bprev() moves bufs.link.next backward (past bufs), and returns it */
	b0 = b = bprev();
	while (b == bw->b ||
	       (!b->orphan && find_onscreen_window(b, t)) ||
	       (b->scratch && b->name[0] == '*')  /* Don't switch to `* Grep Log *', `* Build Log *' or other errbufs */
	      ) {
	  b = bprev();
	  if (b == b0) return 0;
	} 

	return replace_b_in_bw(bw, b, /*do_orphan:*/orphan, /*do_restore:*/0, /*use_berror:*/0, /*do_macros:*/0);
}

int uinsf(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("Name of file to insert (^C to abort): ")), &filehist, doinsf, USTR "Names", NULL, cmplt, NULL, NULL, locale_map, 3)) {
		return 0;
	} else {
		return -1;
	}
}

/* Save and exit */

static int exdone(BW *bw, struct savereq *req,int flg,int *notify)
{
	if (notify)
		*notify = 1;
	rmsavereq(req);
	if (flg) {
		return -1;
	} else {
		bw_unlock(bw);
		bw->b->changed = 0;
		saverr(bw->b->name);
		return uabort1(bw, -1);
	}
}

int uexsve(BW *bw)
{
	if (!bw->b->changed || bw->b->scratch) {
		/* It didn't change or it's just a scratch buffer: don't save */
		uabort(bw, -1);
		return 0;
	} else if (bw->b->name && !exask) {
		/* It changed, it's not a scratch buffer and it's named */
		return dosave1(bw, vsncpy(NULL, 0, sz(bw->b->name)), mksavereq(exdone,NULL,NULL,0,0), NULL);
	} else {
		BW *pbw = wmkpw(bw->parent, joe_gettext(_("Name of file to save (^C to abort): ")), &filehist,
		                dosave1, USTR "Names", NULL, cmplt,
		                mksavereq(exdone,NULL,NULL,1,0), NULL, locale_map, 1);

		if (pbw && bw->b->name) {
			binss(pbw->cursor, bw->b->name);
			pset(pbw->cursor, pbw->b->eof);
			pbw->cursor->xcol = piscol(pbw->cursor);
		}
		if (pbw) {
			return 0;
		} else {
			return -1;
		}
	}
}

/* If buffer is modified, prompt for saving: if user types 'n', uabort(), otherwise just return. */
/* If buffer is not modified, just return. */

static int nask(BW *bw, int c, void *object, int *notify)
{
	if (c == YES_CODE || yncheck(yes_key, c)) {
		/* uexsve macro should be here... */
		if(notify)
			*notify = 1;
		return 0;
	} else if (c == NO_CODE || yncheck(no_key, c)) {
		if(notify)
			*notify = -1;
		genexmsg(bw, 0, NULL);
		abortit(bw);
		return -1;
	} else if (bw->b->count == 1 && bw->b->changed && !bw->b->scratch) {
		if (mkqw(bw->parent, sz(joe_gettext(_("Save changes to this file (y,n,^C)? "))), nask, NULL, object, notify)) {
			return 0;
		} else {
			return -1;
		}
	} else {
		if (notify) {
			*notify = 1;
		}
		return 0;
	}
}

int uask(BW *bw)
{
	return nask(bw, 0, NULL, NULL);
}

/* Kill a buffer: any windows which have it get their buffer replaced with a
 * a scratch buffer */

static int dolose(BW *bw, int c, void *object, int *notify)
{
	W *w;
	B *b, *new_b;
	int cnt;

	if (notify) {
		*notify = 1;
	}
	if (c != YES_CODE && !yncheck(yes_key, c)) {
		return -1;
	}

	b=bw->b;
	cnt = b->count;
	b->count = 1;
	genexmsg(bw, 0, NULL);
	b->count = cnt;

	if ((w = maint->topwin) != NULL) {
		do {
			if ((w->watom->what&TYPETW) && ((BW *)w->object)->b==b) {
				if ((new_b = borphan()) != NULL) {
					BW *bw = (BW *)w->object;
					void *object = bw->object;
					/* FIXME: Shouldn't we wabort() and wcreate here to kill
					   any prompt windows? */

					bwrm(bw);
					w->object = (void *) (bw = bwmk(w, new_b, 0));
					wredraw(w);
					bw->object = object;
				} else {
					BW *bw = (BW *)w->object;
					object = bw->object;
					bwrm(bw);
					w->object = (void *) (bw = bwmk(w, bfind(USTR ""), 0));
					wredraw(w);
					bw->object = object;
					if (bw->o.mnew)
						exmacro(bw->o.mnew,1);
				}
			}
		w = w->link.next;
		} while (w != maint->topwin);
	}
	return 0;
}

int ulose(BW *bw)
{
	msgnw(bw->parent, NULL);
	if (bw->b->count==1 && bw->b->pid) {
		return ukillpid(bw);
	}
	if (bw->b->changed && !bw->b->scratch) {
		if (mkqw(bw->parent, sz(joe_gettext(_("Lose changes to this file (y,n,^C)? "))), dolose, NULL, NULL, NULL)) {
			return 0;
		} else {
			return -1;
		}
	} else {
		return dolose(bw, YES_CODE, NULL, NULL);
	}
}

/* Buffer list */

#ifdef junk

static int dobuf(MENU *m, int x, unsigned char **s)
{
	unsigned char *name;
	BW *bw = m->parent->win->object;
	int *notify = m->parent->notify;

	m->parent->notify = 0;
	name = vsdup(s[x]);
	wabort(m->parent);
	return dorepl(bw, name, NULL, notify);
}

static int abrtb(MENU *m, int x, unsigned char **s)
{
	varm(s);
	return -1;
}

int ubufed(BW *bw)
{
	unsigned char **s = getbufs();

	vasort(av(s));
	if (mkmenu(bw->parent, bw->parent, s, dobuf, abrtb, NULL, 0, s, NULL))
		return 0;
	else {
		varm(s);
		return -1;
	}
}

#endif

unsigned char **sbufs = NULL;	/* Array of command names */

static int bufedcmplt(BW *bw)
{
	if (sbufs) {
		varm(sbufs);
		sbufs = 0;
	}
	if (!sbufs)
		sbufs = getbufs();
	return simple_cmplt(bw,sbufs);
}

static int dobufed(BW *bw, unsigned char *s, void *object, int *notify)
{
/* not understanding this...
	int *notify = bw->parent->notify;

	bw->parent->notify = 0;
	wabort(bw->parent);
*/
	return dorepl(bw, s, NULL, notify);
}

B *bufhist = NULL;

int ubufed(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("Name of buffer to edit (^C to abort): ")), &bufhist, dobufed, USTR "bufed", NULL, bufedcmplt, NULL, NULL, locale_map, 0)) {
		return 0;
	} else {
		return -1;
	}
}


/* Query save loop */

static int doquerysave(BW *bw,int c,struct savereq *req,int *notify)
{
	W *w = bw->parent;
	if (c == YES_CODE || yncheck(yes_key, c)) {
		if (bw->b->name && bw->b->name[0])
			return dosave1(bw, vsncpy(NULL,0,sz(bw->b->name)), req, notify);
		else {
			BW *pbw;
			pbw = wmkpw(bw->parent, joe_gettext(_("Name of file to save (^C to abort): ")), &filehist, dosave1, USTR "Names", NULL, cmplt, req, notify, locale_map, 7);

			if (pbw) {
				return 0;
			} else {
				joe_free(req);
				return -1;
			}
		}
	} else if (c == NO_CODE || yncheck(no_key, c)) {
		/* Find next buffer to save */
		if (bw->b->changed)
			req->not_saved = 1;
		next:
		if (unbuf(bw)) {
			if (notify)
				*notify = 1;
			rmsavereq(req);
			return -1;
		}
		bw = w->object;
		if (bw->b==req->first) {
			if (notify)
				*notify = 1;
			rmsavereq(req);
			genexmsgmulti(bw,1,req->not_saved);
			return 0;
		}
		if (!bw->b->changed || bw->b->scratch)
			goto next;

		return doquerysave(bw,0,req,notify);
	} else {
		unsigned char buf[1024];
		joe_snprintf_1(buf,1024,joe_gettext(_("File %s has been modified.  Save it (y,n,^C)? ")),bw->b->name ? bw->b->name : USTR "(Unnamed)" );
		if (mkqw(bw->parent, sz(buf), doquerysave, NULL, req, notify)) {
			return 0;
			} else {
			/* Should be in abort function */
			rmsavereq(req);
			return -1;
		}
	}
}

static int query_next(BW *bw, struct savereq *req,int flg,int *notify)
{
	if (flg) {
		if (notify)
			*notify = 1;
		rmsavereq(req);
		return -1;
	} else
		return doquerysave(bw,NO_CODE,req,notify);
}

int uquerysave(BW *bw)
{
	W *w = bw->parent;
	B *first;

	/* Get synchronized with buffer ring */
	unbuf(bw);
	bw = w->object;
	first = bw->b;

	/* Find a modified buffer */
	do {
		if (bw->b->changed && !bw->b->scratch)
			return doquerysave(bw,0,mksavereq(query_next,NULL,first,0,0),NULL);
		else if (unbuf(bw))
			return -1;
		bw = w->object;
	} while(bw->b!=first);

	genexmsgmulti(bw,0,0);

	return 0;
}

int ukilljoe(BW *bw)
{
	/* Save file positions */
	set_file_pos_all(bw->parent->t);
	/* FIXME: emacs checks for unsaved modified buffers at this point */
	leave = 1;
	return 0;
}

static int doreload(BW *bw, int c, void *object, int *notify)
{
	B *n;
	if (notify) {
		*notify = 1;
	}
	if (c != YES_CODE && !yncheck(yes_key, c)) {
		return -1;
	}
	n = bload(bw->b->name);
	if (berror) {
		brm(n);
		msgnw(bw->parent, joe_gettext(msgs[-berror]));
		return -1;
	}
	breplace(bw->b, n);
	nredraw(bw->parent->t->t);
	msgnw(bw->parent, joe_gettext(_("File reloaded")));
	return 0;
}

int ureload(BW *bw)
{
	if (!plain_file(bw->b)) {
		msgnw(bw->parent, joe_gettext(_("Can only reload plain files")));
		return -1;
	}
	if (bw->b->changed) {
		if (mkqw(bw->parent, sz(joe_gettext(_("Lose changes to this file (y,n,^C)? "))), doreload, NULL, NULL, NULL)) {
			return 0;
		} else {
			return -1;
		}
	}
	return doreload(bw, YES_CODE, NULL, NULL);
}

int ureload_all(BW *bw)
{
	int count = 0;
	int er = 0;
	B *b;
	for (b = bufs.link.next; b != &bufs; b = b->link.next)
		if (!b->changed && plain_file(b)) {
			B *n = bload(b->name);
			if (berror) {
				msgnw(bw->parent, joe_gettext(msgs[-berror]));
				er = -1;
				brm(n);
			} else {
				breplace(b, n);
				++count;
			}
		}
	nredraw(bw->parent->t->t);
	if (!er) {
		joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("%d files reloaded")), count);
		msgnw(bw->parent, msgbuf);
	}
	return er;
}
