#include <jni.h>
#include "ffmpeg/ffmpeg.h"
#include <android/log.h>

#define TAG "execute_jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

JNIEXPORT jint JNICALL
Java_com_ffmpeg_demo_FFmpegCmd_executeCmd(
        JNIEnv *env,
        jclass this,
        jobjectArray commands) {
    LOGD(TAG);
    int len = (*env)->GetArrayLength(env, commands);
    LOGD("cmd length %d", len);
    char **argv = (char **) malloc(len * sizeof(char *));
    for (int i = 0; i < len; i++) {
        jstring str = (jstring) (*env)->GetObjectArrayElement(env, commands, i);
        char *temp = (char *) (*env)->GetStringUTFChars(env, str, 0);
        LOGD("cmd %d: %s", i, temp);
        argv[i] = (char *) malloc(1024);
        strcpy(argv[i], temp);
        (*env)->ReleaseStringUTFChars(env, str, temp);
    }

    int result = run(len, argv);
    for (int j = 0; j < len; j++) {
        free(argv[j]);
    }
    free(argv);
    return result;
}