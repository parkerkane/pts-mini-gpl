/*
 *	tags file symbol lookup
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 * 	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

/* Count the number of lines in f (from f's seek offset)
 * starting with tag.
 */
static int count_tag_lines(FILE *f, unsigned char const *tag,
    unsigned char *buf, size_t buf_size) {
  size_t tag_len = zlen((unsigned char*)tag);
  unsigned char const *p;
  int count = 0, c;
  while (fgets((char *)buf, buf_size, f)) {
    p = buf;
    while (*p != ' ' && *p != '\t' && *p != '\0') ++p;
    if (*p == '\0') {  /* Read till EOL. */
      while ((c = getc(f)) != '\n' && c >= 0) {}
    }
    if ((*p != '\t' && *p != ' ') ||
        (size_t)(p - buf) != tag_len ||
        0 != zncmp(buf, (unsigned char*)tag, tag_len)) break;
    ++count;
  }
  return count;
}

/* Jump to tag word s, free s */
static int dotag(BW *bw, unsigned char *s, void *obj, int *notify)
{
	unsigned char buf[512];
	unsigned char buf1[sizeof(buf)];
	FILE *f;
	unsigned char *t = NULL;
	int arg = simple_arg;
	int ignore_count = arg > 1 ? arg - 1 : 0;
	/** Number of times tag found. */
	int count = 0;

	if (notify) {
		*notify = 1;
	}
	if (!sLEN(s)) {
		msgnw(bw->parent, joe_gettext(_("Can't search for empty tag")));
		vsrm(s);
		return -1;
	}
	if (bw->b->name) {
		t = vsncpy(t, 0, sz(bw->b->name));
		t = vsncpy(sv(t), sc(":"));
		t = vsncpy(sv(t), sv(s));
	}
	/* first try to open the tags file in the current directory */
	f = fopen("tags", "r");
	if (!f) {
		/* if there's no tags file in the current dir, then query
		   for the environment variable TAGS.
		*/
		char *tagspath = getenv("TAGS");
		if(tagspath){
			f = fopen(tagspath, "r");    
		}
		if(!f){
			msgnw(bw->parent, joe_gettext(_("Couldn't open tags file")));
			vsrm(s);
			vsrm(t);
			return -1;
		}
	}
	while (fgets((char *)buf, sizeof(buf), f)) {
		int x, y, c;

		for (x = 0; buf[x] && buf[x] != ' ' && buf[x] != '\t'; ++x) ;
		c = buf[x];
		buf[x] = 0;
		if (!zcmp(s, buf) || (t && !zcmp(t, buf))) {
			++count;
			if (ignore_count > 0) {
				--ignore_count;
				continue;
			}
			buf[x] = c;
			while (buf[x] == ' ' || buf[x] == '\t') {
				++x;
			}
			for (y = x; buf[y] && buf[y] != ' ' && buf[y] != '\t' && buf[y] != '\n'; ++y) ;
			if (x != y) {
				c = buf[y];
				buf[y] = 0;
				if (doswitch(bw, vsncpy(NULL, 0, sz(buf + x)), NULL, NULL)) {
					vsrm(s);
					vsrm(t);
					fclose(f);
					return -1;
				}
				bw = (BW *) maint->curwin->object;
				p_goto_bof(bw->cursor);
				buf[y] = c;
				while (buf[y] == ' ' || buf[y] == '\t') {
					++y;
				}
				for (x = y; buf[x] && buf[x] != '\n'; ++x) ;
				buf[x] = 0;
				if (x != y) {
					long line = 0;

					if (buf[y] >= '0' && buf[y] <= '9') {
						/* It's a line number */
						sscanf((char *)(buf + y), "%ld", &line);
						if (line >= 1) {
							int omid = mid;

							mid = 1;
							pline(bw->cursor, line - 1), bw->cursor->xcol = piscol(bw->cursor);
							dofollows();
							mid = omid;
						} else {
							msgnw(bw->parent, joe_gettext(_("Invalid line number")));
						}
					} else {
						int z = 0;
						/* It's a search string. New versions of
						   ctags have real regex with vi command.  Old
						   ones do not always quote / and depend on it
						   being last char on line. */
						if (buf[y] == '/' || buf[y] == '?') {
							int ch = buf[y++];
							/* Find terminating / or ? */
							for (x = y + zlen(buf + y); x != y; --x)
								if (buf[x] == ch)
									break;
							/* Copy characters, convert to JOE regex... */
							if (buf[y] == '^') {
								buf1[z++] = '\\';
								buf1[z++] = '^';
								++y;
							}
							while (buf[y] && buf[y] != '\n' && !(buf[y] == ch && y == x)) {
								if (buf[y] == '$' && buf[y+1] == ch) {
									++y;
									buf1[z++] = '\\';
									buf1[z++] = '$';
								} else if (buf[y] == '\\' && buf[y+1]) {
									/* This is going to cause problem...
									   old ctags did not have escape */
									++y;
									if (buf[y] == '\\')
										buf1[z++] = '\\';
									buf1[z++] = buf[y++];
								} else {
									buf1[z++] = buf[y++];
								}
							}
						}
						if (z) {
							int ret;
							count += count_tag_lines(f, s, buf, sizeof(buf));
							if (count > 1) {
								joe_snprintf_3(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("Showing %d of %d for tag %s")), arg, count, s);
								msgnw(bw->parent, msgbuf);
							}
							vsrm(s);
							vsrm(t);
							fclose(f);
							buf1[z] = 0;
							ret = dopfnext(bw, mksrch(vsncpy(NULL, 0, sz(buf1)), NULL, 0, 0, -1, 0, 0, 0), NULL);
							if (ret == 0) ret = STOP_REPEATING;
							return ret;
						}
					}
				}
				vsrm(s);
				vsrm(t);
				fclose(f);
				return 0;
			}
		}
	}
	if (count) {
		joe_snprintf_2(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("Tag %s found only %d times")), s, count);
	} else {
		joe_snprintf_1(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("Tag %s not found")), s);
	}
	msgnw(bw->parent, msgbuf);
	vsrm(s);
	vsrm(t);
	fclose(f);
	return -1;
}

