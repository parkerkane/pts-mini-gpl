This directory contains joe-p37, and improved version of the text editor JOE
(http://joe-editor.sourceforge.net/) v3.7.

Improvements were started by Péter Szabó <pts@fazekas.hu> on 2008-11-08.

Both JOE and joe-p37 are under the GPL.

Most important improvements in joe-p37 over JOE:

* It can use the rightmost column of the screen. Try it with
  `./joe -nosys -nouser' on an xterm, linux or rxvt terminal.
* Safe terminal handling: never writes characters to the terminal which
  garble the screen.
* Correct Unicode, UTF-8 and encoding handling.
* Many segfault bugfixes.
* Many small usability improvements.
* Some performance improvements.

Limitations of joe-p37:

* Only Linux is supported now. Mac OS X may be added in the future.
  The compiled binary runs on FreeBSD too.

Compilation:

* Run

    CC=gcc ./c.sh

* Try it with:

    ./joe -nosys -nouser

* If that works, you may also try it without -nosys and -nouser.

Some techical information
~~~~~~~~~~~~~~~~~~~~~~~~~
Accompanying files (e.g. joerc, documentation, syntax highlight
definitions, termcap definition, autoconf scripts) should be put into
../joe-p37extra.

---

* dofollows(): scroll visible windows to cursor
* Fast status line: `keepup'
* uduptw() "dupw" duplicate current window to an offscreen window
* doedit1(): duplicate current window to an offscreen window, load a different
  buffer to the current window
* How to edit ~/.joe_state? Not with joe...
* Commands setmark and gommark are per-file.
* nextpos/prevpos are global, but updated weirdly (keeps only last 2?)
* correct: pfwrd(p,long n); Move forward n bytes
* correct: pbkwd(p,long n); Move backward n bytes
* if (b != bufs.link.next) abort();  /* post-condition of bnext() */
* if (b != bufs.link.prev) abort();  /* post-condition of bprev() */
* bufs.link contains the buffers in reverse order as bnext()/bprev()
