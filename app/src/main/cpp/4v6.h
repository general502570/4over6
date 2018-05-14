#ifndef IP4V6
#define IP4V6

#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_ADDR "2402:f000:1:4417::900"
// #define SERVER_ADDR "::1"
#define SERVER_PORT 5678

#define MAX_HEART_BEAT_CNT 19

#define MSG_BUF_SIZE 4096
#define MAX_BUF_SIZE 4104
struct Message
{
    int length;
    char type;
    char data[MSG_BUF_SIZE];
};  

#define MSG_IP_REQ      100
#define MSG_IP_RSB      101
#define MSG_DATA_REQ    102
#define MSG_DATA_RSB    103
#define MSG_HEARTBEAT   104

#define JAVA_JNI_UNIX_SOCKET_PATH "/tmp/4over6.sock"
#define JAVA_JNI_PIPE_JTOC_PATH "/data/data/thunt.a4over6/4over6.jtoc"
#define JAVA_JNI_PIPE_CTOJ_PATH "/data/data/thunt.a4over6/4over6.ctoj"
#define JAVA_JNI_PIPE_BUFFER_MAX_SIZE 1024
#define JAVA_JNI_PIPE_OPCODE_SHUTDOWN 100

#define JNI 1
#if JNI
#define DBG_TAG "Backend JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, DBG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, DBG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, DBG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DBG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, DBG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {printf("Debug: "); printf(__VA_ARGS__); } while(0)
#define LOGI(...) do {printf("Info: "); printf(__VA_ARGS__); } while(0)
#define LOGW(...) do {printf("Warning: "); printf(__VA_ARGS__); } while(0)
#define LOGE(...) do {printf("Error: "); printf(__VA_ARGS__); } while(0)
#define LOGF(...) do {printf("Fatal: "); printf(__VA_ARGS__); } while(0)
#endif
#define CHK(expr)                                                      \
    do                                                                   \
    {                                                                    \
        int val;                                                         \
        if ((val = (expr)) == -1)                                         \
        {                                                                \
            LOGF("(line %d): ERROR - %d:%s.\n", __LINE__, val, strerror(errno)); \
            exit(1);                                                     \
        }                                                                \
    } while (0)

#define CHK_WRITE(expr)                                                              \
    do                                                                           \
    {                                                                            \
        int val;                                                                 \
        if ((val = (expr)) < 0)                                                \
        {                                                                        \
            LOGF("Write Error! (line %d): ERROR - %d:%s.\n", __LINE__, val, strerror(errno)); \
        }                                                                        \
    } while (0)

inline
void printMSG(struct Message* msg) {
    LOGD("MSG: len=%d, type=%d, data=%p\n", msg->length, msg->type, msg->data);
}

void connect2Server();
void initPipe();
void* initTimer(void *foo);
void* send2Server(void *foo);
void* stopListening(void *foo);
void init();
void clear();
int main();

#endif // !IP4V6