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

#include "VsOutput.hlsli"

float randomCompute(uint32_t instCount, float4 Position) {
  float ret = frac(sin(Position.x) + cos(Position.y));
  for (int i = 0; i < instCount; i++) {
    ret = frac(sin(ret) + cos(ret));
  }

  return ret;
}

VSOutputPos vsmain(float4 Position : POSITION) {
  VSOutputPos result;
  result.position = Position;
  result.position.z = randomCompute(Config.InstCount, result.position);
  return result;
}