#include <jni.h>

#include <string>
#include <vector>
#include <android_native_app_glue.h>

extern bool InitVulkan(android_app* app);
extern void DeleteVulkan(void);
extern bool IsVulkanReady(void);
extern bool VulkanDrawFrame(int argc, char** argv);

/*!
 * Call the Java method constructCmdLineArgs on the MainActivity class to get
 * the command line arguments from the intent extras.
 */
static std::vector<std::string> get_java_args(android_app* pApp)
{
    JavaVM* jVm  = pApp->activity->vm;
    JNIEnv* jEnv = pApp->activity->env;
    jVm->AttachCurrentThread(&jEnv, nullptr);

    jobject      jMainActivity         = pApp->activity->clazz;
    jclass       jMainActivityClass    = jEnv->GetObjectClass(jMainActivity);
    jmethodID    jConstructCmdLineArgs = jEnv->GetMethodID(jMainActivityClass, "constructCmdLineArgs", "()[Ljava/lang/String;");
    jobjectArray jArgs                 = static_cast<jobjectArray>(jEnv->CallObjectMethod(jMainActivity, jConstructCmdLineArgs));

    std::vector<std::string> args;
    int                      argCount = jEnv->GetArrayLength(jArgs);
    for (int i = 0; i < argCount; i++) {
        jstring     jStr = static_cast<jstring>(jEnv->GetObjectArrayElement(jArgs, i));
        const char* data = jEnv->GetStringUTFChars(jStr, nullptr);
        size_t      size = jEnv->GetStringUTFLength(jStr);
        std::string str(data, size);
        jEnv->ReleaseStringUTFChars(jStr, data);
        args.push_back(str);
    }

    jVm->DetachCurrentThread();
    return args;
}

extern "C"
{
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
                std::vector<std::string> cmdArgs = get_java_args(pApp);

                std::vector<char*> args;
                args.reserve(cmdArgs.size());
                for (std::string& arg : cmdArgs) {
                    args.push_back(const_cast<char*>(arg.c_str()));
                }
                VulkanDrawFrame(cmdArgs.size(), args.data());
            }
        } while (!pApp->destroyRequested);
    }
}
