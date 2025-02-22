// Copyright 2022 Google LLC
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
// #define STB_TRUETYPE_IMPLEMENTATION
// #include "stb_truetype.h"

#include "ppx/ppx.h"
#include "ppx/bitmap.h"

#include "utf8.h"
#include <filesystem>
#include <iostream>
#include <vector>

using namespace ppx;

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cout << "Run " << argv[0] << "[font file path]  [output folder]";
        return 0;
    }
    std::string font_file_path = std::string(argv[1]);
    std::string output_folder  = std::string(argv[2]);

    if (!std::filesystem::exists(font_file_path)) {
        std::cout << "File " << font_file_path << " does not exist!" << std::endl;
        return -1;
    }

    std::string filename             = std::filesystem::path(font_file_path).filename().stem();
    std::string path_base            = output_folder + "/" + filename;
    std::string output_png_path      = path_base + ".png";
    std::string output_metafile_path = path_base + ".inc";
    std::cout << "png file output path: " << output_png_path << std::endl;
    std::cout << "include file path: " << output_metafile_path << std::endl;
    std::ofstream f(output_metafile_path);
    if (!f.is_open()) {
        std::cout << "Failed to create " << output_metafile_path << std::endl;
        return -1;
    }

    ppx::Font font;
    PPX_CHECKED_CALL(ppx::Font::CreateFromFile(font_file_path, &font));

    float                                      fontSize   = 48.0f;
    std::string                                characters = grfx::TextureFont::GetDefaultCharacters();
    FontMetrics                                fontMetrics;
    std::vector<grfx::TextureFontGlyphMetrics> glyphMetrics;

    font.GetFontMetrics(fontSize, &fontMetrics);

    float lineSpacing = fontMetrics.ascent - fontMetrics.descent + fontMetrics.lineGap;

    // Subpixel shift
    float subpixelShiftX = 0.5f;
    float subpixelShiftY = 0.5f;

    // Get glyph metrics and max bounds
    utf8::iterator<std::string::iterator> it(characters.begin(), characters.begin(), characters.end());
    utf8::iterator<std::string::iterator> it_end(characters.end(), characters.begin(), characters.end());
    bool                                  hasSpace = false;
    while (it != it_end) {
        const uint32_t codepoint = utf8::next(it, it_end);
        GlyphMetrics   metrics   = {};
        font.GetGlyphMetrics(fontSize, codepoint, subpixelShiftX, subpixelShiftY, &metrics);
        glyphMetrics.emplace_back(grfx::TextureFontGlyphMetrics{codepoint, metrics});

        if (!hasSpace) {
            hasSpace = (codepoint == 32);
        }
    }
    if (!hasSpace) {
        const uint32_t codepoint = 32;
        GlyphMetrics   metrics   = {};
        font.GetGlyphMetrics(fontSize, codepoint, subpixelShiftX, subpixelShiftY, &metrics);
        glyphMetrics.emplace_back(grfx::TextureFontGlyphMetrics{codepoint, metrics});
    }

    // Figure out a squarish somewhat texture size
    const size_t  nc           = characters.size();
    const int32_t sqrtnc       = static_cast<int32_t>(sqrtf(static_cast<float>(nc)) + 0.5f) + 1;
    int32_t       bitmapWidth  = 0;
    int32_t       bitmapHeight = 0;
    size_t        glyphIndex   = 0;
    for (int32_t i = 0; (i < sqrtnc) && (glyphIndex < nc); ++i) {
        int32_t height = 0;
        int32_t width  = 0;
        for (int32_t j = 0; (j < sqrtnc) && (glyphIndex < nc); ++j, ++glyphIndex) {
            const GlyphMetrics& metrics = glyphMetrics[glyphIndex].glyphMetrics;
            // int32_t             w       = (metrics.box.x1 - metrics.box.x0) + 1;
            int32_t w = metrics.advance + 1;
            int32_t h = (metrics.box.y1 - metrics.box.y0) + 1;
            width     = width + w;
            height    = std::max<int32_t>(height, h);
        }
        bitmapWidth  = std::max<int32_t>(bitmapWidth, width);
        bitmapHeight = bitmapHeight + height;
    }

    std::cout << "bitmapWidth: " << bitmapWidth << ", bitmapHeight: " << bitmapHeight << std::endl;
    // Storage bitmap
    Bitmap bitmap = Bitmap::Create(bitmapWidth, bitmapHeight, Bitmap::Format::FORMAT_R_UINT8);

    const std::string content = R"(/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Copyright (C) 2025 Google LLC
 * The contents of this package are proprietary, confidential information, and
 * are subject to a license agreement between Google and your company.
 */

// Do not modify, code generated using
// https://github.com/google/bigwheels/blob/gen_font_texture/projects/gen_font_texture/main.cpp

