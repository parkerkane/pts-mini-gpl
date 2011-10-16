#ifndef _Ii18n
#define _Ii18n 1

int joe_iswupper PARAMS((struct charmap *,int c));
int joe_iswlower PARAMS((struct charmap *,int c));

int joe_iswalpha PARAMS((struct charmap *,int c));	/* or _ */
int joe_iswalpha_ PARAMS((struct charmap *,int c));

int joe_iswalnum PARAMS((struct charmap *,int c));
int joe_iswalnum_ PARAMS((struct charmap *,int c));

int joe_iswdigit PARAMS((struct charmap *,int c));
int joe_iswspace PARAMS((struct charmap *,int c));
int joe_iswctrl PARAMS((struct charmap *,int c));
int joe_iswpunct PARAMS((struct charmap *,int c));
int joe_iswgraph PARAMS((struct charmap *,int c));
int joe_iswprint PARAMS((struct charmap *,int c));
int joe_iswxdigit PARAMS((struct charmap *,int c));
int joe_iswblank PARAMS((struct charmap *,int c));

/* Returns printed width of control and other non-printable characters.
 * Modified for JOE.
 * Always returns a positive integer.
 * Looking for wswidth? Take a look at scrn.c/txtwidth()
 */
int joe_wcwidth PARAMS((int wide,int c));

int joe_towupper PARAMS((struct charmap *,int c));
int joe_towlower PARAMS((struct charmap *,int c));

/* Returns bool.
 * Returns true if c is a control character which should not be displayed.
 */
int unictrl PARAMS((int c));

#endif
