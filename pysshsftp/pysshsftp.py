#! /usr/bin/python2.4
# by pts@fazekas.hu at Sat Oct 30 17:28:18 CEST 2010

"""pysshsftp: Python SFTP client using the ssh(1) command.

pysshsftp is a proof-of-concept, educational SFTP client for Unix, which uses
the OpenSSH ssh(1) command-line tool (for establishing the secure
connection), but it doesn't use the sftp(1) command-line tool (so it can
e.g. upload files without truncating them first).

Only very little part of the SFTP protocol has been implemented so far
(initialization, uploading, and stat()ting files). The SFTP protocol was
reverse-engineered from sftp-client.c in the OpenSSH 5.1 source code.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of   
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

TODO(pts): Convert some assertions to I/O errors or parse errors.
"""

__author__ = 'pts@fazekas.hu (Peter Szabo)'

import errno
import fcntl
import os
import os.path
import select
import struct
import subprocess
import sys

# --- constants based on sftp.h,v 1.9 2008/06/13
#
# draft-ietf-secsh-filexfer-01.txt

# version
SSH2_FILEXFER_VERSION = 3

# client to server
SSH2_FXP_INIT = 1
SSH2_FXP_OPEN = 3
SSH2_FXP_CLOSE = 4
SSH2_FXP_READ = 5
SSH2_FXP_WRITE = 6
SSH2_FXP_LSTAT = 7
SSH2_FXP_STAT_VERSION_0 = 7
SSH2_FXP_FSTAT = 8
SSH2_FXP_SETSTAT = 9
SSH2_FXP_FSETSTAT = 10
SSH2_FXP_OPENDIR = 11
SSH2_FXP_READDIR = 12
SSH2_FXP_REMOVE = 13
SSH2_FXP_MKDIR = 14
SSH2_FXP_RMDIR = 15
SSH2_FXP_REALPATH = 16
SSH2_FXP_STAT = 17
SSH2_FXP_RENAME = 18
SSH2_FXP_READLINK = 19
SSH2_FXP_SYMLINK = 20

# server to client
SSH2_FXP_VERSION = 2
SSH2_FXP_STATUS = 101
SSH2_FXP_HANDLE = 102
SSH2_FXP_DATA = 103
SSH2_FXP_NAME = 104
SSH2_FXP_ATTRS = 105

SSH2_FXP_EXTENDED = 200
SSH2_FXP_EXTENDED_REPLY = 201

# attributes
SSH2_FILEXFER_ATTR_SIZE = 0x00000001
SSH2_FILEXFER_ATTR_UIDGID = 0x00000002
SSH2_FILEXFER_ATTR_PERMISSIONS = 0x00000004
SSH2_FILEXFER_ATTR_ACMODTIME = 0x00000008
SSH2_FILEXFER_ATTR_EXTENDED = 0x80000000

# portable open modes
SSH2_FXF_READ = 0x00000001
SSH2_FXF_WRITE = 0x00000002
SSH2_FXF_APPEND = 0x00000004
SSH2_FXF_CREAT = 0x00000008
SSH2_FXF_TRUNC = 0x00000010
SSH2_FXF_EXCL = 0x00000020

# statvfs@openssh.com f_flag flags
SSH2_FXE_STATVFS_ST_RDONLY = 0x00000001
SSH2_FXE_STATVFS_ST_NOSUID = 0x00000002

# status messages
SSH2_FX_OK = 0
SSH2_FX_EOF = 1
SSH2_FX_NO_SUCH_FILE = 2
SSH2_FX_PERMISSION_DENIED = 3
SSH2_FX_FAILURE = 4
SSH2_FX_BAD_MESSAGE = 5
SSH2_FX_NO_CONNECTION = 6
SSH2_FX_CONNECTION_LOST = 7
SSH2_FX_OP_UNSUPPORTED = 8
SSH2_FX_MAX = 8

# ---

