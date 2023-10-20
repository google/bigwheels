// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BENCHMARKS_GRAPHICS_PIPELINE_MULTI_DIMENSIONAL_INDEXER_H
#define BENCHMARKS_GRAPHICS_PIPELINE_MULTI_DIMENSIONAL_INDEXER_H

#include "ppx/ppx.h"

using namespace ppx;

// TODO: Consider refactoring
// Indexer that calculates a single dimensional index from a multi-dimensional index
class MultiDimensionalIndexer
{
public:
    // Adds a new dimension with the given `size`.
    void AddDimension(size_t size);

    // Gets the index for the given dimension `indices`.
    size_t GetIndex(const std::vector<size_t>& indices);

private:
    // `mSizes` are the sizes for each dimension.
    std::vector<size_t> mSizes;
    // `mMultipliers` are the multipliers for each dimension to get the index.
    std::vector<size_t> mMultipliers;
};

#endif // BENCHMARKS_GRAPHICS_PIPELINE_MULTI_DIMENSIONAL_INDEXER_H