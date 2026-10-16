#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_build_dir="$project_dir/.pio/host-tests"
mkdir -p "$test_build_dir"
cd "$project_dir"
cc -std=c11 -Wall -Wextra -Werror -pedantic -I main \
    -c main/traffic_light.c -o "$test_build_dir/traffic_light.o"
cc -std=c11 -Wall -Wextra -Werror -pedantic -I main \
    tests/test_traffic_light.c "$test_build_dir/traffic_light.o" -o "$test_build_dir/test-core"
c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I tests/arduino -I main \
    tests/test_firmware.cpp "$test_build_dir/traffic_light.o" -o "$test_build_dir/test-firmware"
"$test_build_dir/test-core"
"$test_build_dir/test-firmware"