# openssh 5.1 sftp-server(1) doesn't accept longer messages.
SFTP_MAX_MSG_LENGTH = 256 * 1024

FX_STATUS_MESSAGES = {
    SSH2_FX_OK: 'No error',
    SSH2_FX_EOF: 'End of file',
    SSH2_FX_NO_SUCH_FILE: 'No such file or directory',
    SSH2_FX_PERMISSION_DENIED: 'Permission denied',
    SSH2_FX_FAILURE: 'Failure',
    SSH2_FX_BAD_MESSAGE: 'Bad message',
    SSH2_FX_NO_CONNECTION: 'No connection',
    SSH2_FX_CONNECTION_LOST: 'Connection lost',
    SSH2_FX_OP_UNSUPPORTED: 'Operation unsupported',
}


class SftpStatusError(IOError):
  def __init__(self, *args):
    if len(args) == 1 and isinstance(args[0], int):
      # Append the error message to the error code.
      args = (args[0], FX_STATUS_MESSAGES.get(args[0], 'Unknown status'))
    IOError.__init__(self, *args)


def SendMsg(f, msg):
  assert isinstance(msg, str)
  # We have to enforce this, because sftp-server.c in OpenSSH 5.1 has this
  # limit, and drops the connection if it gets messages larger than this one.
  assert len(msg) <= SFTP_MAX_MSG_LENGTH
  buf = struct.pack('>L', len(msg)) + msg
  got = f.write(buf)
  assert got is None  # The file(...) object returns None on write().
  f.flush()


def RecvRes(f):
  size_str = f.read(4)
  assert len(size_str) == 4, 'EOF at message length, got=%d' % len(size_str)
  size = int(struct.unpack('>L', size_str)[0])
  assert size <= SFTP_MAX_MSG_LENGTH, 'message too large, size=%d' % size
  res = f.read(size)
  assert len(res) == size, 'EOF in incoming message, got=%d expected=%d' % (
      len(res), size)
  return res


def RaiseErrorIfStatus(res, msg_id):
  if len(res) >= 9 and res.startswith(chr(SSH2_FXP_STATUS)):
    _, res_id, res_status = struct.unpack('>BLL', res[:9])
    assert res_id == msg_id
    raise SftpStatusError(int(res_status))


def RaiseErrorUnlessStatusOk(res, msg_id):
  if len(res) >= 9 and res.startswith(chr(SSH2_FXP_STATUS)):
    _, res_id, res_status = struct.unpack('>BLL', res[:9])
    assert res_id == msg_id
    if res_status != SSH2_FX_OK:
      raise SftpStatusError(int(res_status))
  else:
    assert 0, 'expected SSH2_FXP_STATUS, got %d' % ord(res[0])


def ParseKeyValuePairs(res, i, pair_count):
  """A negative pair_count means no limitation."""
  h = {}
  while i < len(res) and pair_count != 0:
    pair_count -= 1
    i += 4
    assert i <= len(res)
    size = int(struct.unpack('>L', res[i - 4: i])[0])
    assert i + size <= len(res)
    key = res[i : i + size]
    i += size
    i += 4
    assert i <= len(res)
    size = int(struct.unpack('>L', res[i - 4: i])[0])
    assert i + size <= len(res)
    value = res[i : i + size]
    i += size
    h[key] = value
  return i, h


