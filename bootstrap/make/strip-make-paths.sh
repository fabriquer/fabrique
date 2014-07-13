#!/bin/sh

cp Makefile Makefile.bak || exit 1

BUILDROOT="`pwd`"
SRCROOT="`pwd | xargs dirname | xargs dirname`"

sed -i.sedbackup "s#${BUILDROOT}#\${buildroot}#g" Makefile || exit 1
sed -i.sedbackup "s#${SRCROOT}#\${srcroot}#g" Makefile || exit 1
sed -i.sedbackup "s#^buildroot=	\${buildroot}#buildroot=	.#" Makefile || exit 1
sed -i.sedbackup "s#^srcroot=	\${srcroot}#srcroot=	../..#" Makefile || exit 1

wdiff Makefile.bak Makefile | grep '{+' | colordiff
rm Makefile.bak Makefile.sedbackup
