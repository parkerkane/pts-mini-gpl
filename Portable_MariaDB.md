Portable MariaDB is a small, portable binary distribution of the SQL server MariaDB (Monty's fork of MySQL) for Linux i386 (32-bit). Only the mysqld binary and a versatile init script are included. Portable MariaDB can be run by any user in any directory, it doesn't try to access any mysqld data or config files outside its directory. Portable MariaDB can coexist with regular mysqld (MySQL or MariaDB) and other instances of Portable MariaDB on a single machine, as long as they are not configured to listen on the same TCP port. The only dependency of Portable MariaDB is glibc 2.4 (available in Ubuntu Hardy or later).

Why use Portable MariaDB?

  * It's small (not bloated). Fast to dowload, fast to extract, fast to install. Quick size comparison: mariadb-5.2.9-Linux-i686.tar.gz is 144 MB, the corresponding Portable MariaDB .tbz2 is less than 6 MB.
  * It's portable: does not interfere with other MySQL server installations on the same machine.
  * It's self-contained and consistent: copy the database and the configuration in a single directory from one machine to another.

## Installation ##

To run Portable MariaDB, you need a Linux system with glibc 2.4 (e.g. Ubuntu Hardy) or later. 32-bit and 64-bit systems are fine. For 64-bit systems you need the 32-bit compatibility libraries installed. You also need Perl.

<pre>   $ cd /tmp  # Or any other with write access.<br>
$ BASE=http://pts-mini-gpl.googlecode.com/svn/trunk/portable-mariadb.release/<br>
<br>
$ wget -O portable-mariadb.tbz2 $BASE/portable-mariadb.tbz2<br>
$ tar xjvf portable-mariadb.tbz2<br>
$ chmod 700 /tmp/portable-mariadb  # For security.<br>
$ /tmp/portable-mariadb/mariadb_init.pl stop-set-root-password</pre>

## Usage ##

For security, don't do anything as root.

<pre>   $ cd /tmp/portable-mariadb<br>
$ ./mariadb_init.pl restart<br>
Connect with: mysql --socket=/tmp/portable-mariadb/mysqld.sock --user=root --database=test --password<br>
Connect with: mysql --host=127.0.0.1 --user=root --database=test --password</pre>

Feel free to take a look at `/tmp/portable-mariadb/my.cnf` , make modifications, and restart mysqld so that the modifications take effect.

## Security ##

By default, connections are accepted from localhost (Unix domain socket and TCP) only, all MySQL users are refused (except if a password has been set for root above), and root has unrestricted access. Unix permissions (such as the `chmod 700` above) are protecting against data theft and manipulation on
the file level.

It is strongly recommended to change the password of root to a non-empty, strong password before populating the database.

## Java support ##

Java clients with JDBC (MySQL Connector/J) are fully supported. Please note that Java doesn't support Unix doman socket, so make sure in my.cnf that mysqld listens on a TCP port. Please make sure you have `?characterEncoding=UTF8` specified in your JDBC connection URL, otherwise some non-ASCII, non-Latin-1 characters would be converted to `?`.

## Unicode support ##

Just as with MariaDB. All encodings and collations are supported. The latin1 encoding is the default, which can be changed in my.cnf.

## Language support ##

All natural languages (of MariaDB) are supported for error messages. Set the `language' flag in my.cnf accordingly. English is the default.