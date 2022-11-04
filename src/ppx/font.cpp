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

#include "ppx/font.h"

namespace ppx {

Font::Font()
{
}

Font::~Font()
{
}

void Font::AcquireFontMetrics()
{
    stbtt_GetFontVMetrics(
        &mObject->fontInfo,
        &mObject->ascent,
        &mObject->descent,
        &mObject->lineGap);
}

ppx::Result Font::CreateFromFile(const std::filesystem::path& path, ppx::Font* pFont)
{
    if (IsNull(pFont)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if (!std::filesystem::exists(path)) {
        return ppx::ERROR_PATH_DOES_NOT_EXIST;
    }

    std::ifstream is(path.c_str(), std::ios::binary);
    if (!is.is_open()) {
        return ppx::ERROR_BAD_DATA_SOURCE;
    }

    is.seekg(0, std::ios::end);
    size_t size = is.tellg();
    is.seekg(0, std::ios::beg);

    auto object = std::make_shared<Font::Object>();
    if (!object) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    object->fontData.resize(size);
    is.read(reinterpret_cast<char*>(object->fontData.data()), size);

    int stbres = stbtt_InitFont(&object->fontInfo, object->fontData.data(), 0);
    if (stbres == 0) {
        return Result::ERROR_FONT_PARSE_FAILED;
    }

    pFont->mObject = object;
    pFont->AcquireFontMetrics();

    return ppx::SUCCESS;
}

ppx::Result Font::CreateFromMemory(size_t size, const char* pData, ppx::Font* pFont)
{
    if (IsNull(pFont)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    if ((size == 0) || IsNull(pData)) {
        return ppx::ERROR_BAD_DATA_SOURCE;
    }

    auto object = std::make_shared<Font::Object>();
    if (!object) {
        return ppx::ERROR_ALLOCATION_FAILED;
    }

    object->fontData.resize(size);
    std::memcpy(object->fontData.data(), pData, size);

    int stbres = stbtt_InitFont(&object->fontInfo, object->fontData.data(), 0);
    if (stbres == 0) {
        return Result::ERROR_FONT_PARSE_FAILED;
    }

    pFont->mObject = object;
    pFont->AcquireFontMetrics();

    return ppx::SUCCESS;
}

float Font::GetScale(float fontSizeInPixels) const
{
    float scale = stbtt_ScaleForPixelHeight(&mObject->fontInfo, fontSizeInPixels);
    return scale;
}

void Font::GetFontMetrics(float fontSizeInPixels, FontMetrics* pMetrics) const
{
    if (IsNull(pMetrics)) {
        return;
    }

    float scale = stbtt_ScaleForPixelHeight(&mObject->fontInfo, fontSizeInPixels);

    int ascent  = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(
        &mObject->fontInfo,
        &ascent,
        &descent,
        &lineGap);

    pMetrics->ascent  = ascent * scale;
    pMetrics->descent = descent * scale;
    pMetrics->lineGap = lineGap * scale;
}

void Font::GetGlyphMetrics(
    float         fontSizeInPixels,
    uint32_t      codepoint,
    float         subpixelShiftX,
    float         subpixelShiftY,
    GlyphMetrics* pMetrics) const
{
    if (IsNull(pMetrics)) {
        return;
    }

    float scale = GetScale(fontSizeInPixels);

    int advanceWidth = 0;
    int leftBearing  = 0;
    stbtt_GetCodepointHMetrics(
        &mObject->fontInfo,
        static_cast<int>(codepoint),
        &advanceWidth,
        &leftBearing);
    pMetrics->advance     = advanceWidth * scale;
    pMetrics->leftBearing = leftBearing * scale;

    stbtt_GetCodepointBitmapBoxSubpixel(
        &mObject->fontInfo,
        static_cast<int>(codepoint),
        scale,
        scale,
        subpixelShiftX,
        subpixelShiftY,
        &pMetrics->box.x0,
        &pMetrics->box.y0,
        &pMetrics->box.x1,
        &pMetrics->box.y1);
}

void Font::RenderGlyphBitmap(
    float          fontSizeInPixels,
    uint32_t       codepoint,
    float          subpixelShiftX,
    float          subpixelShiftY,
    uint32_t       glyphWidth,
    uint32_t       glyphHeight,
    uint32_t       rowStride,
    unsigned char* pOutput) const
{
    float scale = stbtt_ScaleForPixelHeight(&mObject->fontInfo, fontSizeInPixels);

    stbtt_MakeCodepointBitmapSubpixel(
        &mObject->fontInfo,
        pOutput,
        static_cast<int>(glyphWidth),
        static_cast<int>(glyphHeight),
        static_cast<int>(rowStride),
        scale,
        scale,
        subpixelShiftX,
        subpixelShiftY,
        static_cast<int>(codepoint));
}

//void  Font::GetGlyphBitmap(float fontSizeInPixels)
//{
//    stbtt_MakeCodepointBitmapSubpixel(
//        &mObject->fontInfo,
//        )
//}

//uint2 Font::GetGlyphBounds(float fontSizeInPixels, uint32_t codePoint) const
//{
//    int glyphIndex = stbtt_FindGlyphIndex(&mObject->fontInfo, static_cast<int>(codePoint));
//    if (glyphIndex == 0) {
//        return uint2(0);
//    }
//
//    int advancedWidth   = 0;
//    int leftSideBearing = 0;
//    stbtt_GetGlyphHMetrics(&mObject->fontInfo, glyphIndex, &advancedWidth, &leftSideBearing);
//
//    float scale  = stbtt_ScaleForPixelHeight(&mObject->fontInfo, fontSizeInPixels);
//    float width  = (advancedWidth * scale) + 0.5f;
//    float height = static_cast<float>(mObject->ascent + abs(mObject->descent)) * scale + 0.5f;
//
//    uint2 bounds = uint2(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
//    return bounds;
//}

} // namespace ppx
