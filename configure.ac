# This file is part of the osformat project and distributed under the
# terms of the GNU General Public License v2.
#
# Copyright (c)
#  Martin V\"ath <martin@mvath.de>

dnl keep version in same line as AC_INIT for possible usage in scripts
AC_INIT([osformat], [1.0.5],
  [https://github.com/vaeth/osformat/issues/],
  [osformat],
  [https://github.com/vaeth/osformat/])
AC_PREREQ([2.64])

m4_ifdef([AC_CONFIG_MACRO_DIR],
	[AC_CONFIG_MACRO_DIR([m4])])
m4_ifdef([AC_CONFIG_MACRO_DIRS],
	[AC_CONFIG_MACRO_DIRS([m4 martinm4])])
AC_CONFIG_FILES([
		Makefile
	])

AM_MAINTAINER_MODE()
AM_INIT_AUTOMAKE()
AM_SILENT_RULES([yes])

AC_LANG([C++])
AC_PROG_CXX()

AC_PROG_MKDIR_P()
AM_PROG_AR()
LT_INIT([disable-static])

AC_ARG_ENABLE([warnings],
	[AS_HELP_STRING([--enable-warnings],
		[append warning/testing flags; might produce worse code])],
	[MV_ENABLE([warnings])
	AS_VAR_SET([cmt_warnings], ["on request"])],
	[AS_VAR_SET([warnings], [false])
	AS_VAR_SET([cmt_warnings], ["default"])])
AC_MSG_CHECKING([whether warning options should be used])
MV_MSG_RESULT_BIN([$warnings], [$cmt_warnings])

AS_IF([$warnings],
	[AS_VAR_COPY([oricxxflags], [CXXFLAGS])
	AS_VAR_COPY([orildflags], [LDFLAGS])
	AS_VAR_SET([CFLAGS], [])
	AS_VAR_SET([CXXFLAGS], [])
	AS_VAR_SET([LDFLAGS], [])

	AS_VAR_SET([my_cxxfatal], [])
	AS_VAR_SET([my_ldfatal], [])
	MV_ADDFLAGS([my_cxxfatal], [CXXFLAGS], [AC_COMPILE_IFELSE], [ \
			-Werror \
			-Werror=unknown-warning-option \
			-Wunknown-warning-option \
		])
	MV_ADDFLAGS([my_ldfatal], [LDFLAGS], [AC_LINK_IFELSE], [ \
			$my_cxxfatal \
			-Wl,--fatal-warnings \
		])
	AS_VAR_SET([my_cxxadd], [])
	AS_VAR_SET([my_ldadd], [])
	MV_ADDFLAGS([my_cxxadd], [CXXFLAGS], [AC_COMPILE_IFELSE], [ \
			-Wpedantic \
			-pedantic \
			-Wall \
			-Weverything \
			-Wextra \
			-Wformat=2 \
			-Wunused-variable \
			-Wunused-parameter \
			-Wstring-plus-int \
			-WNSObject-attribute \
			-Winit-self \
			-Wmissing-include-dirs \
			-Wswitch-default \
			-Wunused \
			-Wunused-parameter \
			-Wstrict-aliasing=1 \
			-Wstrict-overflow=5 \
			-Wfloat-equal \
			-Wundef \
			-Wshadow \
			-Wpointer-arith \
			-Wcast-qual \
			-Wcast-align \
			-Wwrite-strings \
			-Wsign-compare \
			-Wmissing-field-initializers \
			-Wnormalized=nfkc \
			-Wpacked \
			-Wredundant-decls \
			-Winvalid-pch \
			-Wvolatile-register-var \
			-Wdisabled-optimization \
			-Wlogical-op \
			-Wvla \
			-Wabi \
			-Wstrict-null-sentinel \
			-Wstring-plus-int \
			-Woverloaded-virtual \
			-Wsign-promo \
			-Wnoexcept \
			-Wmissing-declarations \
			-Wmissing-format-attribute \
			-Wunused-but-set-variable \
			-Wunused-but-set-parameter \
			-Wdelete-non-virtual-dtor \
			-Wint-to-pointer-cast \
			-Wcomment \
			-WNSObject-attribute \
			-fdiagnostic-color=always \
			-Wswitch-bool \
			-Wlogical-not-parantheses \
			-Wsizeof-array-argument \
			-Wbool-compare \
			-Wodr-type-mismatch \
			-Wshift-negative-value \
			-Wshift-overflow \
			-Wshift-overflow=2 \
			-Wnull-dereference \
			-Wduplicated-cond \
			-Wsign-conversion \
			-Wold-style-cast \
			-Wmissing-noreturn \
			-Wsuggest-attribute=pure \
			-Wsuggest-attribute=const \
			-Wsuggest-attribute=noreturn \
			-Wunreachable-code \
			-Waggregate-return \
		], [$my_cxxfatal], [], [:])
	MV_ADDFLAGS([my_ldadd], [LDFLAGS], [AC_LINK_IFELSE], [ \
			-Wl,-z,defs \
			-Wl,--no-undefined \
			-Wl,--no-allow-shlib-undefined \
			-Wl,--no-undefined-version \
			$warn_common \
		], [$my_cxxfatal], [], [:])
	MV_PREPEND([CXXFLAGS], [$my_cxxadd])
	MV_PREPEND([LDFLAGS], [$my_ldadd])])
dnl This is used intentionally for backward compatibility:
dnl			-Wzero-as-null-pointer-constant \
dnl Some purely static classes are used intentionally:
dnl			-Wctor-dtor-privacy \
dnl Not needed anymore with current clang:
dnl			-Wno-unknown-warning-option \
dnl Output is too confusing:
dnl			-fmem-report \
dnl Too many useless warnings or only for C:
dnl			-Wtraditional \
dnl			-Wc++-compat \
dnl			-Wstrict-prototypes \
dnl			-Wold-style-definition \
dnl			-Wmissing-prototypes \
dnl			-Wnested-externs \
dnl			-Wpointer-sign \
dnl Included in -Wall (in gcc-6; not available earlier, anyway)
dnl			-Wtautological-compare \
dnl			-Wmisleading-indentation \
dnl
dnl We do not use (since it gives too many useless warnings or is only for C):
dnl			-Wconversion \
dnl			-Wswitch-enum \
dnl			-Wunsafe-loop-optimizations \
dnl			-Wpadded \
dnl			-Winline \
dnl			-Weffc++ \


AC_OUTPUT([
	osformat.pc
])

AS_ECHO(["

$PACKAGE-$VERSION configured successfully.

"])
