#include <jni.h>
#include <string>
#include <string.h>
#include "4v6.h"

extern "C"
JNIEXPORT jstring JNICALL
Java_thunt_a4over6_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_thunt_a4over6_MainActivity_stringclickedFromJNI(
        JNIEnv *env, jobject /* this */, jint x) {
    
    LOGD("Backend 4over6 thread Starts!");
    main();
    LOGD("Backend 4over6 thread Ends!");
    
    std::string hello = "Backend start!";
    char c[2];
    c[0] = x + 'a';
    c[1] = '\0';
    std::string hello2 = c;
    hello += hello2;
    return env->NewStringUTF(hello.c_str());
}
