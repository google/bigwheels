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

#ifndef ppx_util_h
#define ppx_util_h

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

namespace ppx {

struct RangeU32
{
    uint32_t start;
    uint32_t end;
};

template <typename T>
bool IsNull(const T* ptr)
{
    bool res = (ptr == nullptr);
    return res;
}

template <typename T>
T InvalidValue(const T value = static_cast<T>(~0))
{
    return value;
}

template <typename T>
T RoundUp(T value, T multiple)
{
    static_assert(
        std::is_integral<T>::value,
        "T must be an integral type");

    assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (value + multiple - 1) & ~(multiple - 1);
}

template <typename T>
uint32_t CountU32(const std::vector<T>& container)
{
    uint32_t n = static_cast<uint32_t>(container.size());
    return n;
}

template <typename T>
T* DataPtr(std::vector<T>& container)
{
    T* ptr = container.empty() ? nullptr : container.data();
    return ptr;
}

template <typename T>
const T* DataPtr(const std::vector<T>& container)
{
    const T* ptr = container.empty() ? nullptr : container.data();
    return ptr;
}

template <typename T>
uint32_t SizeInBytesU32(const std::vector<T>& container)
{
    uint32_t size = static_cast<uint32_t>(container.size() * sizeof(T));
    return size;
}

template <typename T>
uint64_t SizeInBytesU64(const std::vector<T>& container)
{
    uint64_t size = static_cast<uint64_t>(container.size() * sizeof(T));
    return size;
}

template <typename T>
bool IsIndexInRange(uint32_t index, const std::vector<T>& container)
{
    uint32_t n   = CountU32(container);
    bool     res = (index < n);
    return res;
}

// Returns true if [a,b) overlaps with [c, d)
inline bool HasOverlapHalfOpen(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    bool overlap = std::max<uint32_t>(a, c) < std::min<uint32_t>(c, d);
    return overlap;
}

inline bool HasOverlapHalfOpen(const ppx::RangeU32& r0, const ppx::RangeU32& r1)
{
    return HasOverlapHalfOpen(r0.start, r0.end, r1.start, r1.end);
}

template <typename T>
typename std::vector<T>::iterator Find(std::vector<T>& container, const T& element)
{
    typename std::vector<T>::iterator it = std::find(
        std::begin(container),
        std::end(container),
        element);
    return it;
}

template <typename T>
typename std::vector<T>::const_iterator Find(const std::vector<T>& container, const T& element)
{
    typename std::vector<T>::iterator it = std::find(
        std::begin(container),
        std::end(container),
        element);
    return it;
}

template <typename T, class UnaryPredicate>
typename std::vector<T>::iterator FindIf(std::vector<T>& container, UnaryPredicate predicate)
{
    typename std::vector<T>::iterator it = std::find_if(
        std::begin(container),
        std::end(container),
        predicate);
    return it;
}

template <typename T, class UnaryPredicate>
typename std::vector<T>::const_iterator FindIf(const std::vector<T>& container, UnaryPredicate predicate)
{
    typename std::vector<T>::const_iterator it = std::find_if(
        std::begin(container),
        std::end(container),
        predicate);
    return it;
}

template <typename T>
bool ElementExists(const T& elem, const std::vector<T>& container)
{
    auto it     = std::find(std::begin(container), std::end(container), elem);
    bool exists = (it != std::end(container));
    return exists;
}

template <typename T>
bool GetElement(uint32_t index, const std::vector<T>& container, T* pElem)
{
    if (!IsIndexInRange(index, container)) {
        return false;
    }
    *pElem = container[index];
    return true;
}

template <typename T>
void AppendElements(const std::vector<T>& elements, std::vector<T>& container)
{
    if (!elements.empty()) {
        std::copy(
            std::begin(elements),
            std::end(elements),
            std::back_inserter(container));
    }
}

template <typename T>
void RemoveElement(const T& elem, std::vector<T>& container)
{
    container.erase(
        std::remove(std::begin(container), std::end(container), elem),
        container.end());
}

template <typename T, class UnaryPredicate>
void RemoveElementIf(std::vector<T>& container, UnaryPredicate predicate)
{
    container.erase(
        std::remove_if(std::begin(container), std::end(container), predicate),
        container.end());
}

template <typename T>
void Unique(std::vector<T>& container)
{
    auto it = std::unique(std::begin(container), std::end(container));
    container.resize(std::distance(std::begin(container), it));
}

inline std::vector<const char*> GetCStrings(const std::vector<std::string>& container)
{
    std::vector<const char*> cstrings;
    for (auto& str : container) {
        cstrings.push_back(str.c_str());
    }
    return cstrings;
}

inline std::vector<std::string> GetNotFound(const std::vector<std::string>& search, const std::vector<std::string>& container)
{
    std::vector<std::string> result;
    for (auto& elem : search) {
        auto it = std::find(
            std::begin(container),
            std::end(container),
            elem);
        if (it == std::end(container)) {
            result.push_back(elem);
        }
    }
    return result;
}

inline std::string FloatString(float value, int precision = 6, int width = 6)
{
    std::stringstream ss;
    ss << std::setprecision(precision) << std::setw(6) << std::fixed << value;
    return ss.str();
}

struct Extent2D
{
    uint32_t width;
    uint32_t height;
};

} // namespace ppx

#endif // ppx_util_h
