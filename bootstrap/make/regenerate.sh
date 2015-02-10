#!/bin/sh

fab --format=gmake -D 'debug=false' -D 'asserts=false' ../../fabfile || exit 1
awk '/^GNUmakefile : / {exit} {print}' GNUmakefile > tmp || exit 1
sed -i '' 's/GNUmakefile//' tmp || exit 1
./strip-make-paths.sh tmp || exit 1
mv tmp GNUmakefile
