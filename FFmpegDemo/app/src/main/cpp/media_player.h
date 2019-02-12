//
// Created by Simple on 2019/2/9.
//

#ifndef FFMPEGDEMO_MEDIA_PLAYER_H
#define FFMPEGDEMO_MEDIA_PLAYER_H

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <stdio.h>
#include <unistd.h>
#include <libavutil/imgutils.h>
#include <android/log.h>
#include <pthread.h>
#include <libavutil/time.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"

#include "AVpacket_queue.h"

#define TAG "MediaPlayer"
#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG, TAG, FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, TAG, FORMAT, ##__VA_ARGS__);

#define MAX_AUDIO_FRAME_SIZE 48000 * 4
#define MIN_SLEEP_TIME_US 1000ll
#define PACKET_SIZE 50
#define AUDIO_TIME_ADJUST_US -200000ll

typedef struct MediaPlayer {
    AVFormatContext *format_context;
    int video_stream_index;
    int audio_stream_index;
    AVCodecContext *video_codec_context;
    AVCodecContext *audio_codec_context;
    AVCodec *video_codec;
    AVCodec *audio_codec;
    ANativeWindow *native_window;
    uint8_t *video_buffer;
    AVFrame *src_frame;
    AVFrame *rgba_frame;
    int video_width;
    int video_height;
    SwrContext *audio_swr_context;
    int out_channel_nb;
    int out_sample_rate;
    enum AVSampleFormat out_sample_fmt;
    jobject audio_track;
    jmethodID audio_track_write_mid;
    uint8_t *audio_buffer;
    AVFrame *audio_frame;
    AVPacketQueue *packets[2];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int64_t start_time;
    int64_t audio_clock;
    pthread_t write_thread;
    pthread_t video_thread;
    pthread_t audio_thread;
} MediaPlayer;

typedef struct Decoder {
    MediaPlayer *player;
    int stream_index;
} Decoder;

MediaPlayer *player;
JavaVM *javaVM;

/**
 * 解封视频
 * @param player
 * @param path
 * @return code （code < 0 error）
 */
int init_input_format_context(MediaPlayer *player, const char *path) {
    // 申请AVFormatContext 解封视频
    player->format_context = avformat_alloc_context();
    if (avformat_open_input(&player->format_context, path, NULL, NULL) != 0) {
        LOGE("Couldn't open file:%s\n", path);
        return -1;
    }
    // 读取媒体流信息
    if (avformat_find_stream_info(player->format_context, NULL) < 0) {
        LOGE("Couldn't find stream information.");
        return -1;
    }
    // 定位音频、视频索引
    player->video_stream_index = -1;
    player->audio_stream_index = -1;
    for (int i = 0; i < player->format_context->nb_streams; i++) {
        switch (player->format_context->streams[i]->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                player->video_stream_index = i;
                break;
            case AVMEDIA_TYPE_AUDIO:
                player->audio_stream_index = i;
                break;
            default:
                LOGE("unknown stream type!");
                break;
        }
    }
    if (player->video_stream_index == -1) {
        LOGE("couldn't find a video stream.");
        return -1;
    }
    if (player->audio_stream_index == -1) {
        LOGE("couldn't find a audio stream.");
        return -1;
    }
    LOGD("video_stream_index=%d", player->video_stream_index);
    LOGD("audio_stream_index=%d", player->audio_stream_index);
    return 0;
}

/**
 * 音视频解码器
 * @param player
 * @return code （code < 0 error）
 */
