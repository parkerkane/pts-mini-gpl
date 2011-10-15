/*
 *	UTF-8 Utilities
 *	Copyright
 *		(C) 2004 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

/* Under AmigaOS we have setlocale() but don't have langinfo.h and associated stuff,
 * so we have to disable the whole piece of code
 */
#ifdef __amigaos
#undef HAVE_SETLOCALE
#endif

/* Cygwin has CODESET, but it's crummy */
#ifdef __CYGWIN__
#undef HAVE_SETLOCALE
#endif

/* If it looks old, forget it */
#ifndef CODESET
#undef HAVE_SETLOCALE
#endif

#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
#	include <locale.h>
#       include <langinfo.h>
#endif

/* nl_langinfo(CODESET) is broken on many systems.  If HAVE_SETLOCALE is undefined,
   JOE uses a limited internal version instead */

/* UTF-8 Encoder
 *
 * c is unicode character.
 * buf is 7 byte buffer- utf-8 coded character is written to this followed by a 0 termination.
 * returns length (not including terminator).
 */

int utf8_encode(unsigned char *buf,int c)
{
	if (c < 0x80) {
		buf[0] = c;
		buf[1] = 0;
		return 1;
	} else if(c < 0x800) {
		buf[0] = (0xc0|(c>>6));
		buf[1] = (0x80|(c&0x3F));
		buf[2] = 0;
		return 2;
	} else if(c < 0x10000) {
		buf[0] = (0xe0|(c>>12));
		buf[1] = (0x80|((c>>6)&0x3f));
		buf[2] = (0x80|((c)&0x3f));
		buf[3] = 0;
		return 3;
	} else if(c < 0x200000) {
		buf[0] = (0xf0|(c>>18));
		buf[1] = (0x80|((c>>12)&0x3f));
		buf[2] = (0x80|((c>>6)&0x3f));
		buf[3] = (0x80|((c)&0x3f));
		buf[4] = 0;
		return 4;
	} else if(c < 0x4000000) {
		buf[0] = (0xf8|(c>>24));
		buf[1] = (0x80|((c>>18)&0x3f));
		buf[2] = (0x80|((c>>12)&0x3f));
		buf[3] = (0x80|((c>>6)&0x3f));
		buf[4] = (0x80|((c)&0x3f));
		buf[5] = 0;
		return 5;
	} else {
		buf[0] = (0xfC|(c>>30));
		buf[1] = (0x80|((c>>24)&0x3f));
		buf[2] = (0x80|((c>>18)&0x3f));
		buf[3] = (0x80|((c>>12)&0x3f));
		buf[4] = (0x80|((c>>6)&0x3f));
		buf[5] = (0x80|((c)&0x3f));
		buf[6] = 0;
		return 6;
	}
}

/* UTF-8 Decoder
 *
 * Returns 0 - 7FFFFFFF: decoded character
 *                   -1: byte accepted, no character decoded yet
 *                   -2: incomplete byte sequence
 *                   -3: no byte sequence started, but character is between 128 - 191, 254 or 255
 */

int utf8_decode(struct utf8_sm *utf8_sm,unsigned char c)
{
	if (utf8_sm->state) {
		if ((c&0xC0)==0x80) {
			utf8_sm->buf[utf8_sm->ptr++] = c;
			--utf8_sm->state;
			utf8_sm->accu = ((utf8_sm->accu<<6)|(c&0x3F));
			if(!utf8_sm->state)
				return utf8_sm->accu;
		} else {
			utf8_sm->state = 0;
			return -2;
		}
	} else if ((c&0xE0)==0xC0) {
		/* 192 - 223 */
		utf8_sm->buf[0] = c;
		utf8_sm->ptr = 1;
		utf8_sm->state = 1;
		utf8_sm->accu = (c&0x1F);
	} else if ((c&0xF0)==0xE0) {
		/* 224 - 239 */
		utf8_sm->buf[0] = c;
		utf8_sm->ptr = 1;
		utf8_sm->state = 2;
		utf8_sm->accu = (c&0x0F);
	} else if ((c&0xF8)==0xF0) {
		/* 240 - 247 */
		utf8_sm->buf[0] = c;
		utf8_sm->ptr = 1;
		utf8_sm->state = 3;
		utf8_sm->accu = (c&0x07);
	} else if ((c&0xFC)==0xF8) {
		/* 248 - 251 */
		utf8_sm->buf[0] = c;
		utf8_sm->ptr = 1;
		utf8_sm->state = 4;
		utf8_sm->accu = (c&0x03);
	} else if ((c&0xFE)==0xFC) {
		/* 252 - 253 */
		utf8_sm->buf[0] = c;
		utf8_sm->ptr = 1;
		utf8_sm->state = 5;
		utf8_sm->accu = (c&0x01);
	} else if ((c&0x80)==0x00) {
		/* 0 - 127 */
		utf8_sm->buf[0] = c;
		utf8_sm->ptr = 1;
		utf8_sm->state = 0;
		return c;
	} else {
		/* 128 - 191, 254, 255 */
		utf8_sm->ptr = 0;
		utf8_sm->state = 0;
		return -3;
	}
	return -1;
}

/* Initialize state machine */

void utf8_init(struct utf8_sm *utf8_sm)
{
	utf8_sm->ptr = 0;
	utf8_sm->state = 0;
}

/* Decode an entire string */

