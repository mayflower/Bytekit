dnl $Id$
dnl config.m4 for extension bytekit

PHP_ARG_ENABLE(bytekit, whether to enable ByteKit support,
[  --enable-bytekit            Enable ByteKit support])

if test "$PHP_BYTEKIT" != "no"; then
  AC_DEFINE(HAVE_BYTEKIT, 1, [ ByteKit ])
  PHP_NEW_EXTENSION(bytekit, bytekit.c, $ext_shared)
fi