def ParseAttrs(res, i):
  """Parse an SFTP attrs (stat) structure in res[i:]."""
  attrs = {}
  i += 4
  assert i <= len(res)
  # Flags are a bitwise or of SSH2_FILEXFER_ATTR_* constants.
  attrs['flags'] = flags = int(struct.unpack('>L', res[i - 4 : i])[0])
  if flags & SSH2_FILEXFER_ATTR_SIZE:
    i += 8
    assert i <= len(res)
    attrs['size'] = int(struct.unpack('>Q', res[i - 8 : i])[0])
  if flags & SSH2_FILEXFER_ATTR_UIDGID:
    i += 8
    assert i <= len(res)
    attrs['uid'] = int(struct.unpack('>L', res[i - 8 : i - 4])[0])
    attrs['gid'] = int(struct.unpack('>L', res[i - 4 : i])[0])
  if flags & SSH2_FILEXFER_ATTR_PERMISSIONS:
    i += 4
    assert i <= len(res)
    attrs['perm'] = int(struct.unpack('>L', res[i - 4 : i])[0])
  if flags & SSH2_FILEXFER_ATTR_ACMODTIME:
    i += 8
    assert i <= len(res)
    attrs['atime'] = int(struct.unpack('>L', res[i - 8 : i - 4])[0])
    attrs['mtime'] = int(struct.unpack('>L', res[i - 4 : i])[0])
  if flags & SSH2_FILEXFER_ATTR_EXTENDED:
    # sftp-server(1) usually doesn't return this one.
    i += 4
    assert i <= len(res)
    count = int(struct.unpack('>L', res[i - 4 : i])[0])
    i, h = ParseKeyValuePairs(res, i)
    for key in h:
      attrs['_' + key] = h[key]
  return attrs


def SftpStat(rf, wf, version, msg_id, path):
  if version == 0:
    msg_type = SSH2_FXP_STAT_VERSION_0
  else:
    msg_type = SSH2_FXP_STAT
  SendMsg(wf, struct.pack('>BLL', msg_type, msg_id, len(path)) + path)
  res = RecvRes(rf)
  RaiseErrorIfStatus(res, msg_id)
  assert len(res) >= 5
  res_type, res_id = struct.unpack('>BL', res[:5])
  assert res_id == msg_id
  assert res_type == SSH2_FXP_ATTRS
  return ParseAttrs(res, 5)