int utf8_decode_string(unsigned char *s)
{
	struct utf8_sm sm;
	int x;
	int c = -1;
	utf8_init(&sm);
	for(x=0;s[x];++x)
		c = utf8_decode(&sm,s[x]);
	return c;
}

/* Decode and advance
 *
 * Returns: 0 - 7FFFFFFF: decoded character
 *  -2: incomplete sequence
 *  -3: bad start of sequence found.
 *
 * p/plen are always advanced in such a way that repeated called to utf8_decode_fwrd do not cause
 * infinite loops.
 */

int utf8_decode_fwrd(unsigned char **p,int *plen)
{
	struct utf8_sm sm;
	unsigned char *s = *p;
	int len;
	int c = -2; /* Return this on no more input. */
	if (plen)
		len = *plen;
	else
		len = -1;

	utf8_init(&sm);

	while (len) {
		c = utf8_decode(&sm, *s);
		if (c >= 0) {
			/* We've got a character */
			--len;
			++s;
			break;
		} else if (c == -2) {
			/* Bad sequence detected.  Caller should feed rest of string in again. */
			break;
		} else if (c == -3) {
			/* Bad start of UTF-8 sequence.  We need to eat this char to avoid infinite loops. */
			--len;
			++s;
			/* But we should tell the caller that something bad was found. */
			break;
		} else {
			/* If c is -1, utf8_decode accepted the character, so we should get the next one. */
			--len;
			++s;
		}
	}

	if (plen)
		*plen = len;
	*p = s;

	return c;
}

/* For systems (BSD) with no nl_langinfo(CODESET) */

/*
 * This is a quick-and-dirty emulator of the nl_langinfo(CODESET)
 * function defined in the Single Unix Specification for those systems
 * (FreeBSD, etc.) that don't have one yet. It behaves as if it had
 * been called after setlocale(LC_CTYPE, ""), that is it looks at
 * the locale environment variables.
 *
 * http://www.opengroup.org/onlinepubs/7908799/xsh/langinfo.h.html
 *
 * Please extend it as needed and suggest improvements to the author.
 * This emulator will hopefully become redundant soon as
 * nl_langinfo(CODESET) becomes more widely implemented.
 *
 * Since the proposed Li18nux encoding name registry is still not mature,
 * the output follows the MIME registry where possible:
 *
 *   http://www.iana.org/assignments/character-sets
 *
 * A possible autoconf test for the availability of nl_langinfo(CODESET)
 * can be found in
 *
 *   http://www.cl.cam.ac.uk/~mgk25/unicode.html#activate
 *
 * Markus.Kuhn@cl.cam.ac.uk -- 2002-03-11
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted. The author
 * disclaims all warranties with regard to this software.
 *
 * Latest version:
 *
 *   http://www.cl.cam.ac.uk/~mgk25/ucs/langinfo.c
 */

unsigned char *joe_getcodeset(unsigned char *l)
{
  static unsigned char buf[16];
  unsigned char *p;

  if (l || ((l = (unsigned char *)getenv("LC_ALL"))   && *l) ||
      ((l = (unsigned char *)getenv("LC_CTYPE")) && *l) ||
      ((l = (unsigned char *)getenv("LANG"))     && *l)) {

    /* check standardized locales */
    if (!zcmp(l, USTR "C") || !zcmp(l, USTR "POSIX") || !zcmp(l, USTR "ascii"))
      return USTR "ascii";

    /* check for encoding name fragment */
    if (zstr(l, USTR "UTF") || zstr(l, USTR "utf"))
      return USTR "UTF-8";

    if ((p = zstr(l, USTR "8859-"))) {
      memcpy((char *)buf, "ISO-8859-\0\0", 12);
      p += 5;
      if (*p >= '0' && *p <= '9') {
	buf[9] = *p++;
	if (*p >= '0' && *p <= '9') buf[10] = *p++;
	return buf;
      }
    }

    if (zstr(l, USTR "EUC-JP")) return USTR "EUC-JP";
    if (zstr(l, USTR "EUC-KR")) return USTR "EUC-KR";
    if (zstr(l, USTR "KOI8-R")) return USTR "KOI8-R";
    if (zstr(l, USTR "KOI8-U")) return USTR "KOI8-U";
    if (zstr(l, USTR "620")) return USTR "TIS-620";
    if (zstr(l, USTR "2312")) return USTR "GB2312";
    if (zstr(l, USTR "HKSCS")) return USTR "Big5HKSCS";   /* no MIME charset */
    if (zstr(l, USTR "Big5") || zstr(l, USTR "BIG5")) return USTR "Big5";
    if (zstr(l, USTR "GBK")) return USTR "GBK";           /* no MIME charset */
    if (zstr(l, USTR "18030")) return USTR "GB18030";     /* no MIME charset */
    if (zstr(l, USTR "Shift_JIS") || zstr(l, USTR "SJIS")) return USTR "Shift_JIS";
    /* check for conclusive modifier */
    if (zstr(l, USTR "euro")) return USTR "ISO-8859-15";
    /* check for language (and perhaps country) codes */
    if (zstr(l, USTR "zh_TW")) return USTR "Big5";
    if (zstr(l, USTR "zh_HK")) return USTR "Big5HKSCS";   /* no MIME charset */
    if (zstr(l, USTR "zh")) return USTR "GB2312";
    if (zstr(l, USTR "ja")) return USTR "EUC-JP";
    if (zstr(l, USTR "ko")) return USTR "EUC-KR";
    if (zstr(l, USTR "ru")) return USTR "KOI8-R";
    if (zstr(l, USTR "uk")) return USTR "KOI8-U";
    if (zstr(l, USTR "pl") || zstr(l, USTR "hr") ||
	zstr(l, USTR "hu") || zstr(l, USTR "cs") ||
	zstr(l, USTR "sk") || zstr(l, USTR "sl")) return USTR "ISO-8859-2";
    if (zstr(l, USTR "eo") || zstr(l, USTR "mt")) return USTR "ISO-8859-3";
    if (zstr(l, USTR "el")) return USTR "ISO-8859-7";
    if (zstr(l, USTR "he")) return USTR "ISO-8859-8";
    if (zstr(l, USTR "tr")) return USTR "ISO-8859-9";
    if (zstr(l, USTR "th")) return USTR "TIS-620";      /* or ISO-8859-11 */
    if (zstr(l, USTR "lt")) return USTR "ISO-8859-13";
    if (zstr(l, USTR "cy")) return USTR "ISO-8859-14";
    if (zstr(l, USTR "ro")) return USTR "ISO-8859-2";   /* or ISO-8859-16 */
    if (zstr(l, USTR "am") || zstr(l, USTR "vi")) return USTR "UTF-8";
    /* Send me further rules if you like, but don't forget that we are
     * *only* interested in locale naming conventions on platforms
     * that do not already provide an nl_langinfo(CODESET) implementation. */
    return USTR "ISO-8859-1"; /* should perhaps be "UTF-8" instead */
  }
  /* binary becomes ASCII here */
  return USTR "ascii";
}

