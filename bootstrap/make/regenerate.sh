#!/bin/sh

fab --format=make -D 'debug=false' ../../fabfile || exit 1
./strip-make-paths.sh || exit 1
