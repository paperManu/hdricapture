#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT(HDRICAPTURE, 0.0.9, emmanueldurand@gmail.com)
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE()

# Check for C and C++ compilers
AC_PROG_CXX
AC_PROG_CC

# Check for header files
AC_HEADER_STDC

# OpenCV
PKG_CHECK_MODULES([OPENCV], [opencv >= 2.3])
if test "x${have_opencv}" = "xfalse" ; then
    AC_MSG_ERROR([Missing opencv])
fi

# LCMS
PKG_CHECK_MODULES([LCMS], [lcms.pc >= 2.2])
if test "x${have_lcms}" = "xfalse" ; then
    AC_MSG_ERROR([Missing lcms])
fi

BOOST_REQUIRE([1.48])
BOOST_SYSTEM
BOOST_FILESYSTEM
BOOST_THREADS
BOOST_CHRONO

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    ])
AC_OUTPUT
