#!/bin/bash

set -eu

if [ $# -ne 1 ]; then
    echo "Usage: $0 BUILD_PATH"
    exit 1
fi

BUILD_PATH=$1

SAMPLE_ASSETS_PATH=third_party/assets/glTF-Sample-Assets
if [ ! -e "$SAMPLE_ASSETS_PATH" ]; then
    git clone https://github.com/KhronosGroup/glTF-Sample-Assets.git "$SAMPLE_ASSETS_PATH"
else
    echo "$SAMPLE_ASSETS_PATH already exists; skipping clone"
fi
cmake --build "$BUILD_PATH"

TEST_RESULTS_PATH=/tmp/test_gltf_$(date +%s)
python3 ./tools/test_gltf_sample_assets.py \
    --program "$BUILD_PATH/bin/vk_gltf_basic_materials" \
    --model-index "$SAMPLE_ASSETS_PATH/Models/model-index.json" \
    --output "$TEST_RESULTS_PATH"

REPORT_PATH=/tmp/gltf_report_$(date +%s)
python3 ./tools/make_gltf_sample_assets_report.py \
    --input "$TEST_RESULTS_PATH" \
    --model-index "$SAMPLE_ASSETS_PATH/Models/model-index.json" \
    --output "$REPORT_PATH"

echo "Report written to: $REPORT_PATH"
