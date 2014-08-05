#!/bin/sh

fab --format=make -D 'debug=false' -D 'asserts=false' ../../fabfile || exit 1
./strip-make-paths.sh || exit 1
