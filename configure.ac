AC_INIT([arm9launcher], [0.0.2], [], [])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects])

#Find AR and RANLIB for lto support before initializing libtool
AC_CHECK_TOOL([AR],gcc-ar)
AC_CHECK_TOOL([RANLIB],gcc-ranlib)
LT_INIT

AC_PROG_CC
AM_PROG_AS
AC_CHECK_TOOL([OBJCOPY],objcopy)

AC_CONFIG_FILES([Makefile src/Makefile ext/Makefile])

AC_OUTPUT

