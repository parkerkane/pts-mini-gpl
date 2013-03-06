#! /bin/bash --
#
# pts-teamviewer-quicklinux.sh: TeamViewer QuickSupport alternative for Ubuntu
# by pts@fazekas.hu at Mon Nov 15 14:18:22 CET 2010
#
# This script has been tested on Ubuntu Lucid, but it should work on earlier
# Ubuntu versions as well.
#
# Invocation instructions for for Ubuntu users:
#
# 1. Press Alt-<F2>.
#
# 2. In the window appearing, type (or copy-paste) the following, and press
#    <Enter>.
#
#      bash -c 'wget -O- goo.gl/k9imm | bash'
#
#    Please note that the ``O'' between the dashes is a capital O. All other
#    characters are lowercase.
#
# 3. Wait for a window with a yellow background to appear.
#
# 4. Wait a few seconds untile TeamViewer is downloaded and started.
#
# 5. In the ``TeamViewer'' window appearing, find the 9-digit ID and 4-digit
#    Password fields (with blue background), and tell them to me.
#
# ----
#
# Longer invocation:
#
#   wget -O- http://pts-mini-gpl.googlecode.com/svn/trunk/pts-teamviewer-quicklinux/pts-teamviewer-quicklinux.sh | sh

if test "$0" = bash || test "$0" = sh || test "$0" = dash ||
   test "$0" = zsh; then
  # The script is not available as a regular file, so save it.
  doit() {
    # This snippet works with bash and dash and zsh.
    set -ex
    TMPDIR="/tmp/pts_teamviewer_quicklinux--$HOSTNAME--$(id -nu)"
    mkdir -p "$TMPDIR"
    SCRIPT="$TMPDIR/quicklinux.sh"
    if test "${BASH%/bash}" = "$BASH" || ! test -x "$BASH"; then
      BASH="$(type bash)" && BASH="${BASH#bash is }"
      test -x "$BASH"
    fi
    ( echo "#!$BASH"; echo "# Auto-generated on $(LC_TIME=C date)"
      cat) >"$SCRIPT"
    chmod +x "$SCRIPT"
    exec "$SCRIPT"
  }
  DOIT=doit
else
  DOIT='source /proc/self/fd/0'
fi

$DOIT <<'END'

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
  ar p "$DEB" data.tar.gz | tar xz
  true >>"$DEB.extracted"
fi

<./opt/teamviewer/teamviewer/7/bin/wrapper \
>./opt/teamviewer/teamviewer/7/bin/wrapper2 \
grep -vE "/winelog|WINEPREFIX="

export WINEPREFIX="$PWD"
"$BASH" ./opt/teamviewer/teamviewer/7/bin/wrapper2 \
    "c:\\Program Files\\TeamViewer\\Version7\\TeamViewer.exe"

END
