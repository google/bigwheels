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

#ifndef ppx_knob_h
#define ppx_knob_h

#include <functional>
#include <iostream>
#include <vector>

namespace ppx {

// ---------------------------------------------------------------------------------------------
// KnobType
// ---------------------------------------------------------------------------------------------

enum class KnobType
{
    Unknown
};

std::ostream& operator<<(std::ostream& strm, KnobType kt);

// ---------------------------------------------------------------------------------------------
// KnobConfigs
// ---------------------------------------------------------------------------------------------

struct KnobConfig
{
    std::string flagName;
    std::string flagDesc;
    std::string displayName;
    int         parentId;
};

// ---------------------------------------------------------------------------------------------
// Knob Classes
// ---------------------------------------------------------------------------------------------

class Knob
{
public:
    Knob() {}
    Knob(KnobConfig config, KnobType type)
        : displayName(config.displayName), flagName(config.flagName), flagDesc(config.flagDesc), type(type) {}
    virtual ~Knob() {}

    std::string        GetDisplayName() { return displayName; }
    std::string        GetFlagName() { return flagName; }
    std::string        GetFlagDesc() { return flagDesc; }
    KnobType           GetType() { return type; }
    void               AddChild(Knob* child) { children.push_back(child); }
    std::vector<Knob*> GetChildren() { return children; }

protected:
    std::string        displayName;
    std::string        flagName;
    std::string        flagDesc;
    KnobType           type;
    std::vector<Knob*> children;
};

} // namespace ppx

#endif // ppx_knob_h
