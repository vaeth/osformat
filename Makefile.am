# This file is part of the osformat project and distributed under the
# terms of the GNU General Public License v2.
#
#   Copyright (c)
#     Martin V\"ath <martin@mvath.de>

AUTOMAKE_OPTIONS = foreign 1.11.2 no-dist-gzip dist-xz subdir-objects -Wobsolete -Wportability -Wextra-portability

ACLOCAL_AMFLAGS = -I m4 -I martinm4

lib_LTLIBRARIES = libosformat.la

libosformat_la_LDFLAGS = -version-info 0:0:0

libosformat_la_SOURCES = \
osformat/osformat.cc \
osformat/osformat.h

pkginclude_HEADERS = \
osformat/osformat.h

TESTS = osformat-test

check_PROGRAMS = osformat-test

osformat_test_SOURCES = \
osformat/osformat-test.cc

osformat_test_LDADD = \
libosformat.la

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = osformat.pc

# Stuff from our distribution
EXTRA_DIST = \
.gitignore \
CPPLINT.cfg \
autogen.sh \
contrib/cpplint.sh \
contrib/make.sh \
contrib/tarball.sh

AUTOCLEANFILES = \
Makefile.in \
aclocal.m4 \
ar-lib \
config.guess \
config.h.in \
config.h.in~ \
config.log \
config.status \
config.sub \
configure \
compile \
depcomp \
install-sh \
ltmain.sh \
missing \
test-driver \
osformat*.asc \
osformat*.tar.bz2 \
osformat*.tar.gz \
osformat*.tar.xz \
osformat*.zip

AUTOCLEANDIRS = \
autom4te.cache \
build \
config \
m4 \
tmpwrk[0-9][0-9]*

# Remove autotool created stuff
.PHONY: autoclean
autoclean: maintainer-clean
	$(AM_V_at)cd "$(top_src_dir)"
	$(AM_V_at)$(RM) -vfr $(AUTOCLEANDIRS)
	$(AM_V_at)$(RM) -vf $(AUTOCLEANFILES)