/* Initialize locale for JOE */

unsigned char *codeset;	/* Codeset of terminal */

unsigned char *non_utf8_codeset;
			/* Codeset of local language non-UTF-8 */

unsigned char *locale_lang;
			/* Our local language */

unsigned char *locale_msgs;
			/* Language to use for editor messages */

struct charmap *locale_map;
			/* Character map of terminal, default map for new files */

struct charmap *locale_map_non_utf8;
			/* Old, non-utf8 version of locale */

void joe_locale()
{
	unsigned char *s, *t, *u;

	s=(unsigned char *)getenv("LC_ALL");
	if (!s || !*s) {
		s=(unsigned char *)getenv("LC_MESSAGES");
		if (!s || !*s) {
			s=(unsigned char *)getenv("LANG");
		}
	}

	if (s)
		s=zdup(s);
	else
		s=USTR "ascii";

	if ((t=zrchr(s,'.')))
		*t = 0;

	locale_msgs = s;

	s=(unsigned char *)getenv("LC_ALL");
	if (!s || !*s) {
		s=(unsigned char *)getenv("LC_CTYPE");
		if (!s || !*s) {
			s=(unsigned char *)getenv("LANG");
		}
	}

	if (s)
		s=zdup(s);
	else
		s=USTR "ascii";

	u = zdup(s);

	if ((t=zrchr(s,'.')))
		*t = 0;

	locale_lang = s;

#ifdef HAVE_SETLOCALE
	setlocale(LC_ALL,(char *)s);
	non_utf8_codeset = zdup((unsigned char *)nl_langinfo(CODESET));
#else
        /**** pts ****/  /* TODO(pts): Make this better? */
	non_utf8_codeset = joe_getcodeset(s);
#endif
	/* LC_CTYPE=en_US.UTF-8 */
	/* s=en_US non_utf8_codeset=ISO-8859-1 */
	/* fprintf(stderr, "s=%s non_utf8_codeset=%s\n", s, non_utf8_codeset); */


	/* printf("joe_locale\n"); */
#ifdef HAVE_SETLOCALE
	/* printf("set_locale\n"); */
	setlocale(LC_ALL,"");
#ifdef ENABLE_NLS
	/* Set up gettext() */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	/* printf("%s %s %s\n",PACKAGE,LOCALEDIR,joe_gettext("New File")); */
#endif
	codeset = zdup((unsigned char *)nl_langinfo(CODESET));
#else
	codeset = joe_getcodeset(u);
#endif

	/* u=en_US.UTF-8 codeset=UTF-8 */
	/* fprintf(stderr, "u=%s codeset=%s\n", u, codeset); */

	locale_map = find_charmap(codeset);
	if (!locale_map)
		locale_map = find_charmap(USTR "ascii");

	locale_map_non_utf8 = find_charmap(non_utf8_codeset);
	if (!locale_map_non_utf8)
		locale_map_non_utf8 = find_charmap(USTR "ascii");

	fdefault.charmap = locale_map;
	pdefault.charmap = locale_map;

/*
	printf("Character set is %s\n",locale_map->name);

	for(x=0;x!=128;++x)
		printf("%x	space=%d blank=%d alpha=%d alnum=%d punct=%d print=%d\n",
		       x,joe_isspace(locale_map,x), joe_isblank(locale_map,x), joe_isalpha_(locale_map,x),
		       joe_isalnum_(locale_map,x), joe_ispunct(locale_map,x), joe_isprint(locale_map,x));
*/

/* Use iconv() */
#ifdef junk
	to_utf = iconv_open("UTF-8", (char *)non_utf8_codeset);
	from_utf = iconv_open((char *)non_utf8_codeset, "UTF-8");
#endif

/* For no locale support */
#ifdef junk
	locale_map = find_charmap("ascii");
	fdefault.charmap = locale_map;
	pdefault.charmap = locale_map;
#endif
	init_gettext(locale_msgs);
}

