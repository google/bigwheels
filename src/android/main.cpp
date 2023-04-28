#include <jni.h>

#include <string>
#include <vector>
#include <android_native_app_glue.h>

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

extern "C"
{
    /*!
     * This the main entry point for a native activity
     */
    void android_main(struct android_app* pApp)
    {
        std::vector<std::string> cmdArgs = get_java_args(pApp);

        std::vector<char*> args;
        args.reserve(cmdArgs.size());
        for (std::string& arg : cmdArgs) {
            args.push_back(const_cast<char*>(arg.c_str()));
        }
        RunApp(pApp, cmdArgs.size(), args.data());
    }
}
