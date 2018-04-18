#include <jni.h>
#include <string>
#include <string.h>

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
    std::string hello = "Button clicked!";
    char c[2];
    c[0] = x + 'a';
    c[1] = '\0';
    std::string hello2 = c;
    hello += hello2;
    return env->NewStringUTF(hello.c_str());
}
