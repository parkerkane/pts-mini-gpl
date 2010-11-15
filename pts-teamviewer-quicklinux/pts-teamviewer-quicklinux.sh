#! /bin/bash --
#
# pts-teamviewer-quicklinux.sh: TeamViewer QuickSupport alternative for Ubuntu
# by pts@fazekas.hu at Mon Nov 15 14:18:22 CET 2010
#
# This script has been tested on Ubuntu Lucid, but it should work on earlier
# Ubuntu versions as well.
#
# Example invocation from any desktop:
#
#   wget -O- http://pts-mini-gpl.googlecode.com/svn/trunk/pts-teamviewer-quicklinux/pts-teamviewer-quicklinux.sh | bash
#   goo.gl/k9imm
#

if test "$0" = bash || test "$0" = sh || test "$0" = dash; then
  # The script is piped, save it to /tmp/...
  # This snippet works with bash and dash and zsh.
  set -ex
  TMPDIR="/tmp/pts_teamviewer_quicklinux--$HOSTNAME--$(id -nu)"
  mkdir -p "$TMPDIR"
  SCRIPT="$TMPDIR/quicklinux.sh"
  if test "${BASH%/bash}" = "$BASH" || ! test -x "$BASH"; then
    BASH="$(type bash)" && BASH="${BASH#bash is }"
    test -x "$BASH"
  fi
  (echo "#!$BASH"; echo "# Auto-generated on $(LC_TIME=C date)") >"$SCRIPT"
  chmod +x "$SCRIPT"
  cat /dev/fd/0 >>"$SCRIPT"
  exec "$SCRIPT"
fi

if test "$1" != --no-xterm; then
  echo "Starting the TeamViewer QuickLinux launcher in an xterm"
  xterm -T "Portable TeamViewer QuickLinux launcher" \
       -bg '#fdd' -fg black \
       -e "$BASH" "$0" --no-xterm &
  exit
fi

trap 'X=$?; set +x
echo "Exit code: $X"
if test "$X" != 0 && test "$X" != 143 && test "$X" != 137; then  # 143: kill -TERM
echo "Press <Enter> to close this window."; read
fi' EXIT

set -ex

: Starting TeamViewer

TMPDIR="/tmp/pts_teamviewer_quicklinux--$HOSTNAME--$(id -nu)"
mkdir -p "$TMPDIR"
cd "$TMPDIR"

echo "DISPLAY=$DISPLAY"
echo "XAUTHORITY=$XAUTHORITY"
xwininfo -root  # Check the X11 connection (should be already checked by xterm)

killall TeamViewer.exe || true

U=" $(uname -a) "
if test "${U/ x86_64 /}" != "$U"; then
  DEB=teamviewer_linux_x64.deb
else
  DEB=teamviewer_linux.deb
fi

if ! test -f "$DEB.extracted"; then
  if ! test -f "$DEB.downloaded"; then
    rm -f "$DEB"
    wget -O "$DEB" http://www.teamviewer.com/download/"$DEB"
    true >>"$DEB.downloaded"
  fi
  ar p teamviewer_linux_x64.deb data.tar.gz | tar xz
  true >>"$DEB.extracted"
fi

<./opt/teamviewer/teamviewer/5/bin/wrapper \
>./opt/teamviewer/teamviewer/5/bin/wrapper2 \
grep -vE '/winelog|WINEPREFIX='

export WINEPREFIX="$PWD"
"$BASH" ./opt/teamviewer/teamviewer/5/bin/wrapper2 \
    'c:\Program Files\TeamViewer\Version5\TeamViewer.exe'