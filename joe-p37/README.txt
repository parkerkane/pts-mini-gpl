This directory contains joe-p37, and improved version of the text editor JOE
(http://joe-editor.sourceforge.net/) version 3.7.

Improvements were started by Péter Szabó <pts@fazekas.hu> on 2008-11-08.

Both JOE and joe-p37 are under the GPL.

Most important improvements in joe-p37 over JOE:

* It can use the rightmost column of the screen. Try it with
  `./joe -nosys -nouser' on an xterm, linux or rxvt terminal.
* Safe terminal handling: never writes characters to the terminal which
  garble the screen.
* Correct Unicode, UTF-8 and encoding handling.
* Consistent buffer ordering in single window mode: `^K q' inserts the newly
  opened file after the current buffer; `^K q' returns to the previous
  buffer; the relative order of the unaffected buffers are always preserved.
* Many segfault bugfixes.
* Many small usability improvements.
* Some performance improvements.
* Supports compilation with uClibc (for small, portable, statically linked
  binaries).

Limitations of joe-p37:

* Only Linux (tested on Ubuntu Lucid, should work on any Linux since 2005)
  and Mac OS X (tested on 10.5 Leopard) are supported now.  The compiled
  Linux binary runs on FreeBSD too.

Compilation and tryout:

* Compile it with (without the leading $):

    $ CC=gcc ./c.sh

* Try it with:

    $ ./joe

  Notice that joe-p37 prints to the rightmost column of the screen, but
  original JOE 3.7 doesn't.

* Try it with only the built-in configs (i.e. /etc/joe/joerc and ~/.joe-p37/rc
  are not read):

    $ ./joe -nosys -nouser

* If you have config files, place them like this:

    ~/.joe-p37/rc
    ~/.joe-p37/syntax/python.jsf
    ~/.joe-p37/charmaps/klingon
    ~/.joe-p37/lang/de.po

  Get the defaults of these config files from the original JOE 3.7
  distribution:

    http://downloads.sourceforge.net/project/joe-editor/JOE%20sources/joe-3.7/joe-3.7.tar.gz

  Please note that joe-p37 works reasonably well without any config files.
  The most important feature you will miss is syntax highlighted, because
  that has to be loaded from external .jsf files.

__EOF__
