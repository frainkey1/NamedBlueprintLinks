#!/usr/bin/env bash
set -eu
task_root="$(cd "$(dirname "$0")/.." && pwd)"
test_binary="$(mktemp /tmp/nbl-tests.XXXXXX)"
trap 'rm -f "$test_binary"' EXIT
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -I "$task_root/Source/NamedBlueprintLinksNodes/Public" \
    "$task_root/Tests/RoutePlannerTests.cpp" -o "$test_binary"
"$test_binary"
