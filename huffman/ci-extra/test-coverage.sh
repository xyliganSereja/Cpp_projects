#!/bin/bash
set -euo pipefail
IFS=$' \t\n'

mkdir -p cmake-build-Debug
rm -rf cmake-build-Debug/*
CXXFLAGS="-ftest-coverage -fprofile-arcs" LDFLAGS='-lgcov' cmake "-DCMAKE_C_COMPILER=gcc" "-DCMAKE_CXX_COMPILER=g++" "-DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake" -GNinja --preset Debug \
  -DENABLE_SLOW_TEST=ON -DTREAT_WARNINGS_AS_ERRORS=ON -S .
cmake --build cmake-build-Debug

./cmake-build-Debug/unit-tests/tests
lcov --capture --directory cmake-build-Debug --output-file coverage.info --ignore mismatch
genhtml coverage.info --output-directory report
URL="report/index.html"
xdg-open $URL || sensible-browser $URL || x-www-browser $URL || gnome-open $URL
