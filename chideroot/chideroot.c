/*
 * chideroot: chroot with better filesystem isolation for Linux
 * by pts@fazekas.hu at Wed Feb 22 17:09:53 CET 2012
 *
 * * chideroot hides directories and filesystems in both both ways:
 *   processes running inside the chroot cannot see anything outside of it,
 *   and processes running outside cannot see anything inside.
 *
 * * chideroot requires that the startup directory is a mount point (i.e. a
 *   filesystem is mounted right there).
 *
 * * You have to run chideroot as root.
 *
 * * chideroot is Linux-specific, because it uses pivot_root(...) and
 *   unshare(CLONE_NEWNS). Kernels somce 2.4.19 should be fine. Tested with
 *   2.6.32, 2.6.35 and 2.6.38.
 *
 * * Upon startup, chroot umounts the startup directory outside.
 *
 * * As a convenience, chroot mounts /proc inside.
 *
 * * chideroot cannot be used on a read-only startup filesystem.
 *
 * * As a convenience, chideroot closes all file descriptors (except or 0, 1
 *   and 2).
 *
 * * As a convenience, chideroot removes all environment variables (except for
 *   TERM).
 *
 * * chideroot doesn't provice process isolation (or any other isolation
 *   than filesystem isolation), so processes inside and outside see each
 *   other (via /proc or in ps(1)) and can kill each other.
 *
 * TODO: Add one-way process isolation with CLONE_NEWPID.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define _GNU_SOURCE 1  /* O_DIRECTORY */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

char buf[5200];
char env_buf[256];
       
