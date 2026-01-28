#!/bin/bash
set -e
cd "$(dirname "$0")"

# ImGui (docking branch for dock support)
if [ ! -d "imgui" ]; then
    git clone --depth 1 -b docking https://github.com/ocornut/imgui.git
else
    (cd imgui && git pull)
fi

# ImPlot (for charts)
if [ ! -d "implot" ]; then
    git clone --depth 1 https://github.com/epezent/implot.git
else
    (cd implot && git pull)
fi

# GLFW (windowing)
if [ ! -d "glfw" ]; then
    git clone --depth 1 https://github.com/glfw/glfw.git
else
    (cd glfw && git pull)
fi

echo "Dependencies fetched successfully"