int init_codec_context(MediaPlayer *player) {
    // 视频解码器
    AVCodecContext *codecVideoCtx = avcodec_alloc_context3(NULL);
    if (codecVideoCtx == NULL) {
        LOGE("couldn't allocate AVCodecContext.");
        return -1;
    }
    avcodec_parameters_to_context(codecVideoCtx, player->format_context->streams[player->video_stream_index]->codecpar);
    player->video_codec_context = codecVideoCtx;
    player->video_codec = avcodec_find_decoder(player->video_codec_context->codec_id);
    if (player->video_codec == NULL) {
        LOGE("couldn't find video Codec.");
        return -1;
    }
    if (avcodec_open2(player->video_codec_context, player->video_codec, NULL) < 0) {
        LOGE("Couldn't open video codec.");
        return -1;
    }
    // 音频解码器
    AVCodecContext *codecAudioCtx = avcodec_alloc_context3(NULL);
    if (codecAudioCtx == NULL) {
        LOGE("couldn't allocate AVCodecContext.");
        return -1;
    }
    avcodec_parameters_to_context(codecAudioCtx, player->format_context->streams[player->audio_stream_index]->codecpar);
    player->audio_codec_context = codecAudioCtx;
    player->audio_codec = avcodec_find_decoder(player->audio_codec_context->codec_id);
    if (player->audio_codec == NULL) {
        LOGE("couldn't find audio Codec.");
        return -1;
    }
    if (avcodec_open2(player->audio_codec_context, player->audio_codec, NULL) < 0) {
        LOGE("Couldn't open audio codec.");
        return -1;
    }
    //视频的宽高
    player->video_width = player->video_codec_context->width;
    player->video_height = player->video_codec_context->height;
    LOGD("video height: %d, width: %d", player->video_height, player->video_width);
    return 0;
}

/**
 * 初始化native window
 * @param player
 * @param env
 * @param surface
 */
void video_player_window_prepare(MediaPlayer *player, JNIEnv *env, jobject surface) {
    player->native_window = ANativeWindow_fromSurface(env, surface);
}

/**
 * 配置音频重采样
 * @param player
 */
void audio_decoder_prepare(MediaPlayer *player) {
    // 准备音频重采样api
    player->audio_swr_context = swr_alloc();
    // 输入采样格式
    enum AVSampleFormat in_sample_format = player->audio_codec_context->sample_fmt;
    // 定义输出采样格式
    player->out_sample_fmt = AV_SAMPLE_FMT_S16;
    // 输入采样率
    int in_sample_rate = player->audio_codec_context->sample_rate;
    // 输入声道布局
    uint64_t in_ch_layout = player->audio_codec_context->channel_layout;
    // 设置输出采样率
    player->out_sample_rate = in_sample_rate;
    // 设置输出声道布局为立体声
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    // 设置输出声道数
    player->out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    // 配置音频重采样参数
    swr_alloc_set_opts(player->audio_swr_context,
                       out_ch_layout,
                       player->out_sample_fmt,
                       player->out_sample_rate,
                       in_ch_layout,
                       in_sample_format,
                       in_sample_rate,
                       0,
                       NULL);
    int result = swr_init(player->audio_swr_context);
    LOGD("swr_init result = %d", result);
}

/**
 * 音频播放准备
 * @param player
 * @param env
 * @param this
 * @return  code （code < 0 error）
 */
int audio_player_prepare(MediaPlayer *player, JNIEnv *env, jclass this) {
    jclass this_class = (*env)->GetObjectClass(env, this);\
    if (!this_class) {
        LOGE("MediaPlayer class not found error!")
        return -1;
    }
    // 调用createAudioTrack方法
    jmethodID create_audio_track = (*env)->GetMethodID(env, this_class, "createAudioTrack",
                                                       "(II)Landroid/media/AudioTrack;");
    if (!create_audio_track) {
        LOGE("can not find createAudioTrack method!")
        return -1;
    }
    jobject audio_track = (*env)->CallObjectMethod(env,
                                                   this,
                                                   create_audio_track,
                                                   player->out_sample_rate,
                                                   player->out_channel_nb);
    // 调用 AudioTrack play()方法
    jclass audio_track_class = (*env)->GetObjectClass(env, audio_track);
    jmethodID audio_track_play = (*env)->GetMethodID(env, audio_track_class, "play", "()V");
    (*env)->CallVoidMethod(env, audio_track, audio_track_play);
    // 记录 AudioTrack 引用
    player->audio_track = (*env)->NewGlobalRef(env, audio_track);
    // 记录 AudioTrack write方法
    player->audio_track_write_mid = (*env)->GetMethodID(env, audio_track_class, "write", "([BII)I");
    // 准备 Buffer
    player->audio_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRAME_SIZE);
    // 申请帧的数据
    player->audio_frame = av_frame_alloc();
    return 0;
}

/**
 * 准备两个队列（音频、视频）
 * @param player
 * @param size
 */
void init_queue(MediaPlayer *player) {
    for (int i = 0; i < 2; i++) {
        AVPacketQueue *queue = queue_init(PACKET_SIZE);
        player->packets[i] = queue;
    }
}

