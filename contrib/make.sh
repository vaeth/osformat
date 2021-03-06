#!/usr/bin/env sh
# SPDX-License-Identifier: GPL-2.0-only
set -u

LC_ALL=C
export LC_ALL

Echo() {
	printf '%s\n' "$*"
}

Usage() {
	Echo "Usage: ${0##*/} [options] args-for-make
Available options are
  -q  quiet
  -n  Stop after ./configure, i.e. do not run make
  -l  Install into ./local
  -e  Keep environment - do not modify LDFLAGS, CXXFLAGS, CFLAGS, CC
  -w  Enable warnings
  -g  Use clang++, setting CXX, filtering some flags (default if available)
  -G  Use default CXX (mnemonic: GNU)
  -C  Avoid CCACHE
  -x  Recache (i.e. ignore broken ccache)
  -X  Clear CCACHE
  -c OPT Add OPT to ./configure
  -r  Change also directory permissions to root (for fakeroot-ng)"
	exit ${1:-1}
}

Info() {
	$quiet || printf '\033[01;32m%s\033[0m\n' "$*"
}

InfoVerbose() {
	$quiet || printf '\n\033[01;33m%s\033[0m\n\n' "$*"
}

Die() {
	Echo "${0##*/}: error: $1" >&2
	exit ${2:-1}
}

SetCcache() {
	dircc='/usr/lib/ccache/bin'
	if $use_ccache && case ":$PATH:" in
	*":$dircc:"*)
		false;;
	esac
	then	Info "export PATH=$dircc:\$PATH"
		PATH=$dircc:$PATH
		export PATH
	fi
	if $recache
	then	Info "export CCACHE_RECACHE=true"
		CCACHE_RECACHE=true
		export CCACHE_RECACHE
	fi
	CCACHE_SLOPPINESS='file_macro,time_macros,include_file_mtime'
	Info "export CCACHE_SLOPPINESS='$CCACHE_SLOPPINESS'"
	export CCACHE_SLOPPINESS
	testcc=
	for dircc in \
		"$HOME/.ccache" \
		../.ccache \
		../ccache \
		../../.ccache \
		../../ccache
	do	[ -z "${dircc:++}" ] || ! test -d "$dircc" && continue
		testcc=`cd -P -- "$dircc" >/dev/null 2>&1 && \
				printf '%sA' "$PWD"` && \
			testcc=${testcc%A} && [ -n "${testcc:++}" ] \
			&& test -d "$testcc" && break
	done
	if [ -n "${testcc:++}" ]
	then	Info "export CCACHE_DIR=$testcc"
		Info 'export CCACHE_COMPRESS=true'
		CCACHE_DIR=$testcc
		CCACHE_COMPRESS=true
		export CCACHE_DIR CCACHE_COMPRESS
	fi
	$clear_ccache || return 0
	Info 'ccache -C'
	ccache -C
}

clang_cxx=`PATH=${PATH-}${PATH:+:}/usr/lib/llvm/*/bin command -v clang++ 2>/dev/null` \
  && [ -n "${clang_cxx:++}" ] && clang=: || clang=false

prefix=/usr
quiet=false
earlystop=false
keepenv=false
warnings=false
use_chown=false
use_ccache=:
automake_extra=
optimization=false
recache=false
clear_ccache=false
OPTIND=1
while getopts 'lqgGnewCxXyYc:rhH' opt
do	case $opt in
	l)	prefix=$PWD/local;;
	q)	quiet=:;;
	g)	clang=:;;
	G)	clang=false;;
	n)	earlystop=:;;
	e)	keepenv=:;;
	w)	warnings=:;;
	C)	use_ccache=false;;
	x)	recache=:;;
	X)	clear_ccache=:;;
	c)	configure_extra=$configure_extra" $OPTARG";;
	r)	use_chown=:;;
	'?')	exit 1;;
	*)	Usage 0;;
	esac
done
if [ $OPTIND -gt 1 ]
then	( eval '[ "$(( 0 + 1 ))" = 1 ]' ) >/dev/null 2>&1 && \
	eval 'shift "$(( $OPTIND - 1 ))"' || shift "`expr $OPTIND - 1`"
fi

configure_extra=--prefix=$prefix

SetCcache
! $warnings || configure_extra=$configure_extra' --enable-warnings'
$quiet && quietredirect='>/dev/null' || quietredirect=

if $use_chown
then	ls /root >/dev/null 2>&1 && \
		Die 'you should not really be root when you use -r' 2
	chown -R root:root .
fi

Filter() {
	for currvar in \
		CFLAGS \
		CXXFLAGS \
		LDFLAGS \
		CPPFLAGS
	do	eval oldflags=\${${currvar}-}
		newflags=
		for currflag in $oldflags
		do	case " $* " in
			*" $currflag "*)	continue;;
			esac
			newflags=$newflags${newflags:+ }$currflag
		done
		eval $currvar=\$newflags
	done
}

FilterClang() {
	Filter \
		-fnothrow-opt \
		-frename-registers \
		-funsafe-loop-optimizations \
		-fgcse-sm \
		-fgcse-las \
		-fgcse-after-reload \
		-fpredictive-commoning \
		-ftree-switch-conversion \
		-fno-ident \
		-flto-partition=none \
		-ftree-vectorize \
		-fweb \
		-fgraphite \
		-fgraphite-identity \
		-floop-interchange \
		-floop-strip-mine \
		-floop-block \
		-fno-enforce-eh-specs \
		-fdirectives-only \
		-ftracer
}

if ! $keepenv
then	unset CFLAGS CXXFLAGS LDFLAGS CPPFLAGS CXX
	if command -v portageq >/dev/null 2>&1
	then	CFLAGS=`portageq envvar CFLAGS`
		CXXFLAGS=`portageq envvar CXXFLAGS`
		LDFLAGS=`portageq envvar LDFLAGS`
		CPPFLAGS=`portageq envvar CPPFLAGS`
		CXX=`portageq envvar CXX`
	fi
	if $clang
	then	CXX=$clang_cxx
		FilterClang
	fi
	[ -z "${CXX:++}" ] || export CXX
fi
[ -z "${CXXFLAGS:++}" ] || Info "export CXXFLAGS='${CXXFLAGS}'"
[ -z "${LDFLAGS:++}" ] || Info "export LDFLAGS='${LDFLAGS}'"
[ -z "${CPPFLAGS:++}" ] || Info "export CPPFLAGS='${CPPFLAGS}'"
[ -z "${CXX:++}" ] || Info "export CXX='${CXX}'"
if ! test -e Makefile
then	if ! test -e configure || ! test -e Makefile.in
	then	InfoVerbose './autogen.sh' $automake_extra
		eval "./autogen.sh $automake_extra $quietredirect" \
			|| Die 'autogen failed'
	fi
	InfoVerbose './configure' $configure_extra
	eval "./configure $configure_extra $quietredirect" || \
		Die 'configure failed'
fi
$earlystop && exit 0
InfoVerbose 'make' $*
command -v make >/dev/null 2>&1 || Die 'cannot find make'
if $quiet
then	exec make ${1+"$@"} >/dev/null
else	exec make ${1+"$@"}
fi
