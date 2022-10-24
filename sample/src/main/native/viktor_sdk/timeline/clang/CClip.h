//
// Created by rainmeterLotus on 2021/12/18.
//

#ifndef VIKTOR_FFMPEG_CCLIP_H
#define VIKTOR_FFMPEG_CCLIP_H

#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

class CClip {
public:
    CClip(std::string filePath,long startTime,long endTime):m_file_path(filePath),m_start_micro_sec(startTime),m_end_micro_sec(endTime){

    }

    ~CClip() {}

    void release();

    int openFile();
    /**
     *
     * @param isVideoStreamMain 是否视频轨道为主，
     * 该方法会同时打开音频和视频轨道，以哪个轨道成功返回值为主
     * @return
     */
    int openDecode(bool isVideoStreamMain);

    std::string m_file_path;
    long m_start_micro_sec;
    long m_end_micro_sec;
    long m_in_point_us;
    long m_out_point_us;

    long m_seek_to_us;

    long m_duration_us;
    int m_video_index = -1;
    int m_audio_index = -1;
    int m_orientation = 0;
    int m_width = 0;
    int m_height = 0;

    int audio_enable = 0;
    int video_enable = 1;

    AVFormatContext *in_fmt_ctx = nullptr;
    AVCodecContext *video_in_codec_ctx = nullptr;
    AVCodecContext *audio_in_codec_ctx = nullptr;
    bool isFirst = false;
    bool isLast = false;

private:
    int init_decode(int stream_index,AVCodecContext **avctx_p);
};


#endif //VIKTOR_FFMPEG_CCLIP_H