def SftpUpload(rf, wf, version, msg_ids,
              local_filename, remote_filename, open_flags,
              block_size, max_pending):
  assert isinstance(block_size, int) and 0 < block_size <= 0x7fffffff
  assert isinstance(max_pending, int) and max_pending > 0
  print >>sys.stderr, 'info: will upload %r to %r' % (
      local_filename, remote_filename)
  uf = open(local_filename, 'rb')
  fcntl_flags = None
  try:
    st = os.fstat(uf.fileno())

    # Send the open command.
    # 
    # TODO(pts): sftp-client.c adds SSH2_FILEXFER_ATTR_ACMODTIME -- is it
    #            useful? Will it be retained (doesn't seem so)?
    attr_flags = SSH2_FILEXFER_ATTR_PERMISSIONS | SSH2_FILEXFER_ATTR_ACMODTIME
    attr_perm = st.st_mode & 07777  # sftp-client uses 0777.
    attr_atime = int(st.st_atime)
    attr_mtime = int(st.st_mtime)
    msg_id = msg_ids[0]
    msg_ids[0] += 1
    SendMsg(
        wf,
        struct.pack('>BLL', SSH2_FXP_OPEN, msg_id, len(remote_filename)) +
        remote_filename +
        struct.pack('>LLLLL', SSH2_FXF_WRITE | open_flags,
                    attr_flags, attr_perm, attr_atime, attr_mtime))

    res = RecvRes(rf)
    RaiseErrorIfStatus(res, msg_ids[0] - 1)
    assert len(res) >= 5
    res_type, res_id = struct.unpack('>BL', res[:5])
    assert res_id == msg_id
    assert res_type == SSH2_FXP_HANDLE
    assert len(res) >= 9
    handle_size = int(struct.unpack('>L', res[5 : 9])[0])
    # The filehandle is a string.
    handle = res[9 : 9 + handle_size]
    # Example handle: '\x00\x00\x00\x00'.
    print >>sys.stderr, 'info: got handle %r for uploading' % handle

    # Do the file transfer.
    #
    # Change rf to nonblocking mode so we can do partial reads (of ACKs) on
    # it.
    rfd = rf.fileno()
    fcntl_flags = fcntl.fcntl(rfd, fcntl.F_GETFL, 0)
    new_fcntl_flags = fcntl_flags | os.O_NONBLOCK
    fcntl.fcntl(rfd, fcntl.F_SETFL, new_fcntl_flags)
    # pending_acks contains the message IDs (msg_id) already sent to the
    # server, but not yet acknowledged by the server.
    pending_acks = set()
    ofs = 0
    res = ''
    res_size = None
    had_partial_read = False
    # If this 21 is changed to 20, sftp-server.c in OpenSSH 5.1 would start
    # dropping the connection because the SSH2_FXP_WRITE messages would
    # exceed its hardcoded SFTP_MAX_MSG_LENGTH.
    max_block_size = SFTP_MAX_MSG_LENGTH - 21 - len(handle)
    if block_size > max_block_size:
      block_size = max_block_size
    # Make the block size divisible by 16384 to get good memory page
    # alignment and thus high performance disk reads. (On Linux i386, the
    # divisor 4096 would have been enough.)
    if block_size > 16383:
      block_size &= ~16383;
    while True:
      # TODO(pts): Read at most 128kB from the file at once, and fill the
      # send buffer with parts of a message.
      data = uf.read(block_size)
      if 0 < len(data) < block_size:
        # Allow a partial read, just before EOF.
        # TODO(pts): What if somebody is appending to the file while we're
        # reading it?
        assert not had_partial_read, 'partial read, got %d' % len(data)
        had_partial_read = True

      # Read ACK responses from the SFTP server.
      if data:
        num_acks_to_read = len(pending_acks) - max_pending + 1
        do_ack_while_available = num_acks_to_read <= 0
      else:
        num_acks_to_read = len(pending_acks)
        do_ack_while_available = False
      print >>sys.stderr, 'info: upload len(data)=%d nacks=%d doack=%r' % (
          len(data), num_acks_to_read, do_ack_while_available)
      while num_acks_to_read > 0 or do_ack_while_available:
        if res_size is None:
          read_size = 4 - len(res)
        else:
          read_size = res_size - len(res)
        try:
          res_new = rf.read(read_size)
        except IOError, e:
          if e[0] != errno.EAGAIN:
            raise
          if do_ack_while_available:
            break
          select.select((rfd,), (), ())  # Wait for input to be available.
          continue
        assert res_new, 'EOF while reading from ACKs'
        res += res_new
        if res_size is None:
          if len(res) == 4:
            res_size = int(struct.unpack('>L', res)[0])
            # We are expecting an SSH2_FXP_STATUS response, which should not
            # be longer than 9 bytes. But we're generous and allow a bit more.
            assert res_size <= 128, (
                'expected write response too large, size=%d' % res_size)
            res = ''
          continue
        if len(res) < res_size:
          continue
        # Now we managed to read a whole message to res. Let's process it.
        assert len(res) >= 5
        res_type, res_id = struct.unpack('>BL', res[:5])
        # TODO(pts): Check res_type.
        res_id = int(res_id)
        assert res_id in pending_acks
        pending_acks.remove(res_id)
        num_acks_to_read -= 1
        RaiseErrorUnlessStatusOk(res, res_id)
        res = ''
        res_size = None
        
      if not data:  # End of local file.
        break
      msg_id = msg_ids[0]
      pending_acks.add(msg_id)
      msg_ids[0] += 1
      # TODO(pts): Use a buffer (in uf.read) to avoid a long data copy here.
      SendMsg(wf, ''.join(
          [struct.pack('>BLL', SSH2_FXP_WRITE, msg_id, len(handle)),
           handle, struct.pack('>QL', ofs, len(data)), data]))
      ofs += len(data)

    print >>sys.stderr, 'info: data uploaded, ACKs received, closing'

    # Restore the blocking mode for rfd.
    if fcntl_flags is not None and fcntl_flags != new_fcntl_flags:
      fcntl.fcntl(rfd, fcntl.F_SETFL, fcntl_flags)
      new_fcntl_flags = fcntl_flags

    # Send the close command.
    msg_id = msg_ids[0]
    msg_ids[0] += 1
    SendMsg(wf, struct.pack('>BLL', SSH2_FXP_CLOSE, msg_id, len(handle)) +
            handle)
    res = RecvRes(rf)
    RaiseErrorUnlessStatusOk(res, msg_id)
  finally:
    if fcntl_flags is not None and fcntl_flags != new_fcntl_flags:
      fcntl.fcntl(rfd, fcntl.F_SETFL, fcntl_flags)
    uf.close()


