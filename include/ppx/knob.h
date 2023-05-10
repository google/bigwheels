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

// ---------------------------------------------------------------------------------------------
// Knob Classes
// ---------------------------------------------------------------------------------------------

// Knob is an abstract class which contains common features for all knobs and knob hierarchy
class Knob
{
public:
    Knob(const std::string& displayName, const std::string& flagName, const std::string& flagDesc)
        : mDisplayName(displayName), mFlagName(flagName), mFlagDesc(flagDesc) {}

    std::string GetDisplayName() const { return mDisplayName; }
    std::string GetFlagName() const { return mFlagName; }
    std::string GetFlagDesc() const { return mFlagDesc; }

    // Child knobs are displayed in the UI below their parent and slightly indented
    void               AddChild(Knob* pChild) { mChildren.push_back(pChild); }
    std::vector<Knob*> GetChildren() const { return mChildren; }

    virtual void Draw() = 0;

    // Flag description in usage message
    virtual std::string FlagText() const = 0;

    // Updates knob value from commandline flag
    virtual Result UpdateFromFlag(const CliOptions& opts) = 0;

    // Default values are the values when the application starts. The knobs can be reset to default by a button in the UI
    virtual void ResetToDefault() = 0;

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
    KnobBoolCheckbox(const std::string& displayName, const std::string& flagName, const std::string& flagDesc, bool defaultValue, std::function<void(bool)> callback = NULL)
        : Knob(displayName, flagName, flagDesc)
    {
        mCallback = callback;
        SetValue(defaultValue, true);
    }

    void Draw() override;
    void ResetToDefault() override { SetValue(mDefaultValue); }

    // FlagText format:
    // --flag_name <true|false> : flag description
    std::string FlagText() const override;

    // Updates mValue and mDefaultValue, expected flag format:
    // --flag_name <true|false>
    Result UpdateFromFlag(const CliOptions& opts) override;

    bool GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    // If newVal is valid:
    // - If updateDefault, mDefaultValue is updated
    // - If newVal is different from mValue:
    //   - mValue is updated
    //   - mCallback is triggered
    void SetValue(bool newVal, bool updateDefault = false);

private:
    bool mValue;
    bool mDefaultValue;

    // mCallback will be called on the new bool value every time it is toggled in the UI, or updated with SetValue()
    std::function<void(bool)> mCallback;
};

// KnobIntSlider is an int knob that will be displayed as a slider in the UI
// ImGui sliders can also become input boxes with ctrl + right click
class KnobIntSlider
    : public Knob
{
public:
    KnobIntSlider(const std::string& displayName, const std::string& flagName, const std::string& flagDesc, int defaultValue, int minValue, int maxValue, std::function<void(int)> callback = NULL)
        : Knob(displayName, flagName, flagDesc)
    {
        if (minValue >= maxValue) {
            PPX_LOG_FATAL(flagName << " KnobIntSlider: slider invalid range " << minValue << "~" << maxValue);
        }
        mMinValue = minValue;
        mMaxValue = maxValue;
        mCallback = callback;
        SetValue(defaultValue, true);
    }

    void Draw() override;
    void ResetToDefault() override { SetValue(mDefaultValue); }

    // FlagText format:
    // --flag_name <mMinValue~mMaxValue> : flag description
    std::string FlagText() const override;

    // Updates mValue and mDefaultValue, expected flag format:
    // --flag_name <int>
    Result UpdateFromFlag(const CliOptions& opts) override;

    int GetValue() const { return mValue; }

    // Used for when mValue needs to be updated outside of UI
    // Returns success if newVal is valid, fails otherwise
    // If newVal is valid:
    // - If updateDefault, mDefaultValue is updated
    // - If newVal is different from mValue:
    //   - mValue is updated
    //   - mCallback is triggered
    Result SetValue(int newVal, bool updateDefault = false);

private:
    int mValue;
    int mDefaultValue;

    // mValue will be clamped to the mMinValue to mMaxValue range, inclusive
    int mMinValue;
    int mMaxValue;

    // mCallback will be called on the new int value every time the slider is deselected in the UI, or updated with SetIntValue()
    std::function<void(int)> mCallback;
};

