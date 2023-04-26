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

#include "ppx/window.h"
#include "ppx/application.h"

namespace ppx {

// -------------------------------------------------------------------------------------------------
// Headless Window
// -------------------------------------------------------------------------------------------------

class WindowImplHeadless : public Window
{
public:
    using Window::Window;

    Result Create(const char* title) final
    {
        PPX_LOG_INFO("Headless mode: skipping initialization of glfw");
        return Window::Create(title);
    }
};

std::unique_ptr<Window> Window::GetImplHeadless(Application* pApp)
{
    return std::unique_ptr<Window>(new WindowImplHeadless(pApp));
}

// -------------------------------------------------------------------------------------------------
// Window
// -------------------------------------------------------------------------------------------------

Window::Window(Application* pApp)
    : mApp(pApp)
{
}

WindowSize Window::Size() const
{
    return {mApp->GetSettings()->window.width, mApp->GetSettings()->window.height};
}

} // namespace ppx
