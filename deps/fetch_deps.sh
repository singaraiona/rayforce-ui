#!/bin/bash
set -euo pipefail

# Error handler for better reporting
trap 'echo "Error: Command failed at line $LINENO: $BASH_COMMAND" >&2' ERR

cd "$(dirname "$0")"

# ImGui (docking branch for dock support)
if [ ! -d "imgui" ]; then
    echo "Cloning ImGui (docking branch)..."
    git clone --depth 1 -b docking https://github.com/ocornut/imgui.git
else
    echo "Updating ImGui..."
    git -C imgui pull
fi

# ImPlot (for charts)
if [ ! -d "implot" ]; then
    echo "Cloning ImPlot..."
    git clone --depth 1 https://github.com/epezent/implot.git
else
    echo "Updating ImPlot..."
    git -C implot pull
fi

# Note: GLFW must be installed via system package manager
# e.g., apt install libglfw3-dev (Debian/Ubuntu)
#       brew install glfw (macOS)
#       dnf install glfw-devel (Fedora)

echo "Dependencies fetched successfully"
