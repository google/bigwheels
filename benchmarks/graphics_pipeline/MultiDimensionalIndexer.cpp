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

#include "MultiDimensionalIndexer.h"

using namespace ppx;

void MultiDimensionalIndexer::AddDimension(size_t size)
{
    for (size_t i = 0; i < mMultipliers.size(); i++) {
        mMultipliers[i] *= size;
    }
    mSizes.push_back(size);
    mMultipliers.push_back(1);
}

size_t MultiDimensionalIndexer::GetIndex(const std::vector<size_t>& indices)
{
    PPX_ASSERT_MSG(indices.size() == mSizes.size(), "The number of indices must be the same as the number of dimensions");
    size_t index = 0;
    for (size_t i = 0; i < indices.size(); i++) {
        PPX_ASSERT_MSG(indices[i] < mSizes[i], "Index out of range");
        index += indices[i] * mMultipliers[i];
    }
    return index;
}