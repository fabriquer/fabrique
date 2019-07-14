#!/bin/sh

# Run Fabrique to generate a GNUmakefile here in this directory
fab --format=gmake ../../fabfile || exit 1

# Remove makefile regeneration: the build host may not have a `fab`
awk '/^GNUmakefile : / {exit} {print}' GNUmakefile > tmp || exit 1
sed -i '' 's/GNUmakefile//' tmp || exit 1

# Use the default host compiler rather than assuming Clang.
# TODO: remove once the main build description stops making this assumption.
sed -i '' 's#bin/clang++#bin/c++#' tmp || exit 1

# Relativize paths to avoid hardcoding /home/jon/fab/foo
./strip-make-paths.sh tmp || exit 1

mv tmp GNUmakefile
