#include <jni.h>

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include <game-activity/native_app_glue/android_native_app_glue.h>

extern bool InitVulkan(android_app* app);
extern void DeleteVulkan(void);
extern bool IsVulkanReady(void);
extern bool VulkanDrawFrame(void);

extern "C"
{
#include <game-activity/native_app_glue/android_native_app_glue.c>

    /*!
     * Handles commands sent to this Android application
     * @param pApp the app the commands are coming from
     * @param cmd the command to handle
     */
    void handle_cmd(android_app* pApp, int32_t cmd)
    {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                InitVulkan(pApp);
                break;
            case APP_CMD_TERM_WINDOW:
                // The window is being destroyed. Use this to clean up your userData to avoid leaking
                // resources.
                DeleteVulkan();
                break;
            default:
                break;
        }
    }

    /*!
     * This the main entry point for a native activity
     */
    void android_main(struct android_app* pApp)
    {
        // register an event handler for Android events
        pApp->onAppCmd = handle_cmd;

        // This sets up a typical game/event loop. It will run until the app is destroyed.
        int                  events;
        android_poll_source* pSource;
        do {
            // Process all pending events before running game logic.
            if (ALooper_pollAll(0, nullptr, &events, (void**)&pSource) >= 0) {
                if (pSource) {
                    pSource->process(pApp, pSource);
                }
            }

            if (IsVulkanReady()) {
                VulkanDrawFrame();
            }
        } while (!pApp->destroyRequested);
    }
}