void delete_queue(MediaPlayer *player) {
    int i;
    for (i = 0; i < 2; ++i) {
        queue_free(player->packets[i]);
    }
}

/**
 * 生产者
 * @param arg
 * @return
 */
void *write_packet_to_queue(void *arg) {
    MediaPlayer *player = (MediaPlayer *) arg;
    AVPacket packet;
    int ret;
    while (1) {
        // 读取视频中的一帧或音频中的若干帧
        ret = av_read_frame(player->format_context, &packet);
        if (ret < 0) {
            break;
        }
        LOGD("pop packet stream_index = %d, size = %d, pos = %lld", packet.stream_index, packet.size, packet.pos);
        if (packet.stream_index == player->video_stream_index
            || packet.stream_index == player->audio_stream_index) {
            AVPacketQueue *queue = player->packets[packet.stream_index];
            // 写入到队列中
            pthread_mutex_lock(&player->mutex);
            AVPacket *data = queue_push(queue, &player->mutex, &player->cond);
            pthread_mutex_unlock(&player->mutex);
            //拷贝（间接赋值，拷贝结构体数据）
            *data = packet;
        }
    }
}

int64_t get_play_time(MediaPlayer *player) {
    return (int64_t) (av_gettime() - player->start_time);
}

void player_sleep_for_frame(MediaPlayer *player, int64_t stream_time) {
    pthread_mutex_lock(&player->mutex);
    for (;;) {
        int64_t current_video_time = get_play_time(player);
        int64_t sleep_time = stream_time - current_video_time;
        if (sleep_time < -300000ll) {
            // 300 ms late
            int64_t new_value = player->start_time - sleep_time;
            player->start_time = new_value;
            pthread_cond_broadcast(&player->cond);
        }

        if (sleep_time <= MIN_SLEEP_TIME_US) {
            // We do not need to wait if time is slower then minimal sleep time
            break;
        }

        if (sleep_time > 500000ll) {
            // if sleep time is bigger then 500ms just sleep this 500ms
            // and check everything again
            sleep_time = 500000ll;
        }
        //等待指定时长
        pthread_cond_timeout_np(&player->cond, &player->mutex,
                                (unsigned int) (sleep_time / 1000ll));
    }
    pthread_mutex_unlock(&player->mutex);
}

int decode_video(MediaPlayer *player, AVPacket *packet) {
    int ret = 0;
    // 设置buffer 的大小和格式
    ANativeWindow_setBuffersGeometry(
            player->native_window,
            player->video_width,
            player->video_height,
            WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;

    // 申请帧
    player->src_frame = av_frame_alloc();
    player->rgba_frame = av_frame_alloc();
    if (player->src_frame == NULL || player->rgba_frame == NULL) {
        LOGE("Couldn't allocate video frame.");
        return -1;
    }

    // 申请缓存内存
    int bufferBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, player->video_width, player->video_height, 1);
    player->video_buffer = (uint8_t *) av_malloc(bufferBytes * sizeof(uint8_t));
    // 将申请的内存指针与帧关联
    av_image_fill_arrays(
            player->rgba_frame->data,
            player->rgba_frame->linesize,
            player->video_buffer,
            AV_PIX_FMT_RGBA,
            player->video_width,
            player->video_height,
            1);
    // 设置转码格式 将yuv格式转码成rgb
    struct SwsContext *sws_context = sws_getContext(
            player->video_width,
            player->video_height,
            player->video_codec_context->pix_fmt,
            player->video_width,
            player->video_height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL);
    // 对该帧进行解码
    ret = avcodec_send_packet(player->video_codec_context, packet);
    if (ret != 0) {
        LOGE("avcodec_send_packet error...");
        return -1;
    }
    while (avcodec_receive_frame(player->video_codec_context, player->src_frame) == 0) {
        // 对解码后的数据进行转码
        ANativeWindow_lock(player->native_window, &window_buffer, 0);
        int result = sws_scale(sws_context,
                               (uint8_t const *const *) player->src_frame->data,
                               player->src_frame->linesize,
                               0,
                               player->video_height,
                               player->rgba_frame->data,
                               player->rgba_frame->linesize);
        LOGD("sws_scale result = %d...", result);
        uint8_t *dist = window_buffer.bits;
        int dist_stride = window_buffer.stride * 4;
        uint8_t *src = player->rgba_frame->data[0];
        int src_stride = player->rgba_frame->linesize[0];
        // 由于window的stride和帧的stride不同,因此需要逐行复制
        for (int h = 0; h < player->video_height; h++) {
            memcpy(dist + h * dist_stride, src + h * src_stride, (size_t) src_stride);
        }
        // 视频同步
        // 获取该帧的pts时间
        int64_t pts = player->src_frame->pts;
        AVStream *video_stream = player->format_context->streams[player->video_stream_index];
        int64_t pts_time = av_rescale_q(pts, video_stream->time_base, AV_TIME_BASE_Q);
        // 同步视频
        player_sleep_for_frame(player, pts_time);
        // 提交buffer
        int post_result = ANativeWindow_unlockAndPost(player->native_window);
        LOGD("ANativeWindow_unlockAndPost result = %d...", post_result);
    }

    return ret;
}

