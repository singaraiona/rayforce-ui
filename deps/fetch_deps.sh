#!/bin/bash
set -euo pipefail

# Error handler for better reporting
trap 'echo "Error: Command failed at line $LINENO: $BASH_COMMAND" >&2' ERR

cd "$(dirname "$0")"

# Rayforce (core dependency - fetched on fresh build)
if [ ! -d "rayforce" ]; then
    echo "Cloning Rayforce..."
    git clone --depth 1 git@github.com:RayforceDB/rayforce.git
else
    echo "Updating Rayforce..."
    git -C rayforce pull
fi

# Other deps (imgui, implot, glfw, nanosvg) are checked into the repo

echo "Dependencies fetched successfully"