void to_utf8(struct charmap *map,unsigned char *s,int c)
{
	int d = to_uni(map,c);

	if (d==-1)
		utf8_encode(s,'?');
	else
		utf8_encode(s,d);
#ifdef junk
	/* Iconv() way */
	unsigned char buf[10];
	unsigned char *ibufp = buf;
	unsigned char *obufp = s;
	int ibuf_sz=1;
	int obuf_sz= 10;
	buf[0]=c;
	buf[1]=0;

	if (to_utf==(iconv_t)-1 ||
	    iconv(to_utf,(char **)&ibufp,&ibuf_sz,(char **)&obufp,&obuf_sz)==(size_t)-1) {
		s[0]='?';
		s[1]=0;
	} else {
		*obufp = 0;
	}
#endif
}

int from_utf8(struct charmap *map,unsigned char *s)
{
	int d = utf8_decode_string(s);
	int c = from_uni(map,d);
	if (c==-1)
		return '?';
	else
		return c;

#ifdef junk
	/* Iconv() way */
	int ibuf_sz=zlen(s);
	unsigned char *ibufp=s;
	int obuf_sz=10;
	unsigned char obuf[10];
	unsigned char *obufp = obuf;


	if (from_utf==(iconv_t)-1 ||
	    iconv(from_utf,(char **)&ibufp,&ibuf_sz,(char **)&obufp,&obuf_sz)==((size_t)-1))
		return '?';
	else
		return obuf[0];
#endif
}

void my_iconv(unsigned char *dest,struct charmap *dest_map,
              unsigned char *src,struct charmap *src_map)
{
	if (dest_map == src_map) {
		zcpy (dest, src);
		return;
	}

	if (src_map->type) {
		/* src is UTF-8 */
		if (dest_map->type) {
			/* UTF-8 to UTF-8? */
			zcpy (dest, src);
		} else {
			/* UTF-8 to non-UTF-8 */
			while (*src) {
				int len = -1;
				int c = utf8_decode_fwrd(&src, &len);
				if (c >= 0) {
					int d = from_uni(dest_map, c);
					if (d >= 0)
						*dest++ = d;
					else
						*dest++ = '?';
				} else
					*dest++ = 'X';
			}
			*dest = 0;
		}
	} else {
		/* src is not UTF-8 */
		if (!dest_map->type) {
			/* Non UTF-8 to non-UTF-8 */
			while (*src) {
				int c = to_uni(src_map, *src++);
				int d;
				if (c >= 0) {
					d = from_uni(dest_map, c);
					if (d >= 0)
						*dest++ = d;
					else
						*dest++ = '?';
				} else
					*dest++ = '?';
			}
			*dest = 0;
		} else {
			/* Non-UTF-8 to UTF-8 */
			while (*src) {
				int c = to_uni(src_map, *src++);
				if (c >= 0)
					dest += utf8_encode(dest, c);
				else
					*dest++ = '?';
			}
			*dest = 0;
		}
	}
}

/* Guess character set */

int guess_non_utf8;
int guess_utf8;

/**** pts ****/

typedef char const* charset_t;

/* TODO(pts): Convert these to arrays */
/** Charset not indicated within the file. */
charset_t CS_UNKNOWN = ".UNKNOWN";
charset_t CS_CONFLICT = ".CONFLICT";
/** Detected an encoding specification (such as windows-cp1250), but it is
 * not supported (yet).
 */
charset_t CS_UNSUPPORTED = ".UNSUPPORTED";
/** A charset which is known to be bad or impossible. */
charset_t CS_ERROR = ".ERROR";
/** Conversion is no-op (same as Latin-1), but can be autodetected */
charset_t CS_BINARY = ".BINARY";

/* joe_getcodeset() must be able to parse these */
charset_t CS_ASCII = "ascii";
charset_t CS_UTF8 = "UTF-8";
charset_t CS_UTF16BE = "UTF-16BE";
charset_t CS_UTF16LE = "UTF-16LE";
charset_t CS_KOI8R = "KOI8-R";
charset_t CS_KOI8U = "KOI8-U";
charset_t CS_TIS620 = "TIS-620";
charset_t CS_GB2312 = "GB2312";
charset_t CS_BIG5HKSCS = "BIG5HKSCS";
charset_t CS_BIG5 = "BIG5";
charset_t CS_GBK = "GBK";
charset_t CS_GB18030 = "GB18030";
charset_t CS_SHIFTJIS = "Shift_JIS";
charset_t CS_EUCJP = "EUC-JP";
charset_t CS_EUCKR = "EUC-KR";

/* joe_getcodeset() must be able to parse these */
charset_t charsets_latin[] = {
  NULL,
  "ISO-8859-1",
  "ISO-8859-2",
  "ISO-8859-3",
  "ISO-8859-4",
  "ISO-8859-5",
  "ISO-8859-6",
  "ISO-8859-7",
  "ISO-8859-8",
  "ISO-8859-9",
  "ISO-8859-10",
  "ISO-8859-11",
  "ISO-8859-12",
  "ISO-8859-13",
  "ISO-8859-14",
  "ISO-8859-15",
  "ISO-8859-16",
  "ISO-8859-17",
  "ISO-8859-18",
  "ISO-8859-19",
};