#pragma once

namespace android {
namespace {

struct Vertex {
    float2 position[4];
    float2 uv[4];
};

)";

    f << content;
    f << "constexpr int32_t kCharStart   = " << int32_t(*characters.begin()) << ";" << std::endl;
    f << "constexpr int32_t kCharEnd     = " << int32_t(characters.back()) << ";" << std::endl;
    f << "constexpr int32_t kCharCount   = kCharEnd - kCharStart + 1;" << std::endl;
    f << "constexpr int32_t kLineSpace   = " << int32_t(lineSpacing) << ";" << std::endl;

    f << R"(
// clang-format off
const Vertex textVertices[kCharCount] = {
)";

    // Render glyph bitmaps
    const float    invBitmapWidth  = 1.0f / static_cast<float>(bitmapWidth);
    const float    invBitmapHeight = 1.0f / static_cast<float>(bitmapHeight);
    const uint32_t rowStride       = bitmap.GetRowStride();
    const uint32_t pixelStride     = bitmap.GetPixelStride();
    uint32_t       y               = 0;
    glyphIndex                     = 0;
    for (int32_t i = 0; (i < sqrtnc) && (glyphIndex < nc); ++i) {
        uint32_t x      = 0;
        uint32_t height = 0;
        for (int32_t j = 0; (j < sqrtnc) && (glyphIndex < nc); ++j, ++glyphIndex) {
            uint32_t            codepoint = glyphMetrics[glyphIndex].codepoint;
            const GlyphMetrics& metrics   = glyphMetrics[glyphIndex].glyphMetrics;
            // uint32_t            w         = static_cast<uint32_t>(metrics.box.x1 - metrics.box.x0) + 1;
            uint32_t w = metrics.advance + 1;
            uint32_t h = static_cast<uint32_t>(metrics.box.y1 - metrics.box.y0) + 1;

            uint32_t offset  = (y * rowStride) + (x * pixelStride);
            char*    pOutput = bitmap.GetData() + offset;
            font.RenderGlyphBitmap(fontSize, codepoint, subpixelShiftX, subpixelShiftY, w, h, rowStride, reinterpret_cast<unsigned char*>(pOutput));

            glyphMetrics[glyphIndex].size.x = static_cast<float>(w);
            glyphMetrics[glyphIndex].size.y = static_cast<float>(h);

            glyphMetrics[glyphIndex].uvRect.u0 = x * invBitmapWidth;
            glyphMetrics[glyphIndex].uvRect.v0 = y * invBitmapHeight;
            glyphMetrics[glyphIndex].uvRect.u1 = (x + w - 1) * invBitmapWidth;
            glyphMetrics[glyphIndex].uvRect.v1 = (y + h - 1) * invBitmapHeight;

            x             = x + w;
            height        = std::max<int32_t>(height, h);
            auto pMetrics = &glyphMetrics[glyphIndex];

            float2 P   = float2(0, pMetrics->glyphMetrics.box.y0);
            float2 P0  = P;
            float2 P1  = P + float2(0, pMetrics->size.y);
            float2 P2  = P + pMetrics->size;
            float2 P3  = P + float2(pMetrics->size.x, 0);
            float2 uv0 = float2(pMetrics->uvRect.u0, pMetrics->uvRect.v0);
            float2 uv1 = float2(pMetrics->uvRect.u0, pMetrics->uvRect.v1);
            float2 uv2 = float2(pMetrics->uvRect.u1, pMetrics->uvRect.v1);
            float2 uv3 = float2(pMetrics->uvRect.u1, pMetrics->uvRect.v0);

            f << "    {";
            // pos
            f << "{";
            f.unsetf(std::ios_base::floatfield);
            f << "{" << P0.x << ", " << std::setw(3) << P0.y << "},";
            f << "{" << P1.x << ", " << std::setw(3) << P1.y << "},";
            f << "{" << P2.x << ", " << std::setw(3) << P2.y << "},";
            f << "{" << P3.x << ", " << std::setw(3) << P3.y << "}";
            f << "},";
            // uv

            f << "{" << std::fixed << std::setprecision(7);
            f << "{" << uv0.x << ", " << uv0.y << "},";
            f << "{" << uv1.x << ", " << uv1.y << "},";
            f << "{" << uv2.x << ", " << uv2.y << "},";
            f << "{" << uv3.x << ", " << uv3.y << "}";
            f << "}";

            f << "},";
            f << "    // '" << char(codepoint) << "'";
            f << std::endl;
        }
        y += height;
    }

    f << "};" << std::endl;
    f << "// clang-format on" << std::endl;
    f << "} // namespace" << std::endl;
    f << "} // namespace android" << std::endl;
    f.close();

    return Bitmap::SaveFilePNG(output_png_path, &bitmap);
}