def main(argv):
  ssh_args = argv[1:] or ['127.0.0.1']
  # TODO(pts): Check that ssh_args doesn't already contain a command.
  ssh_args.append('sftp')

  # TODO(pts): Also open STDIN and STDOUT.
  p = subprocess.Popen(['ssh', '-s'] + ssh_args,
                       stdin=subprocess.PIPE, stdout=subprocess.PIPE)
  rf = p.stdout
  wf = p.stdin

  try:
    # Do the init handshake.
    SendMsg(wf, struct.pack('>BL', SSH2_FXP_INIT, SSH2_FILEXFER_VERSION))
    res = RecvRes(rf)

    # Parse the init response.
    assert res.startswith(chr(SSH2_FXP_VERSION))
    # Example res: '\x02\x00\x00\x00\x03\x00\x00\x00\x18'
    #     'posix-rename@openssh.com\x00\x00\x00\x011\x00\x00\x00\x13'
    #     'statvfs@openssh.com\x00\x00\x00\x012\x00\x00\x00\x14'
    #     'fstatvfs@openssh.com\x00\x00\x00\x012'
    assert len(res) >= 5
    version = int(struct.unpack('>L', res[1 : 5])[0])
    # Don't check this, we should be able to use any server version, just like
    # what sftp-client does.
    #assert version in (0, 1, 2, 3), (
    #    'version mismatch: client=%d server=%d' %
    #    (SSH2_FILEXFER_VERSION, version))
    i = 5
    i, server_init_flags = ParseKeyValuePairs(res, i, -1)

    # Example: {'fstatvfs@openssh.com': '2', 'statvfs@openssh.com': '2',
    #           'posix-rename@openssh.com': '1'}
    print >>sys.stderr, 'info: server_init_flags = %r' % server_init_flags
    msg_id = 0

    # Send a simple stat request and parse the response.
    msg_id += 1
    attrs = SftpStat(rf, wf, version, msg_id, '/dev/null')
    print >>sys.stderr, 'info: stat("/dev/null") = %r' % attrs

    # Upload (put) a file.
    if os.path.isfile('test.upload'):
      msg_ids = [msg_id + 1]
      # block_size should be less than 1024 * 256 (SFTP_MAX_MSG_LENGTH, as
      # defined on the server, in sftp-server.c of OpenSSH 5.1).
      #
      # If open_flags=SSH2_FXF_EXCL|SSH2_FXF_CREAT is specified, and the remote
      # file already exists, then SftpStatusError(SSH2_FX_FAILURE) would be
      # raised, without a specific indication of the real error message or
      # errno.
      SftpUpload(rf, wf, version, msg_ids, 'test.upload', '/tmp/remote.upload',
                 open_flags=SSH2_FXF_CREAT,  # | SSH2_FXF_TRUNC,
                 block_size=1 << 22, max_pending=64)
      msg_id = msg_ids[0] - 1
  finally:
    # Close the SSH client connection.
    wf.close()
    rf.close()
    print >>sys.stderr, 'info: waiting for the SSH process to exit'
    status = p.wait()
  assert status == 0, 'SSH process exited with status=0x%x' % status
  print >>sys.stderr, 'info: sftp done'


if __name__ == '__main__':
  sys.exit(main(sys.argv))
