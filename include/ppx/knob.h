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

#include "ppx/command_line_parser.h"
#include "ppx/config.h"
#include "ppx/log.h"

#include <functional>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace ppx {

template <typename T>
using OnChangeCallback = std::function<void(const T& value)>;

// ---------------------------------------------------------------------------------------------
// Knob Classes
// ---------------------------------------------------------------------------------------------

class Knob;
class KnobBoolCheckbox;
class KnobIntSlider;
class KnobStrDropdown;
class KnobManager;

// KnobManager owns the knobs in an application
class KnobManager
{
public:
    KnobManager() {}
    virtual ~KnobManager();
    KnobManager(const KnobManager&)            = delete;
    KnobManager& operator=(const KnobManager&) = delete;

private:
    // mRoots is used for drawing with hierarchy
    std::vector<Knob*> mRoots;

    // mKnobs is used for operations that affect all knobs
    // Knobs are added on creation and never deleted until knobManager is destroyed
    std::vector<Knob*> mKnobs;

    // mFlagNames is kept to prevent multiple knobs having the same mFlagName
    std::unordered_set<std::string> mFlagNames;

public:
    // Utilities
    bool IsEmpty() { return mRoots.empty(); }
    void ResetAllToDefault();

    // Create knobs
    template <typename T, typename... ArgsT>
    T* CreateKnob(const std::string& flagName, Knob* parent, ArgsT... args)
    {
        for (auto ch : flagName) {
            PPX_ASSERT_MSG(!isspace(ch), "knob cannot be created as its name has whitespace");
        }
        PPX_ASSERT_MSG(mFlagNames.count(flagName) == 0, "knob with this name already exists");

        T* knobPtr = new T(flagName, std::forward<ArgsT>(args)...);

        RegisterKnob(flagName, knobPtr, parent);
        return knobPtr;
    }

    // ImGui
    void DrawAllKnobs(bool inExistingWindow = false);

    // Command-line flags
    std::string GetUsageMsg();
    void        UpdateFromFlags(const CliOptions& opts);

    // RegisterKnob will store newly created knob
    void RegisterKnob(std::string flagName, Knob* newKnob, Knob* parent);
};

// Knob is an abstract class which contains common features for all knobs and knob hierarchy
class Knob
{
    friend void KnobManager::RegisterKnob(std::string flagName, Knob* newKnob, Knob* parent);

public:
    Knob(const std::string& flagName)
        : mFlagName(flagName), mDisplayName(flagName) {}

    std::string        GetDisplayName() const { return mDisplayName; }
    std::string        GetFlagName() const { return mFlagName; }
    std::string        GetFlagHelp() const { return mFlagDesc; }
    std::vector<Knob*> GetChildren() const { return mChildren; }

    // Populate flag members
    void SetDisplayName(std::string displayName) { mDisplayName = displayName; }
    void SetFlagDesc(std::string flagDesc) { mFlagDesc = flagDesc; }

    virtual void Draw() = 0;

    // Flag description in usage message
    virtual std::string GetFlagHelpText() const = 0;

    // Updates knob value from commandline flag
    virtual void UpdateFromFlag(const CliOptions& opts) = 0;

    // Default values are the values when the application starts. The knobs can be reset to default by a button in the UI
    virtual void ResetToDefault() = 0;

private:
    // Child knobs are displayed in the UI below their parent and slightly indented
    void AddChild(Knob* pChild) { mChildren.push_back(pChild); }

protected:
    std::string        mDisplayName;
    std::string        mFlagName;
    std::string        mFlagDesc;
    std::vector<Knob*> mChildren;
};

// KnobBoolCheckbox is a boolean knob that will be displayed as a checkbox in the UI
class KnobBoolCheckbox
    : public Knob
{
public:
    KnobBoolCheckbox(const std::string& flagName, bool defaultValue, OnChangeCallback<bool> callback = nullptr)
        : Knob(flagName)
    {
        mCallback = callback;
        SetDefault(defaultValue);
    }

    void Draw() override;
    void ResetToDefault() override { SetValue(mDefaultValue); }

    // GetFlagHelpText format:
    // --flag_name <true|false> : flag description
    std::string GetFlagHelpText() const override;

    // Updates mValue and mDefaultValue, expected flag format:
    // --flag_name <true|false>
    void UpdateFromFlag(const CliOptions& opts) override;

    bool GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    // If newVal is different from mValue:
    //   - mValue is updated
    //   - mCallback is triggered
    void SetValue(bool newVal);

private:
    // SetDefault will set mDefaultValue and update mValue
    void SetDefault(bool newVal);

private:
    bool mValue;
    bool mDefaultValue;

    // mCallback will be called on the new bool value every time it is toggled in the UI, or updated with SetValue()
    OnChangeCallback<bool> mCallback;
};