// KnobStrDropdown is an int knob that will be displayed as a dropdown in the UI
// The knob stores the index of a selected choice from a list of allowed strings
class KnobStrDropdown
    : public Knob
{
public:
    KnobStrDropdown(const std::string& displayName, const std::string& flagName, const std::string& flagDesc, int defaultIndex, const std::vector<std::string>& choices, std::function<void(int)> callback = NULL)
        : Knob(displayName, flagName, flagDesc)
    {
        if (choices.size() < 1) {
            PPX_LOG_FATAL(flagName << " KnobStrDropdown: too few allowed choices");
        }
        mChoices  = choices;
        mCallback = callback;
        auto res  = SetIndex(defaultIndex, true);
        if (res != SUCCESS)
            PPX_LOG_FATAL(flagName << "cannot be initialized");
    }

    void Draw() override;
    void ResetToDefault() override { SetIndex(mDefaultIndex); }

    // FlagText format:
    // --flag_name <"choice1"|"choice2"|...> : flag description
    std::string FlagText() const override;

    // Updates mIndex and mDefaultIndex, expected flag format:
    // --flag_name <str>
    Result UpdateFromFlag(const CliOptions& opts) override;

    int         GetIndex() const { return mIndex; }
    std::string GetStr() const { return mChoices[mIndex]; }

    // Used for when mIndex needs to be updated outside of UI
    // Returns success if newI is valid, fails otherwise
    // If newI is valid:
    // - If updateDefault, mDefaultIndex is updated
    // - If newI is different from mIndex:
    //   - mIndex is updated
    //   - mCallback is triggered
    Result SetIndex(int newI, bool updateDefault = false);

    // Used for setting from flags, will set mIndex to appropriate value if newVal matches a choice exactly
    // Updates mDefaultIndex, mIndex, and calls mCallback with the same logic as above
    Result SetIndex(std::string newVal, bool updateDefault = false);

private:
    // mIndex indicates which of the mChoices is selected
    int                      mIndex;
    int                      mDefaultIndex;
    std::vector<std::string> mChoices;

    // mCallback will be called on the new index every time the dropdown is deselected in the UI, or updated with SetIndex()
    std::function<void(int)> mCallback;
};

// KnobManager owns the knobs in an application
class KnobManager
{
public:
    KnobManager() {}
    virtual ~KnobManager();
    KnobManager(const KnobManager&)            = delete;
    KnobManager& operator=(const KnobManager&) = delete;

    // Utilities
    bool IsEmpty() { return mRoots.empty(); }
    void ResetAllToDefault();

    // Create knobs
    template <typename T, typename... ArgsT>
    T* CreateKnob(Knob* parent, ArgsT... args)
    {
        T* knobPtr = new T(std::forward<ArgsT>(args)...);

        // Reject invalid flagName
        std::string flagName = knobPtr->GetFlagName();
        for (auto& ch : flagName) {
            if (isspace(ch)) {
                PPX_LOG_ERROR("\"" << flagName << "\" cannot be created as it has whitespace");
                return nullptr;
            }
        }
        if (mFlagNames.count(flagName) > 0) {
            PPX_LOG_ERROR(flagName << " already exists and cannot be defined again");
            return nullptr;
        }

        // Store knob in members
        mFlagNames.insert(flagName);
        mKnobs.push_back(knobPtr);
        if (parent != nullptr) {
            parent->AddChild(knobPtr);
        }
        else {
            mRoots.push_back(knobPtr);
        }

        PPX_LOG_INFO("Created knob " << flagName);
        return knobPtr;
    }

    // ImGUI
    void DrawAllKnobs(bool inExistingWindow = false);

    // Command-line flags
    std::string GetUsageMsg();
    Result      UpdateFromFlags(const CliOptions& opts);

private:
    // mRoots is used for drawing with hierarchy
    std::vector<Knob*> mRoots;

    // mKnobs is used for operations that affect all knobs
    // Knobs are added on creation and never deleted until knobManager is destroyed
    std::vector<Knob*> mKnobs;

    // mFlagNames is kept to prevent multiple knobs having the same mFlagName
    std::unordered_set<std::string> mFlagNames;
};

void DrawKnobs(const std::vector<Knob*>& knobPtrs);

} // namespace ppx

#endif // ppx_knob_h
