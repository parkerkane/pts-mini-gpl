/*
 * preproc.h: .joerc preprocessor
 * by pts@fazekas.hu at Tue Oct  3 11:19:48 CEST 2006
 *
 * Entry point for each line: preproess_rc_line()
 */

/** like strstr(), but slow and case sensitive. No advanced preconditioning
 * (like B--Moore) or such.
 */
unsigned char* strfixstr(unsigned char *s1, unsigned char *needle, int needle_len)
{
  int l1;
  if (!needle_len) return (unsigned char*) s1;
  if (needle_len==1) return zchr(s1, (char)needle[0]);
  l1 = zlen(s1);
  while (l1 >= needle_len) { /* Imp: index(s1, needle[0]) here, as in pts-ftpd */
    l1--;
    if (0==memcmp(s1,needle,needle_len)) return (unsigned char *) s1;
    s1++;
  }
  return NULL;
}

// strglobmatch("almamxy","?lmam*??")

/**
 * @param glob
 * pattern may cotain `*'s (0 or more any character) and `?'s (a single
 * any character) as metacharacters.
 * @return true iff string `str' matches the glob pattern `glob'.
 */
int strglobmatch(unsigned char *str, unsigned char *glob)
{
  /* Test: strglobmatch("almamxyz","?lmam*??") */
  int min;
  while (glob[0]!='\0') {
    if (glob[0]!='*') {
      if ((glob[0]=='?') ? (str[0]=='\0') : (str[0]!=glob[0])) return 0;
      glob++; str++;
    } else { /* a greedy search is adequate here */
      min=0;
      while (glob[0]=='*' || glob[0]=='?') min+= *glob++=='?';
      while (min--!=0) if (*str++=='\0') return 0;
      min=0; while (glob[0]!='*' && glob[0]!='?' && glob[0]!='\0') { glob++; min++; }
      if (min==0) return 1; /* glob ends with star */
      if (NULL==(str=strfixstr(str, glob-min, min))) return 0;
      str+=min;
    }
  }
  return str[0]=='\0';
}


/**** pts ****/
/** Finds the end of the paren-expression. Uses ')' and ',' as trailers.
 * @return NULL if end too early, otherwise char after last paren
 * @example find_expr_end("foo()bar)zz")=="zz"
 */
unsigned char *find_expr_end(unsigned char *bb) {
  unsigned depth;
  unsigned char c;
  depth=1;
  while (depth>0) {
    if ((c=*bb++)=='\0') {
      return NULL;
    } else if (c=='(') {
      depth++;
    } else if (c==')') {
      if (--depth==0) break;
    } else if (c==',' && depth==1) {
      break;
    } else if (c=='\\') {
      if (*bb!='\0') bb++;
    } else if (c=='"') {
      while (1) {
        if ((c=*bb++)=='\\') {
          if (*bb!='\0') bb++;
        } else if (c=='"') {
          break;
        } else if (c=='\0') {
          return NULL;
        }
      }
    }
  }
  return bb;
}

/**** pts ****/
/** Evaluates preprocessor expression starting at ba, puts the result to
 * *pp...pend, increments *pp. Can call itself recursively.
 * @return past the end of the command
 */