int main(int argc, char **argv) {
  /* TODO: Add functionality to rmdir new_root, possibly create a subdir to
   * hide it from other users.
   */
  const char *new_root;
  const char *env[] = { NULL, NULL };
  int got, i, fd, pid, pid2, pfd[2], status;
  int okc, failc;
  DIR *dir;
  FILE *f;
  char *p, *q;
  struct dirent *dirent;

  /* Parse command-line */
  (void)argc;
  if (argv[1] == NULL) {
    fprintf(stderr, "Usage: %s <new-root> [<command> [<arg>]]\n", argv[0]);
    return 1;
  }
  if (strlen(argv[1]) > sizeof(buf) - 10) {
    fprintf(stderr, "error: name of new_root too long\n");
    return 2;
  }
  new_root = argv[1];

  p = getenv("TERM");
  if (p != NULL) {
    /* TODO: Find it in environ manually, so env_buf is not needed. */
    strcpy(env_buf, "TERM=");
    if (strlen(p) >= sizeof(env_buf) - 5) {
      fprintf(stderr, "error: TERM too long\n");
      return 2;
    }
    strncpy(env_buf + 5, p, sizeof(env_buf) - 5);
    env_buf[sizeof(env_buf) - 1] = '\0';  /* Just for extra safety. */
    env[0] = env_buf;
  }

  fd = open(new_root, O_RDONLY | O_DIRECTORY);  /* Can open a directory. */
  if (fd < 0) {
    perror("open new_root");
    return 2;
  }
  got = umount(new_root);
  if (got == 0) {
    /* Can't succeed, we have fd open. */
    fprintf(stderr, "error: unexpected umount success\n");
    return 2;
  }
  
  if (errno == EINVAL) {
    /* TODO: Detect that new_root is a mount point. */
    fprintf(stderr, "error: new_root is not mounted\n");
    return 2;
  }
  if (errno != EBUSY) {
    perror("umount1");
    return 2;
  }
  close(fd);
  got = pipe(pfd);
  if (got != 0) {
    perror("pipe");
    return 2;
  }
  pid = fork();
  if (pid < 0) {
    perror("fork");
    return 2;
  }
  if (pid == 0) {  /* Child: will umount new_root outside. */
    char c;
    close(pfd[1]);
    got = read(pfd[0], &c, 1);  /* Wait for parent to activate us. */
    if (got != 0) {
      if (got == 0)
        return 0;
      perror("read pfd");
      return 2;
    }
    got = umount(new_root);
    if (got != 0) {
      perror("umount child");
      return 2;
    }
    return 0;
  }
  close(pfd[0]);
  /* umount is needed otherwise the child won't be able to umount new_root.
   * TODO: Umount everything in new_root.
   */
  /* buf is long enough, we've checked that */
  strcpy(buf, new_root);  /* TODO: Use snprintf for extra safety. */
  strcat(buf, "/proc");
  umount(buf);  /* Don't check the result. */

  got = syscall(SYS_unshare, CLONE_NEWNS);
  if (got != 0) {
    perror("unshare");
    return 2;
  }
  got = chdir(new_root);
  if (got != 0) {
    perror("chdir1");
    return 2;
  }
  mkdir("put_old", 0700);  /* Don't check the result. */
    
  got = syscall(SYS_pivot_root, ".", "put_old");
  if (got != 0) {
    /* TODO: Report nice error if put_old is not a directory. */
    /* See restrictions is `man 2 pivot_root' for possible error reasons. */
    perror("pivot_root");
    return 2;
  }
  got = chroot(".");
  if (got != 0) {
    perror("chroot");
    return 2;
  }
  got = chdir("/");
  if (got != 0) {
    perror("chdir2");
    return 2;
  }
  close(pfd[1]);  /* Activate the child, who will umount(new_root). */
  /* Wait for the child to finish the umount. */
  pid2 = waitpid(pid, &status, 0);
  if (pid2 < 0) {
    perror("waitpid");
    return 2;
  }
  if (pid != pid2) {
    fprintf(stderr, "error: unexpected child exit\n");
    return 2;
  }
  if (status != 0) {
    fprintf(stderr, "error: child failed with status=0x%x\n", status);
    return 2;
  }

  umount("/proc");  /* Don't check the result. */
  mkdir("/proc", 0755);  /* Don't check the result. */
  got = mount("none", "/proc", "proc", 0, NULL);
  if (got != 0) {
    perror("mount /proc");
    return 2;
  }

  /* Close all file descriptors other than 0, 1 and 2 (/proc/self/fd). */
  dir = opendir("/proc/self/fd");
  if (dir == NULL) {
    perror("opendir /proc/self/fd");
    return 2;
  }
  fd = dirfd(dir);
  while (NULL != (dirent = readdir(dir))) {
    if (1 == sscanf(dirent->d_name, "%d", &i) && i != fd && i > 2) {
      close(i);  /* Don't check the result. */
    }
  }
  closedir(dir);

  /* Umount all filesystems in put_old. */
  do {
    okc = 0; failc = 0;
    f = fopen("/proc/mounts", "r");
    if (f == NULL) {
      perror("fopen /proc/mounts");
      return 2;
    }
    while (NULL != (p = fgets(buf, sizeof(buf) - 1, f))) {
      if (buf[0] == '\0' || buf[strlen(buf) - 1] != '\n') {
        fprintf(stderr, "error: mounts line empty or too long\n");
        return 2;
      }
      p = buf;
      while (p[0] != '\0' && p[0] != ' ' && p[0] != '\n') {
        ++p;
      }
      while (p[0] == ' ') {
        ++p;
      }
      if (p[0] == '\0' || p[0] == '\n') {
        fprintf(stderr, "error: mounts mount point not found: %s", buf);
        return 2;
      }
      q = p;
      while (q[0] != '\0' && q[0] != ' ') {
        ++q;
      }
      if (q[0] == '\0' || q[0] == '\n') {
        /* No \n. */
        fprintf(stderr, "error: mounts mount point ends: %s", buf);
        return 2;
      }
      q[0] = '\0';
      if (0 == strncmp(p, "/put_old", 8) && (p[8] == '/' || p[8] == '\0')) {
        if (umount(p) != 0) {
          /* It's OK to fail here, because some filesystems are inside
           * each other, and we may try to umount the outer filesystem first.
           */
          /* TODO: Sort by the number of slashes. */
          ++failc;
        } else {
          ++okc;
        }
      }
    }
    if (ferror(f)) {
      fprintf(stderr, "error: reading mounts\n");
      return 2;
    }
    fclose(f);
  } while (okc > 0 && failc > 0);  /* Try to make more progress umounting. */
  if (failc > 0) {
    fprintf(stderr, "error: could not umount put_old\n");
    return 2;
  }

  got = rmdir("/put_old");
  if (got != 0) {
    perror("rmdir put_old");
    return 2;
  }

  if (argv[2] == NULL) {
    got = execle("/bin/sh", "sh", "-i", NULL, env);
  } else {
    got = execve(argv[2], argv + 2, (void*)env);
  }
  if (got != 0) {
    perror("execl");
    return 2;
  }
  return 0;
}