#if 'a'!=97 || 'A'!=65
#  error ASCII system expected
#endif

#define TOLOWER(c) ((c)+((c)-'A'+0U<='Z'-'A'+0U ? 'a'-'A' : 0))
#define ISALPHA(c) (((c)|32)-'a'+0U<='z'-'a'+0U) /* Dat: no Perl \w '_' */
#define ISDIGIT(c) ((c)-'0'+0U<='9'-'0'+0U)
#define ISEMACSNAME(c) (ISALPHA(c) || (c)=='-')
/** 1st char of XML name */
#define ISXMLNAME1(c) (ISALPHA(c))
/** Beyond the 1st char of an XML tag or attribute name (ASCII). (ad hoc) */
#define ISXMLNAME2(c) (ISALPHA(c) || ISDIGIT(c) || (c)=='-' || (c)==':')
#define ISWSPACE(c) ((c)==0 || (c)==32 || (c)-8U<=5U)
#define ISWNZSPACE(c) ((c)==32 || (c)-8U<=5U)

/** Ignores case */
static int my_memcasecmp(char const *s1, char const *s2, unsigned len) {
  unsigned char const* u1=(unsigned char const*)s1;
  unsigned char const* u2=(unsigned char const*)s2;
  int dif;
  while (len--!=0) {
    if (0!=(dif=TOLOWER(*u1)-TOLOWER(*u2))) return dif;
    u1++; u2++;
  }
  return 0;
}

charset_t charset_combine(charset_t cs1, charset_t cs2) {
  return cs1==cs2 ? cs1
       : 0 == strcmp(cs1, cs2) ? cs1
       : 0 == strcmp(cs1, CS_UNKNOWN) ? cs2
       : 0 == strcmp(cs2, CS_UNKNOWN) ? cs1
       : CS_CONFLICT; /* Imp: handle conflicting CS_UNSUPPORTED */
}

/** @return CS_ or CS_UNSUPPORTED (never CS_UNKNOWN) */
charset_t charset_by_name_len(char const *name, unsigned namelen) {
  char tmp[42], *p;
  char iso_p=0;
  if (namelen<sizeof(tmp)) {
    p=tmp; while (namelen--!=0 && *name!='\0') {
      if (*name=='-' || *name=='_') name++;
      else { *p++=TOLOWER(*name); name++; }
    }
    *p='\0';
    /* printf("tmp=(%s)\n", tmp); */
    if (0) {}
    else if (0==strcmp(tmp,"utf8")) return CS_UTF8;
    else if (0==strcmp(tmp,"utf8ws")) return CS_UTF8;
    else if (0==strcmp(tmp,"utf8bom")) return CS_UTF8;
    else if (0==strcmp(tmp,"ascii")) return CS_ASCII;
    else if (0==strcmp(tmp,"usascii")) return CS_ASCII;
    else if (0==strcmp(tmp,"c")) return CS_ASCII;
    else if (0==strcmp(tmp,"posix")) return CS_ASCII;
    else if (0==strcmp(tmp,"binary")) return CS_BINARY;
    else if (0==strcmp(tmp,"8bit")) return CS_BINARY;
    else if (0==strcmp(tmp,"8bitbinary")) return CS_BINARY;
    else if (0==strcmp(tmp,"utf16be")) return CS_UTF16BE;
    else if (0==strcmp(tmp,"utf16le")) return CS_UTF16LE;
    else if (0==strcmp(tmp,"utf16")) return CS_UTF16BE; /* Dat: (X)Emacs-compatible */
    else if (0==strcmp(tmp,"utf16bom")) return CS_UTF16BE; /* Dat: (X)Emacs-compatible */
    else if (0==strcmp(tmp,"utf16littleendian")) return CS_UTF16LE; /* (X)Dat: Emacs-compatible */
    else if (0==strcmp(tmp,"utf16littleendianbom")) return CS_UTF16LE; /* (X)Dat: Emacs-compatible */
    else if (0==strcmp(tmp,"utf16bigendian")) return CS_UTF16BE; /* (X)Dat: Emacs-compatible */
    else if (0==strcmp(tmp,"utf16bigendianbom")) return CS_UTF16BE; /* (X)Dat: Emacs-compatible */
    else if (0==strncmp(tmp,"utf16",5)) return CS_ERROR;
    else if (0==strcmp(tmp,"koi8r")) return CS_KOI8R;
    else if (0==strcmp(tmp,"koi8u")) return CS_KOI8U;
    else if (0==strcmp(tmp,"tis620")) return CS_TIS620;
    else if (0==strcmp(tmp,"620")) return CS_TIS620;
    else if (0==strcmp(tmp,"gb2312")) return CS_GB2312;
    else if (0==strcmp(tmp,"2312")) return CS_GB2312;
    else if (0==strcmp(tmp,"big5hkscs")) return CS_BIG5HKSCS;
    else if (0==strcmp(tmp,"hkscs")) return CS_BIG5HKSCS;
    else if (0==strcmp(tmp,"big5")) return CS_BIG5;
    else if (0==strcmp(tmp,"gbk")) return CS_GBK;
    else if (0==strcmp(tmp,"gb18030")) return CS_GB18030;
    else if (0==strcmp(tmp,"18030")) return CS_GB18030;
    else if (0==strcmp(tmp,"shiftjis")) return CS_SHIFTJIS;
    else if (0==strcmp(tmp,"shift_jis")) return CS_SHIFTJIS;
    else if (0==strcmp(tmp,"sjis")) return CS_SHIFTJIS;
    else if (0==strcmp(tmp,"eucjp")) return CS_EUCJP;
    else if (0==strcmp(tmp,"euckr")) return CS_EUCJP;

    p=tmp;
    iso_p=0;
    if (0==strncmp(tmp,"isolatin",8)) { iso_p=1; p=tmp+8; }
    else if (0==strncmp(tmp,"iso8859",7)) { iso_p=1; p=tmp+7; }
    else if (0==strncmp(tmp,"latin",5)) { iso_p=1; p=tmp+5; }
    else if (0==strncmp(tmp,"8859",4)) { iso_p=1; p=tmp+4; }
    if (iso_p) {
      /* ISO-8859-1 .. ISO-8859-19 */
      if ((p[0]>='1' && p[0]<='9') && p[1]=='\0') return charsets_latin[p[0] - '0'];
      if (p[0] == '1' && (p[1]>='0' && p[1]<='9') && p[2]=='\0') return charsets_latin[p[1] - '0' + 10];
    }
  }
  /* printf("(%s)\n", p); */
  return CS_UNSUPPORTED;
}