static unsigned char **get_tag_list()
{
	unsigned char buf[512];
	unsigned char tag[512];
	int i,pos;
	FILE *f;
	unsigned char **lst = NULL;
	
	f = fopen("tags", "r");
	if (f) {
		while (fgets((char *)buf, 512, f)) {
			pos = 0;
			for (i=0; i<512; i++) {
				if (buf[i] == ' ' || buf[i] == '\t') {
					pos = i;
					i = 512;
				}
			}
			if (pos > 0) {
				zncpy(tag, buf, pos);
				tag[pos] = '\0';
			}
			lst = vaadd(lst, vsncpy(NULL, 0, sz(tag)));
		}
		fclose(f);	
	}
	return lst;
}

static unsigned char **tag_word_list;

static int tag_cmplt(BW *bw)
{
	/* Reload every time: we should really check date of tags file...
	if (tag_word_list)
		varm(tag_word_list); */

	if (!tag_word_list)
		tag_word_list = get_tag_list();

	if (!tag_word_list) {
		ttputc(7);
		return 0;
	}

	return simple_cmplt(bw,tag_word_list);
}

static B *taghist = NULL;

/** Copy word at p to new B and return it. */
static B *get_word_at_p(P *p) {
	struct charmap *charmap = p->b->o.charmap;
	B *bret;
	P *q;
	int c;
	p = pdup(p, USTR "utag");
	q = pdup(p, USTR "utag");

	while (joe_isalnum_(charmap,(c = prgetc(p))))
		/* do nothing */;
	if (c != NO_MORE_DATA) {
		pgetc(p);
	}
	pset(q, p);
	while (joe_isalnum_(charmap,(c = pgetc(q))))
		/* do nothing */;
	if (c != NO_MORE_DATA) {
		prgetc(q);
	}
	bret = bcpy(p, q);
	prm(p);
	prm(q);
	return bret;
}

/** Jump to the error message or tag on bw->cursor. */
int utaghere(BW *bw) {
	unsigned char *word;
	B *b;
	if (bw->b == errbuf) return ujump(bw);
	b = get_word_at_p(bw->cursor);
	word = brvs(b->bof, b->eof->byte);
	brm(b);
	if (!sLEN(word)) {
		msgnw(bw->parent, joe_gettext(_("No tag word at cursor")));
		vsrm(word);
		return -1;
	}
	return dotag(bw, word, NULL, NULL);
}

int utag(BW *bw)
{
	BW *pbw;

	pbw = wmkpw(bw->parent, joe_gettext(_("Tag search: ")), &taghist, dotag, NULL, NULL, tag_cmplt, NULL, NULL, locale_map, 0);
	if (pbw && joe_isalnum_(bw->b->o.charmap,brch(bw->cursor))) {
		binsb_decref(pbw->cursor, get_word_at_p(bw->cursor));
		pset(pbw->cursor, pbw->b->eof);
		pbw->cursor->xcol = piscol(pbw->cursor);
	}
	if (pbw) {
		return 0;
	} else {
		return -1;
	}
}
