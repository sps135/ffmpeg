//
// Created by Simple on 2019/2/8.
//
#include <jni.h>
#include "media_player.h"

jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_6;
}

/**
 * setup 初始化
 * @param env
 * @param jthis
 * @param filePath
 * @param surface
 * @return (code < 0 error)
 */
JNIEXPORT jint JNICALL Java_com_ffmpeg_demo_MediaPlayer_setup
        (JNIEnv *env, jclass jthis, jstring filePath, jobject surface) {
    int ret = 0;
    const char *path_name = (*env)->GetStringUTFChars(env, filePath, JNI_FALSE);

    player = malloc(sizeof(MediaPlayer));
    if (player == NULL) {
        return -1;
    }

    ret = init_input_format_context(player, path_name);
    if (ret < 0) {
        return ret;
    }

    ret = init_codec_context(player);
    if (ret < 0) {
        return ret;
    }

    video_player_window_prepare(player, env, surface);

    audio_decoder_prepare(player);

    ret = audio_player_prepare(player, env, jthis);
    if (ret < 0) {
        return ret;
    }

    init_queue(player);

    return ret;
}

/**
 * 播放
 * @param env
 * @param jthis
 * @return
 */
JNIEXPORT jint JNICALL Java_com_ffmpeg_demo_MediaPlayer_play
        (JNIEnv *env, jclass jthis) {
    int ret = 0;

    // 初始化锁
    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);

    // 生产者线程
    pthread_create(&player->write_thread, NULL, write_packet_to_queue, (void *) player);
    sleep(1);
    player->start_time = 0;

    // 视频、音频消费者线程
    Decoder data1 = {player, player->video_stream_index}, *decoder_data1 = &data1;
    pthread_create(&player->video_thread, NULL, decode_fun, (void *) decoder_data1);

    Decoder data2 = {player, player->audio_stream_index}, *decoder_data2 = &data2;
    pthread_create(&player->audio_thread, NULL, decode_fun, (void *) decoder_data2);

    pthread_join(player->write_thread, NULL);
    pthread_join(player->video_thread, NULL);
    pthread_join(player->audio_thread, NULL);

    return ret;
}

JNIEXPORT void JNICALL Java_com_ffmpeg_demo_MediaPlayer_release
        (JNIEnv *env, jclass jthis) {
    pthread_detach(player->write_thread);
    pthread_detach(player->video_thread);
    pthread_detach(player->audio_thread);

    pthread_cond_destroy(&player->cond);
    pthread_mutex_destroy(&player->mutex);

    av_free(player->video_buffer);
    av_free(player->rgba_frame);
    av_free(player->src_frame);
    av_free(player->audio_buffer);
    av_free(player->audio_frame);

    avformat_close_input(&player->format_context);

    avcodec_close(player->video_codec_context);
    avcodec_close(player->audio_codec_context);

    ANativeWindow_release(player->native_window);

    delete_queue(player);

    free(player);
}