charset_t charset_by_name(char const *name) {
  return charset_by_name_len(name, strlen(name));
}


/** Detect charset in the 1st line (parsed by Emacs), e.g.
 * -*- foo; bar: baz; charset: utf-8 -*-
 */
static charset_t charset_detect_emacs_line1(char const *s) {
  char const *p, *q, *r;
  while (1) {
    while (*s!='\0' && *s!='\n' && (s[0]!='-' || s[1]!='*' || s[2]!='-')) s++;
    if (s[0]!='-') break;
    s+=3;
    q=s;
    while (*s!='\0' && *s!='\n' && (s[0]!='-' || s[1]!='*' || s[2]!='-')) s++;
    while (1) {
      /* Dat: now q...s contains an Emacs variable list */
      while (q!=s && ISWSPACE(*q)) q++;
      p=q;
      while (q!=s && ISEMACSNAME(*q)) q++;
      r=q;
      while (r!=s && ISWSPACE(*r)) r++;
      if (*r==':' && q-p==6 && 0==my_memcasecmp(p,"coding",6)) {
        r++; /* Dat: skip ':' */
        while (r!=s && ISWSPACE(*r)) r++;
        q=r;
        while (q!=s && !ISWSPACE(*q) && *q!=';' && *q!=',') q++; /* Imp: maybe accept quotes */
        return charset_by_name_len(r, q-r);
      }
      while (r!=s && *r!=';') r++;
      if (r==s) break;
      q=r+1;
    }
    if (s[0]!='-') break;
    s+=3;
  } while (*s++=='-');
  return CS_UNKNOWN;
}

/** Dat: case sensitive.
 * @param s e.g "foo='bar'   bar='baz'/>
 * @return attr value with `&lt;' instead of `<' etc.; or NULL if missing
 */
static char const* get_xml_attribute(char const *s, char const *attrname, unsigned *len_ret) {
  char const *p, *q, *r, *t;
  /* Imp: check that no '<' inside */
  while (1) {
    while (*s!='\0' && *s!='>' && !ISXMLNAME1(*s)) s++;
    if (!ISXMLNAME1(*s)) break;
    p=s++;
    while (ISXMLNAME2(*s)) s++;
    q=s;
    /* Dat: now the attr name is p...q */
    /* printf("attrname=(%s)%d\n", p, q-p); */
    while (ISWNZSPACE(*s)) s++;
    if (*s=='=') {
      s++;
      while (ISWNZSPACE(*s)) s++;
      if (*s=='"') {
        r=++s;
        while (*s!='\0' && *s!='>' && *s!='"') s++;
        if (*s!='"') break;
        t=s++;
        /* printf("r=(%s)\n", r); printf("t=(%s)\n", t); */
      } else if (*s=='\'') {
        r=++s;
        while (*s!='\0' && *s!='>' && *s!='\'') s++;
        if (*s!='\'') break;
        t=s++;
      } else {
        r=s;
        while (*s!='>' && !ISWSPACE(*s)) s++; /* Dat: also stops at '\0' */
        if (*s=='>' || *s=='\0') break;
        t=s;
      }
      /* Dat: now the attr value is r...t */
      if (0==strncmp(p, attrname, q-p)) {
        /* Dat: we do a vague check on the closing >; Imp: check for ?>, syntax etc. */
        while (1) {
          while (*s!='\0' && *s!='>' && *s!='"' && *s!='\'') s++;
          if (*s=='\0') return NULL; /* Dat: XML tag syntax error, not found */
          if (*s=='>') break;
          if (*s=='"') {
            while (*s!='\0' && *s!='>' && *s!='"') s++;
            if (*s++!='"') return NULL;
          } else if (*s=='\'') {
            while (*s!='\0' && *s!='>' && *s!='\'') s++;
            if (*s++!='\'') return NULL;
          }
        }
        *len_ret=t-r;
        return r;
      }
    } /* IF *s=='=' */
  }
  return NULL;
}

