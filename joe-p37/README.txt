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
  The most important feature you will m

__EOF__