// KnobIntSlider is an int knob that will be displayed as a slider in the UI
// ImGui sliders can also become input boxes with ctrl + right click
class KnobIntSlider
    : public Knob
{
public:
    KnobIntSlider(const std::string& flagName, int defaultValue, int minValue, int maxValue, OnChangeCallback<int> callback = nullptr)
        : Knob(flagName)
    {
        PPX_ASSERT_MSG(minValue < maxValue, "invalid range to initialize slider");
        PPX_ASSERT_MSG(minValue <= defaultValue && defaultValue <= maxValue, "defaultValue is out of range");

        mMinValue = minValue;
        mMaxValue = maxValue;
        mCallback = callback;
        SetDefault(defaultValue);
    }

    void Draw() override;
    void ResetToDefault() override { SetValue(mDefaultValue); }

    // GetFlagHelpText format:
    // --flag_name <mMinValue~mMaxValue> : flag description
    std::string GetFlagHelpText() const override;

    // Updates mValue and mDefaultValue, expected flag format:
    // --flag_name <int>
    void UpdateFromFlag(const CliOptions& opts) override;

    int GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    // If newVal is valid and different from mValue:
    //   - mValue is updated
    //   - mCallback is triggered
    void SetValue(int newVal);

private:
    // IsValidValue returns true if val is within allowed range
    bool IsValidValue(int val);

    // SetDefault will set mDefaultValue and update mValue
    void SetDefault(int newVal);

private:
    int mValue;
    int mDefaultValue;

    // mValue will be clamped to the mMinValue to mMaxValue range, inclusive
    int mMinValue;
    int mMaxValue;

    // mCallback will be called on the new int value every time the slider is deselected in the UI, or updated with SetIntValue()
    OnChangeCallback<int> mCallback;
};

// KnobStrDropdown is an int knob that will be displayed as a dropdown in the UI
// The knob stores the index of a selected choice from a list of allowed strings
class KnobStrDropdown
    : public Knob
{
public:
    KnobStrDropdown(const std::string& flagName, size_t defaultIndex, std::vector<std::string>::iterator choicesBegin, std::vector<std::string>::iterator choicesEnd, OnChangeCallback<size_t> callback = nullptr)
        : Knob(flagName)
    {
        PPX_ASSERT_MSG(choicesBegin != choicesEnd, "too few allowed choices");
        PPX_ASSERT_MSG(static_cast<int>(defaultIndex) > 0 && static_cast<int>(defaultIndex) < choicesEnd - choicesBegin, "defaultIndex is out of range");

        mChoices  = std::vector<std::string>(choicesBegin, choicesEnd);
        mCallback = callback;
        SetDefault(defaultIndex);
    }

    void Draw() override;
    void ResetToDefault() override { SetIndex(mDefaultIndex); }

    // GetFlagHelpText format:
    // --flag_name <"choice1"|"choice2"|...> : flag description
    std::string GetFlagHelpText() const override;

    // Updates mIndex and mDefaultIndex, expected flag format:
    // --flag_name <str>
    void UpdateFromFlag(const CliOptions& opts) override;

    size_t      GetIndex() const { return mIndex; }
    std::string GetStr() const { return mChoices[mIndex]; }

    // Used for when mIndex needs to be updated outside of UI
    // If newI is valid and different from mIndex:
    //   - mIndex is updated
    //   - mCallback is triggered
    void SetIndex(size_t newI);

    // Used for setting from flags
    // If newVal matches a choice exactly:
    //   - mIndex is updated
    //   - mCallback is triggered
    void SetIndex(std::string newVal);

private:
    // IsValidIndex returns true if val is within allowed range
    bool IsValidIndex(size_t val);

    // SetDefault will set mDefaultIndex and update mIndex
    void SetDefault(size_t newI);
    void SetDefault(std::string newVal);

private:
    // mIndex indicates which of the mChoices is selected
    size_t                   mIndex;
    size_t                   mDefaultIndex;
    std::vector<std::string> mChoices;

    // mCallback will be called on the new index every time the dropdown is deselected in the UI, or updated with SetIndex()
    OnChangeCallback<size_t> mCallback;
};

// Helper functions
void DrawKnobs(const std::vector<Knob*>& knobPtrs);

} // namespace ppx

#endif // ppx_knob_h