static charset_t charset_detect_xml(char const *s) {
  unsigned len;
  charset_t cs;
  while (ISWNZSPACE(*s)) s++;
  if (0==strncmp(s,"<?xml",5) && s[5]!='\0' && !ISXMLNAME2(s[5])) {
    if (NULL!=(s=get_xml_attribute(s+5,"encoding",&len))) {
      cs=charset_by_name_len(s, len);
      if (0 == strcmp(cs, CS_UTF16BE) ||
          0 == strcmp(cs, CS_UTF16LE)) return CS_ERROR; /* Dat: UTF-16 impossible here since already parsed as 8-bit */
      return cs;
    }
  }
  return CS_UNKNOWN;
}

/** Detects: HTML <meta http-equiv="content-type" content="text/html; charset=...">
 */
static charset_t charset_detect_html_meta(char const *s) {
  unsigned len;
  charset_t cs;
  char const *p;
 first_tag:
  while (ISWNZSPACE(*s)) s++;
  if (*s++!='<') goto unk;
  while (ISWNZSPACE(*s)) s++;
  if (*s=='!') { /* Dat: <!DOCTYPE */ /* Imp: more precise, only once */
    while (*s!='\0' && *s!='>') s++;
    if (*s++=='\0') goto unk;
    goto first_tag;
  }
  if (!ISXMLNAME1(*s)) goto unk;
  p=s++;
  while (ISXMLNAME2(*s)) s++;
  if (!ISWNZSPACE(*s) && *s!='>') goto unk;
  if (!((s-p==4 && 0!=my_memcasecmp(p, "html", 4)) || (s-p==4 && 0!=my_memcasecmp(p, "head", 4)))) goto unk;
  while (*s!='\0') {
    if (*s++!='<') continue;
    /* Imp: <script>, <style> */
    if (s[0]=='-' && s[1]=='-') { /* Dat: HTML comment */
      /* Dat: parse as HTML comment, not as SGML comment (SGML comment is: -- comment1 -- not_comment -- comment2 --) */
      s+=2;
      while (*s!='\0' && (s[0]!='-' || s[1]!='-' || s[2]!='>')) s++;
      if (*s=='\0') goto unk;
      s+=3; continue;
    }
    /* Imp: proper <?...?> and <%...%> */
    while (ISWNZSPACE(*s)) s++;
    if (!ISXMLNAME1(*s)) { /* Dat: e.g `</tag>', <?...> */
     skiptag:
      while (*s!='\0' && *s!='>') s++;
      if (*s=='\0') goto unk;
      s++; continue;
    }
    p=s++;
    while (ISXMLNAME2(*s)) s++;
    if (!ISWNZSPACE(*s) && *s!='>') goto unk; /* Dat: HTML syntax error in beginning-of-tag */
    if ((s-p==4 && 0==my_memcasecmp(p, "body", 4))
     || (s-p==2 && 0==my_memcasecmp(p, "h1", 2))
     || (s-p==2 && 0==my_memcasecmp(p, "br", 2))
     || (s-p==1 && 0==my_memcasecmp(p, "p", 1))
     ) goto unk; /* Dat: reached HTML body */
    if (!(s-p==4 && 0==my_memcasecmp(p, "meta", s-p))) goto skiptag;
    while (ISWNZSPACE(*s)) s++;
    /* printf("metatag=(%s)\n", s); */
    if (NULL!=(p=get_xml_attribute(s,"http-equiv",&len)) || NULL!=(p=get_xml_attribute(s,"HTTP-EQUIV",&len))) {
      /* Dat: we keep `<' as `&lt;' etc., no problem */
      if (len==7 && 0==my_memcasecmp(p, "charset", len)
       && (NULL!=(p=get_xml_attribute(s,"content",&len)) || NULL!=(p=get_xml_attribute(s,"CONTENT",&len)))
         ) {
        cs=charset_by_name_len(p, len);
       gotcs:
        if (0 == strcmp(cs, CS_UTF16BE) ||
            0 == strcmp(cs, CS_UTF16LE)) return CS_ERROR; /* Dat: UTF-16 impossible here since already parsed as 8-bit */
        return cs;
      }
      if (len==12 && 0==my_memcasecmp(p, "content-type", len)
       && (NULL!=(p=get_xml_attribute(s,"content",&len)) || NULL!=(p=get_xml_attribute(s,"CONTENT",&len)))
         ) {
        while (1) {
          while (len!=0 && *p!=';') { p++; len--; }
          if (len==0) break;
          p++; len--;
          while (len!=0 && ISWNZSPACE(*p)) { p++; len--; }
          if (len>8 && 0==my_memcasecmp(p, "charset=", 8)) {
            unsigned i=0;
            p+=8; len-=8;
            while (i<len && p[i]!=';') i++;
            cs=charset_by_name_len(p, i);
            goto gotcs;
          }
        }
      }
    }
    goto skiptag;
  }
 unk:
  return CS_UNKNOWN;
}

