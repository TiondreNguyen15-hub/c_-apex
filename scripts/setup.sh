#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .deps
fetch_dependency() {
    local name="$1" url="$2" ref="$3" commit="$4"
    if [ ! -d ".deps/$name" ]; then
        git clone --depth 1 --branch "$ref" "$url" ".deps/$name"
    fi
    test "$(git -C ".deps/$name" rev-parse HEAD)" = "$commit" || {
        echo "Unexpected $name version; preserve local changes and restore pinned dependency." >&2
        exit 1
    }
}
fetch_dependency imgui https://github.com/ocornut/imgui.git v1.91.9b f5befd2d29e66809cd1110a152e375a7f1981f06
fetch_dependency SDL https://github.com/libsdl-org/SDL.git release-2.32.0 7a44b1ab002cee6efa56d3b4c0e146b7fbaed80b
make -j2 test