int decode_audio(MediaPlayer *player, AVPacket *packet) {
    int got_frame = 0, ret;
    //解码
    ret = avcodec_decode_audio4(player->audio_codec_context, player->audio_frame, &got_frame, packet);
    if (ret < 0) {
        LOGE("avcodec_decode_audio4 error...");
        return -1;
    }
    //解码一帧成功
    if (got_frame > 0) {
        //音频格式转换
        swr_convert(player->audio_swr_context, &player->audio_buffer, MAX_AUDIO_FRAME_SIZE,
                    (const uint8_t **) player->audio_frame->data, player->audio_frame->nb_samples);
        int out_buffer_size = av_samples_get_buffer_size(NULL, player->out_channel_nb,
                                                         player->audio_frame->nb_samples, player->out_sample_fmt, 1);

        //音视频帧同步
        int64_t pts = packet->pts;
        if (pts != AV_NOPTS_VALUE) {
            AVStream *stream = player->format_context->streams[player->audio_stream_index];
            player->audio_clock = av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
            player_sleep_for_frame(player, player->audio_clock + AUDIO_TIME_ADJUST_US);
        }

        if (javaVM != NULL) {
            JNIEnv *env;
            (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
            jbyteArray audio_sample_array = (*env)->NewByteArray(env, out_buffer_size);
            jbyte *sample_byte_array = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);
            //拷贝缓冲数据
            memcpy(sample_byte_array, player->audio_buffer, (size_t) out_buffer_size);
            //释放数组
            (*env)->ReleaseByteArrayElements(env, audio_sample_array, sample_byte_array, 0);
            //调用AudioTrack的write方法进行播放
            (*env)->CallIntMethod(env, player->audio_track, player->audio_track_write_mid,
                                  audio_sample_array, 0, out_buffer_size);
            //释放局部引用
            (*env)->DeleteLocalRef(env, audio_sample_array);
        }
    }
    if (javaVM != NULL) {
        (*javaVM)->DetachCurrentThread(javaVM);
    }
    return 0;
}

void *decode_fun(void *arg) {
    Decoder *decoder = (Decoder *) arg;
    MediaPlayer *player = decoder->player;
    int stream_index = decoder->stream_index;

    // 找到视频或音频队列
    AVPacketQueue *queue = player->packets[stream_index];
    int ret = 0, video_frame_count = 0, audio_frame_count = 0;

    while (1) {
        // 从队列中取出一个压缩数据
        pthread_mutex_lock(&player->mutex);
        AVPacket *packet = (AVPacket *) queue_pop(queue, &player->mutex, &player->cond);
        pthread_mutex_unlock(&player->mutex);
        LOGD("pop packet stream_index = %d, size = %d, pos = %lld", packet->stream_index, packet->size, packet->pos);

        if (stream_index == player->video_stream_index) {
            // 视频流
            ret = decode_video(player, packet);
            LOGD("decode video stream = %d", video_frame_count++);
        } else if (stream_index == player->audio_stream_index) {//音频流
            ret = decode_audio(player, packet);
            LOGD("decode audio stream = %d", audio_frame_count++);
        }

        av_packet_unref(packet);

        if (ret < 0) {
            LOGE("decode stream decode_fun ret = %d", ret);
            break;
        }
    }
}

#endif //FFMPEGDEMO_MEDIA_PLAYER_H