/** @return CS_BINARY or CS_UNKNOWN */
charset_t charset_detect_binary(register char const *s, unsigned slen) {
  if (slen>31 && s[0]=='\177' && s[1]=='E' && s[2]=='L' && s[3]=='F') return CS_BINARY; /* Dat: elf exe, .so, .o */
  /* Dat: .exe: If the word value at offset 18h is 40h or greater, the word value at 3Ch is typically an offset to a Windows header (starting with "NE"). */
  if (slen>255 && s[0]=='M' && s[1]=='Z' && 3[(unsigned char const*)s]<2) return CS_BINARY; /* Dat: DOS/Windows .exe, .dll; s[24]=='@': Windows */
  if (slen>2 && s[0]=='\0' && s[1]!='\0' && s[2]!='\0') return CS_BINARY; /* Dat: e.g. SICStus .ql file */
  if (slen>4 && s[0]=='A' && s[1]=='2' && s[2]=='\0' && s[3]!='\0' && s[4]!='\0') return CS_BINARY; /* Dat: e.g. SICStus .ql file */
  if (slen>8 && s[0]=='\037' && s[1]=='\x8b') return CS_BINARY; /* Dat: gzip */
  if (slen>8 && s[0]=='B' && s[1]=='Z' && s[2]=='h') return CS_BINARY; /* Dat: bzip2 */
  if (slen>8 && s[0]=='R' && s[1]=='a' && s[2]=='r' && s[3]=='!') return CS_BINARY; /* Dat: RAR compressed */
  if (slen>8 && s[0]=='P' && s[1]=='K' && s[2]=='\003' && s[3]=='\004') return CS_BINARY; /* Dat: ZIP compressed */
  if (slen>8 && s[0]=='P' && s[1]=='K' && s[2]=='\005' && s[3]=='\006') return CS_BINARY; /* Dat: ZIP compressed, empty ZIP file */
  if (slen>31 && s[0]=='%' && s[1]=='P' && s[2]=='D' && s[3]=='F' && s[4]=='-') return CS_BINARY; /* Dat: PDF */
  if (slen>8 && s[0]=='\377' && s[1]=='\330' && s[2]=='\377') return CS_BINARY; /* Dat: JPEG */ /* Imp: allow more '\377' at the beginning */
  if (slen>8 && s[0]=='G' && s[1]=='I' && s[2]=='F' && s[3]=='8' && (s[4]=='9' || s[4]=='7') && s[5]=='a') return CS_BINARY; /* Dat: GIF */
  if (slen>8 && s[0]=='\211' && s[1]=='P' && s[2]=='N' && s[3]=='G' && s[4]=='\r' && s[5]=='\n' && s[6]=='\032' && s[7]=='\n') return CS_BINARY; /* Dat: PNG */
  if (slen>31 && s[0]=='#' && s[1]=='!') {
    /* Dat: SICStus .sav and .po start with a #!-line, without any charset spec */
    char const*p=s, *pend=s+slen;
    /* Dat: 1st line: "#! /bin/sh\n" */
    while (p!=pend && *p!='\n') p++;
    if (p!=pend) p++;
    if (pend-p>16 && 0==memcmp(p,"exec sicstus -r ",16)) return CS_BINARY; /* Dat: SICStus .sav and .po files */
  }
  /* Imp: more formats */
  return CS_UNKNOWN;
}

/** @param s null-terminated string at the beginning of the file
 * @param ignore_ret; the number of characters to be ignored at the beginning
 * of the file
 */
charset_t charset_detect(register char const *s, unsigned *ignore_ret) {
  charset_t cs1=CS_UNKNOWN;
  *ignore_ret=0;
  if (s[0]=='\xFE' && s[1]=='\xFF'                ) { *ignore_ret=2; return CS_UTF16BE; } /* Dat: no more check */
  if (s[0]=='\xFF' && s[1]=='\xFE'                ) { *ignore_ret=2; return CS_UTF16LE; } /* Dat: no more check */
  if (s[0]=='\xEF' && s[1]=='\xBB' && s[2]=='\xBF') { *ignore_ret=3; cs1=CS_UTF8; }
  cs1=charset_combine(cs1, charset_detect_emacs_line1(s));
  cs1=charset_combine(cs1, charset_detect_xml(s));
  cs1=charset_combine(cs1, charset_detect_html_meta(s));
  return cs1;
  /* TODO(pts): maybe LaTeX \usepackage[latin2]{inputenc} */
  /* Imp: maybe Prolog :- charset(...). */
  /* Imp: maybe MIME-header Charset:, Content-type: */
  /* Imp: Perl use utf8; etc. */
  /* Imp: Java, Python encoding? */
  /* Imp: vi charset detection */
}


/* We can assume that buf is null-terminated, i.e. buf[len] == '\0' */
struct charmap *guess_map(unsigned char *buf, int len)
{
	charset_t cs;
	unsigned ignore_ret;
	struct charmap *guessed_map;

	/* No info? Use default */
	if (!len || !(locale_map->type ? guess_utf8 : guess_non_utf8))
		return locale_map;

	/* SUXX: On a non-UTF-8 terminal, "ascii" is an equivalent of
	 * locale_map, there is no conversion.
	 */
	cs = charset_detect((char const*)buf, &ignore_ret);
	if (0 == strcmp(cs, CS_UNSUPPORTED))  /* Detected but unsupported. */
		return find_charmap(USTR "ascii");
	if (0 == strcmp(cs, CS_UNKNOWN))  /* Not detected. */
		return locale_map;
	/* This can become NULL, especially if 0 == strcmp(cs, CS_UNKNOWN). */
	guessed_map = find_charmap((unsigned char*)cs);
	if (guessed_map == NULL)
		return locale_map;
	return guessed_map;
}
