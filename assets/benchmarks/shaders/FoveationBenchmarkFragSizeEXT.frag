
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

#version 450
#extension GL_EXT_fragment_invocation_density : enable

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform Params
{
    // Seed for pseudo-random noise generator.
    // This is combined with the fragment location to generate the color.
    uint seed;

    // Number of additional hash computations to perform for each fragment.
    uint extraHashRounds;

    // Per-channel weights of the noise in the output color.
    vec3 noiseWeights;

    // Color to mix with the noise.
    vec3 color;
}
params;

// Computes a pseudo-random hash.
uint pcg_hash(uint value)
{
    uint state = value * 747796405u + 2891336453u;
    uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Get the output color, which is a mix of pseudo-random noise and a constant
// color.
vec4 getColor(uint seed, uint extraHashRounds, vec3 noiseWeights, vec3 color, vec2 position)
{
    uint hashWord = seed;
    hashWord      = pcg_hash(hashWord ^ uint(position.x));
    hashWord      = pcg_hash(hashWord ^ uint(position.y));
    for (int i = 0; i < extraHashRounds; ++i) {
        hashWord = pcg_hash(hashWord);
    }
    vec3 noise = vec3(
        float(hashWord & 0xFF) / 255.0,
        float((hashWord >> 8) & 0xFF) / 255.0,
        float((hashWord >> 16) & 0xFF) / 255.0);
    return vec4(mix(color, noise, noiseWeights), 1.0);
}

void main()
{
    // Adjust params so that color G&B channels are shading density, and noise
    // is removed from these channels.
    ivec2 fragSize       = gl_FragSizeEXT;
    vec2  shadingDensity = vec2(1.0 / float(fragSize.x), 1.0 / float(fragSize.y));
    vec3  color          = vec3(params.color.x, shadingDensity);
    vec3  noiseWeights   = vec3(params.noiseWeights.x, 0.001, 0.001);
    outColor             = getColor(params.seed, params.extraHashRounds, noiseWeights, color, gl_FragCoord.xy);
}
