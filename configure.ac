AC_PREREQ(2.61)
AC_INIT([mimesis], [0.1])
AM_INIT_AUTOMAKE([std-options subdir-objects nostdinc silent-rules -Wall])
AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])

AM_PROG_AR
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_INSTALLDIR

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11

AC_CONFIG_FILES([Makefile src/Makefile src/mimesis.pc test/Makefile])
AC_OUTPUT