unsigned char *preprocess_expr(unsigned char *ba, unsigned char **pp,
 unsigned char *pend, int *errp,
 unsigned char *fname, unsigned fline)
{
  unsigned char *bb, *bc, *p=*pp, *q, *r;
  unsigned cmdlen;
  /** 0: keep intact, 1: negate, 2: convert to "" or "1" */
  int negc=0;
  while (*ba=='!') { ba++; if (3==++negc) negc=1; }
  if (NULL==(bb=find_expr_end(ba))) {
    fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Missing expression end $(...)")), fname, fline);
    *errp=1; goto ret;
  }
  cmdlen=bb-ba-1;
  /* vvv Dat: don't touch pp! */
  if (0==zncmp((unsigned char*)"answer()",ba,cmdlen)) { /* 42 */
    if (p!=pend) *p++='4';
    if (p!=pend) *p++='2';
  } else if (0==zncmp((unsigned char*)"concat(",ba,7)) { /* evaluate its aguments and concatenate them */
    ba+=7;
    while ((ba=preprocess_expr(ba,&p,pend,errp,fname,fline))[-1]==',') {}
  } else if (0==zncmp((unsigned char*)"getenv(",ba,7)) {
    unsigned char *s;
    q=p;
    ba=preprocess_expr(ba+7,&p,pend,errp,fname,fline);
    /* Imp: error if ba[-1]==',' */
    if (p!=pend) {
      *p='\0';
      if (0==zcmp(q,(unsigned char*)"3.7")) s=(unsigned char*)"1"; /* we have joe-p37 */
      else if (NULL==(s=(unsigned char*)getenv((char*)q))) s=(unsigned char*)"";
      /* fprintf(stderr, "getenv(%s)=%s.\n", q, s); */
      p=q; /* remove variable name */
      while (*s!='\0') { /* append s to results */
        if (p!=pend) *p++=*s;
        s++;
      }
    }
  } else if (0==zncmp((unsigned char*)"iseq(",ba,5)) {
    q=p;
    ba=preprocess_expr(ba+5,&p,pend,errp,fname,fline);
    r=p;
    if (ba[-1]==',') ba=preprocess_expr(ba,&p,pend,errp,fname,fline);
    if (p-r==r-q && 0==memcmp(q,r,r-q)) {
      p=q;
      if (p!=pend) *p++='1';
    } else p=q;
  } else if (0==zncmp((unsigned char*)"isglob(",ba,7)) { /* isglob(str,glob) => "" or "1" */
    q=p;
    ba=preprocess_expr(ba+7,&p,pend,errp,fname,fline);
    if (p!=pend) {
      *p++='\0';
      r=p;
      if (ba[-1]==',') ba=preprocess_expr(ba,&p,pend,errp,fname,fline);
      if (p!=pend) {
        *p='\0';
        if (strglobmatch(q, r)) *q++='1'; /* Dat: we have free space for 1 character here */
      }
    }
    p=q; /* clear temporary area */
  } else if (0==zncmp((unsigned char*)"if(",ba,3)) { /* quoted string, e.g. q(fo,o\(bar) -> fo,o(bar */
    q=p;
    ba=preprocess_expr(ba+3,&p,pend,errp,fname,fline);
    if (ba[-1]!=',') { /* missing if and else branches */
      /* Imp: display error */
    } else if (q==p) { /* if condition result is an empty string => `else' branch */
      /*fprintf(stderr, "fe1 %s.\n", ba);*/
      ba=find_expr_end(ba); /* skip `then' branch */
      /*fprintf(stderr, "fe2 %s.\n", ba);*/
      p=q; /* remove output of condition (Dat: redundant for empty condition result) */
      if (ba[-1]==',') { /* evaluate `else' branch */
        preprocess_expr(ba,&p,pend,errp,fname,fline);
      }
    } else {
      p=q; /* remove output of condition */
      preprocess_expr(ba,&p,pend,errp,fname,fline);
    }
  } else if (0==zncmp((unsigned char*)"or(",ba,3)) { /* logical `or' with lazy evaluation. "" or "1" */
    q=p; ba+=3;
    while (1) {
      r=p; ba=preprocess_expr(ba,&p,pend,errp,fname,fline);
      if (p!=r) { /* true (non-empty) return value */
#if 0
        if (p!=pend) *q++='1'; /* convert value to 1 */
#else
        /* fprintf(stderr, "\nL=(%d) S=(%s)\n", r, p-r); */
        if (p-r>pend-p) p=(pend-p)+r;
        memmove(q,r,p-r); q+=p-r; /* keep value */
#endif
        break; /* lazy evaluation */
      }
      if (ba[-1]!=',') break; /* empty result */
    }
    p=q;
  } else if (0==zncmp((unsigned char*)"and(",ba,4)) { /* logical `and' with lazy evaluation. "" or "1" */
    q=p; ba+=4;
    while (1) {
      r=p; ba=preprocess_expr(ba,&p,pend,errp,fname,fline);
      if (ba[-1]!=',') { /* no false branches found => return true */
#if 0
        if (p!=pend) *q++=='1';
#else
        if (p-r>pend-p) p=(pend-p)+r;
        memmove(q,r,p-r); q+=p-r; /* keep value */
#endif
        break;
      }
      if (p==r) { /* false (empty) branch => return false (empty) */
        break; /* lazy evaluation */
      }
    }
    p=q;
  } else if (0==zncmp((unsigned char*)"qq(",ba,3)) { /* return quoted string, e.g. q(fo,o\(bar) -> fo,o(bar */
    ba+=3; bc=ba;
    while (NULL!=(bc=find_expr_end(bc)) && bc[-1]==',') {}
   do_qq:
    if (bc==NULL) bc=bb;
    bc--;
    while (ba!=bc) {
      if (*ba=='\\' && ba[1]!='\0') ba++; /* remove quote */ /* Imp: \t, \x etc. */
      if (p!=pend) *p++=*ba;
      ba++;
    }
  } else if (*ba=='"'+0U) { /* return quoted string, e.g. q(fo,o\(bar) -> fo,o(bar */
    bc=ba;
    while (NULL!=(bc=find_expr_end(bc)) && bc[-1]==',') {} /* always bc[-1]=='"' */
    ba++;
    /* fprintf(stderr, "\nL=%d S=(%s)\n",bc-ba,ba); sleep(5); */
    goto do_qq;
  } else if (*ba=='('+0U) { /* concatenate */
    /* Dat: see also concat(..) */
    ba++;
    while ((ba=preprocess_expr(ba,&p,pend,errp,fname,fline))[-1]==',') {}
  } else {
    unsigned char tmp[64], *tp=tmp, *tend1=tmp+sizeof tmp-1, *bp=ba;
    while (tp!=tend1 && *bp!='(' && *bp!='\0' && *bp!=')') *tp++=*bp++;
    *tp='\0';
    /* fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Unknown preprocessor function: %s")), fname, fline, tmp); */
    *errp=1;
  }
 ret:
  /* Dat: now expression is between ba...bb, bb[-1]=')' or bb[-1]==',' */
  if (negc==1) {
    /* negation: empty string => "1", otherwise empty */
    if (*pp==p && p!=pend) *(*pp)++='1';
  } else if (negc==2) {
    if (*pp!=p) *(*pp)++='1';
  } else {
    *pp=p;
  }
  return bb;
}

