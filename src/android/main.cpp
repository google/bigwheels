#include "ppx/log.h"
#include "ppx/profiler.h"

#include <android_native_app_glue.h>
#include <jni.h>
#include <string>
#include <vector>

extern bool RunApp(android_app* pAndroidContext, int argc, char** argv);

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

// The Android activity can go by many more states, like PAUSED.
// Right now, we just need to be able not to crash if we are stopped and resumed.
enum ApplicationState
{
    // The JNI code is loaded and running, but the activity is not started.
    READY,
    // The activity is started, the Applicaiton/Window code should handle events.
    RUNNING,
    // The activity is being destroyed. This is a transient state until we return.
    DESTROYED
};

// NOTE: JNI libraries can outlive the activity. Meaning we can re-enter the android_main
// without re-running global constructors or reseting the BSS.
static ApplicationState gApplicationState = READY;

// When the activity not running, we have no window. To simplify BigWheels code, we
// handle those states here, and restart the application once we are back live.
static void defaultCommandHandler(struct android_app* app, int32_t cmd)
{
    PPX_ASSERT_MSG(cmd == APP_CMD_START || cmd == APP_CMD_STOP || cmd == APP_CMD_DESTROY || cmd == APP_CMD_SAVE_STATE || cmd == APP_CMD_LOST_FOCUS || cmd == APP_CMD_GAINED_FOCUS, "Handled in the default-handler a message we shouldn't. This is a bug.");

    if (cmd == APP_CMD_START) {
        gApplicationState = RUNNING;
        return;
    }

    if (cmd == APP_CMD_STOP) {
        gApplicationState = READY;
        return;
    }

    if (cmd == APP_CMD_DESTROY) {
        gApplicationState = DESTROYED;
        return;
    }
}

int32_t defaultInputHandler(struct android_app*, AInputEvent*)
{
    PPX_ASSERT_MSG(false, "Handled an input message without a window. This is a bug.");
    return 0;
}

void RunAndroidEventLoop(struct android_app* pApp)
{
    pApp->onAppCmd     = defaultCommandHandler;
    pApp->onInputEvent = defaultInputHandler;
    pApp->userData     = nullptr;

    while (gApplicationState == READY) {
        int                  events;
        android_poll_source* pSource;
        if (ALooper_pollAll(/* timeoutMillis= */ 0,
                            /* outFd= */ nullptr,
                            /* outEvents= */ &events,
                            /* outData= */ (void**)&pSource) >= 0) {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }
    }
}

extern "C"
{
    /*!
     * This the main entry point for a native activity
     */
    void android_main(struct android_app* pApp)
    {
        // On Android, the library is loaded once, and it's lifetime is tied to the classloader lifetime.
        // This means the activity can be destroyed, but the library still loaded.
        // When the activity get's restarted, the library is not reloaded, meaning no global reinitialization!
        gApplicationState = RUNNING;

        std::vector<std::string> cmdArgs = get_java_args(pApp);

        std::vector<char*> args;
        args.reserve(cmdArgs.size());
        for (std::string& arg : cmdArgs) {
            args.push_back(const_cast<char*>(arg.c_str()));
        }

        while (gApplicationState != DESTROYED) {
            if (gApplicationState == RUNNING) {
                // The profiler assumed the application is run once per process lifetime. This is wrong on
                // Android, we need to cleanup some state.
                ppx::Profiler::ReinitializeGlobalVariables();
                RunApp(pApp, cmdArgs.size(), args.data());
                gApplicationState = READY;
            }
            else {
                RunAndroidEventLoop(pApp);
            }
        }
    }
}
