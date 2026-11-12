# Replay Studio

A C++17 Dear ImGui + SDL2 player visualization UI driven by local replay data.

- Toggle player names, bounding boxes, and recorded skeletons independently.
- Edit name, box, and bone RGBA colors and line thickness (1–8 pixels).
- Play, pause, restart, and scrub a replay. Reset display styles with one button.
- Runs independently, with no game process integration or credentials.

## Build and run

Requires Linux, `g++`, `make`, `git`, and the SDL2 runtime (`libSDL2-2.0.so.0`).
The cloud image already supplies these. On Debian/Ubuntu, the SDL runtime package
is `libsdl2-2.0-0`. An interactive run needs a graphical desktop/display.

```sh
cd /workspace/c_-apex
bash scripts/setup.sh
./build/replay-viewer data/sample.replay
```

Setup fetches pinned Dear ImGui 1.91.9b and SDL 2.32.0 headers into ignored
`.deps/`, checks their exact Git commits, builds, and runs the tests. SDL uses a
software renderer, so no OpenGL development packages or GPU are required.
Dependencies keep their upstream licenses in `.deps/imgui/LICENSE.txt` and
`.deps/SDL/LICENSE.txt`. The executable and dependency caches are ignored.

## Replay format

Each non-comment line is one player observation:

```text
time_seconds "player name" x y width height j0x j0y ... j9x j9y
```

Times must be nonnegative and ascending. Rows with the same timestamp form a
frame. Names must be unique within a frame. Bounds use normalized viewport
coordinates (top-left origin), and must fit within `[0,1]`. All ten joint pairs
use normalized coordinates relative to the player's bounds. Joint order:
head, neck, left shoulder, left hand, right shoulder, right hand, hips,
left foot, right knee, right foot. Bone segments connect these recorded points.
The bundled data is synthetic sample replay data. Frames are stepped without
interpolation; the last timestamp is the loop endpoint. Use a replay path as the
first argument to load another file. Malformed data reports an error and exits.

## Validation

```sh
make test
```

This checks valid replay loading and rejects empty input, out-of-bounds boxes,
out-of-order frames, duplicate names, missing joints, and unexpected fields.
It also creates an SDL software renderer under the dummy video driver and
renders three Dear ImGui replay frames. This works without a desktop and does
not substitute for a manual interaction check on a graphical display.