/**
 * Modifies line buffer containing a joe configuration line in place.
 * 
 * @param buf,buflen contain one joerc file line (terminated with "\n\0").
 *   Will be modified in place.
 * @param tmp,tmplen temporary buffer
 * @return 0 on OK, 1 on err
 */
int preprocess_rc_line(unsigned char *buf, unsigned buflen,
 unsigned char *tmp, unsigned tmplen,
 unsigned char *fname, unsigned fline) {
  /* Imp: doc: env var too long */
  /* Imp: early check overflow */
  int err=0;
  unsigned char *bufend=buf+zlen(buf), *p, *pend, *ba, *bb, *s;
  if (bufend!=buf && bufend[-1]!='\n' && bufend!=buf+buflen-1) {
    *bufend++='\n'; *bufend='\0';
  }
  /* vvv Return if line still not ending with '\n' */
  if (bufend==buf || bufend[-1]!='\n') return 0;
  /* vvv Return if line doesn't start with preprocessor character '$' */
  if (buf[0]!='$') return 0;
  /* TODO(pts) dynamically allocatte tmp (possibly also buf) */
  p=tmp; pend=p+tmplen;
  ba=buf;
  while (1) {
    while (*ba!='\0' && (ba[0]!='$' || ba[1]!='(')) {
      if (p == pend)
        goto done;
      *p++=*ba;
      ba++;
    }
    if (*ba=='\0') break;
    if (ba[0]!='$' || ba[1]!='(') continue;
    ba+=2;
    if (*ba==')' || *ba=='\0') { /* $() -> empty string */
      ba++;
    } else if (ba[1]==')') { /* $(C) -> C */
      if (p == pend)
        goto done;
      *p++=ba[0];
      ba+=2;
    } else {
      s=(unsigned char*)"";
      bb=ba;
      while (*bb+0U=='_'+0U || (*bb-'a'+0U<='z'+0U) || (*bb-'A'+0U<='Z'+0U) ||
        (*bb-'0'+0U<='9'+0U)) bb++;
      if (*bb==')') { /* $(CX) -> getenv(CX) */
        /* Dat: modifies buf in place */
        *bb='\0';
        /* Dat: no support for $(3.7) here, it would be a syntax error in
         * joe-p35
         */
        if (NULL==(s=(unsigned char*)getenv((char*)ba))) s=(unsigned char*)"";
        *bb=')'; ba=bb+1;
      } else if (*bb!='(') {
        fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Bad in $(...)")), fname, fline);
        err=1; break;
      } else { /* expression: find nested... */
        do {
          ba=preprocess_expr(ba,&p,pend,&err,fname,fline);
        } while (ba[-1]==',');
        if (p == pend)
          goto done;
      }          
      while (*s!='\0') { /* append s to results */
        if (p == pend)
          goto done;
        *p++=*s;
        s++;
      }
    }
  } /* WHILE */
 done:
  if (p == pend) {
    fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Preprocessed line too long, truncated")), fname, fline);
    p[-1]='\0'; err=1;
  }
  *p='\0';
  zcpy(buf,tmp);
  /*fprintf(stderr, "buf=(%s)\n", buf);*/
  return err;
}

