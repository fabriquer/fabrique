#!/bin/sh
#
# This script is a workaround for the fact that llvm-lit doesn't accept -MMD or -MF
# flags. If it did, we could make *it* enumerate the test dependencies for us.
#

echo "#"
echo "# Generated at `date` by `basename $0`"
echo "#"
echo "# WARNING: Do not modify by hand!"
echo "#          Re-run $0 when adding, removing or renaming tests."
echo "#"
echo "# TODO: make llvm-lit accept -MMD -MMF flags so we can remove this hack"
echo "#"
echo "all_files = files("

#
# It would be nice to use llvm-lit --show-tests to do this, but that doesn't include
# test dependencies within Inputs/ directories.
#
find . -type f -name '*.fab' | grep -v './manifest.fab'

#${llvm_lit} --show-tests ${test_suite} \
#	| grep fab \
#	| awk -F "::" -v date="${date}" -f ${script_dir}/list-tests.awk

echo ");"
