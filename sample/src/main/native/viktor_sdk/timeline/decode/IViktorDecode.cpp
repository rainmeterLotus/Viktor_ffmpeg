//
// Created by rainmeterLotus on 2022/1/4.
//

#include "IViktorDecode.h"

int IViktorDecode::decoder_decode_frame(ViktorDecoder *d, AVFrame *frame, CClip *clip) {
    VIKTOR_LOGD("decoder_decode_frame start");
    VIKTOR_LOGD("decoder_decode_frame d:%p", d);
    VIKTOR_LOGD("decoder_decode_frame d->avctx:%p", d->avctx);

    int ret = AVERROR(EAGAIN);
    for (;;) {
        AVPacket pkt;
        /**
        流连续的情况下，不断调用 avcodec_receive_frame 获取解码后的frame
        d->queue就是video PacketQueue(videoq)，
        d->pkt_serial是最近一次取的Packet的序列号。
        在判断完d->queue->serial == d->pkt_serial确保流连续后，
        循环调用avcodec_receive_frame，有取到帧，就返回。
        （即使还没送入新的Packet，这是为了兼容一个Packet可以解出多个Frame的情况）
        */

        VIKTOR_LOGD("decoder_decode_frame d->queue->serial:%d,--d->pkt_serial:%d", d->queue->serial, d->pkt_serial);
        if (d->queue->serial == d->pkt_serial) {
            do {
                if (d->queue->abort_request) {
                    return -1;
                }

                switch (d->avctx->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        VIKTOR_LOGD("avcodec_receive_frame get result,video ret:%d,msg:%s", ret, av_err2str(ret));
                        if (ret >= 0) {
                            /**
                             * 获取解码后的frame成功，判断是否需要重新排序frame的pts
                             */
                            if (decoder_reorder_pts == -1) {
                                frame->pts = frame->best_effort_timestamp;
                            } else if (!decoder_reorder_pts) {
                                frame->pts = frame->pkt_dts;
                            }


                        }
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            AVRational tb = (AVRational) {1, frame->sample_rate};
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
                            else if (d->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE) {
                                d->next_pts = frame->pts + frame->nb_samples;
                                d->next_pts_tb = tb;
                            }
                        }
                        break;
                }

                /**
                 * 目前下面逻辑针对视频轨道，音频后期考虑
                 * 下面逻辑是为了处理当前片段是否已经解码结束，所谓的结束有2点：
                 * 1.当前帧的pts > 当前片段允许的时间（m_end_micro_sec）
                 * 2.当前clip已经解码到最后了（ret == AVERROR_EOF）
                 * 满足以上2点一个之后，就需要通知下一个需要解码的片段，不要等待了，你可以解码了（见ViktorVideoDecode::decode_start）
                 */
                long frame_pts = frame->pts * av_q2d(d->avctx->pkt_timebase) * 1000000;
                VIKTOR_LOGD("decoder_decode_frame gogo frame_pts:%ld,d->decode_state:%d", frame_pts, d->decode_state);
                VIKTOR_LOGD("decoder_decode_frame gogo d->queue->serial:%d,--d->pkt_serial:%d", d->queue->serial, d->pkt_serial);
                if ((frame_pts > clip->m_end_micro_sec) && d->decode_state > 0) {
                    VIKTOR_LOGD("decoder_decode_frame bingo wait_decode_cond->notify_one");
                    //通知等待解码的地方，可以解码了（见ViktorVideoDecode::decode_start）
                    d->decode_state = 0;
                    d->wait_decode_cond->notify_one();
                    if (clip->isLast) {
                        VIKTOR_LOGD("decoder_decode_frame clip->isLast path：%s", clip->m_file_path.c_str());
                        /**
                         * 这里需要将  d->pkt_serial 赋给d->finished，需要在Demux中判断是否需要循环播放（见ViktorDemux::is_loop）
                         */
                        d->finished = d->pkt_serial;
                        //再次解码之前，必须使用avcodec_flush_buffers重新编码
                        avcodec_flush_buffers(d->avctx);
                        return 0;
                    }
                }

                if (ret == AVERROR_EOF) {
                    //通知等待解码的地方，可以解码了（见ViktorVideoDecode::decode_start）
                    d->decode_state = 0;
                    d->wait_decode_cond->notify_one();
                    VIKTOR_LOGD("decoder_decode_frame AVERROR_EOF");
                    /**
                     * 这里需要将  d->pkt_serial 赋给d->finished，需要在Demux中判断是否需要循环播放（见ViktorDemux::is_loop）
                     */
                    d->finished = d->pkt_serial;
                    //再次解码之前，必须使用avcodec_flush_buffers重新编码
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }

                //最终解码成功
                if (ret >= 0) {
                    VIKTOR_LOGD("decoder_decode_frame wow! we got a frame!!!");
                    return 1;
                }

                /**
                 * AVERROR(EAGAIN)
                 * avcodec_receive_frame 如果返回该值代表：
                 * output is not available in this state - user must try to send new input，
                 * 大致意思就是这个状态下表明输入端还么有数据，就是avcodec_send_packet还没有输入数据
                 *
                 * 只要不等于AVERROR(EAGAIN)就一直在do while中去调用avcodec_receive_frame
                 */
            } while (ret != AVERROR(EAGAIN));
        }

        do {
            if (d->queue->nb_packets == 0) {
                /**
                 * 通知唤醒read_thread方法中获取Packet时的is->read_frame_cond 等待
                 */
                VIKTOR_LOGD("decoder_decode_frame no AVPacket，notify Demux read data and input queue");
                d->empty_queue_cond->notify_one();
            }

            if (d->packet_pending) {//如果有待重发的pkt，则先取待重发的pkt
                VIKTOR_LOGD("decoder_decode_frame again obtain packet_pending AVPacket");
                av_packet_move_ref(&pkt, &d->pkt);
                d->packet_pending = 0;
            } else {
                VIKTOR_LOGD("decoder_decode_frame ok,to go queue obtain AVPacket");
                if (packet_queue_get(d->queue, &pkt, &d->pkt, 1, &d->pkt_serial, d->decode_state) < 0) return -1;
            }

            if (d->queue->serial == d->pkt_serial) {
                VIKTOR_LOGD("decoder_decode_frame bingo! got an AVPacket,break!");
                break;
            }
            VIKTOR_LOGD("decoder_decode_frame something happen");
            av_packet_unref(&pkt);
        } while (1);

        long pkt_pts = pkt.pts * av_q2d(d->avctx->pkt_timebase) * 1000000;
        VIKTOR_LOGD("decoder_decode_frame will use this AVPacket to decode, pkt_pts:%ld", pkt_pts);
        /**
         * 往PacketQueue送入一个flush_pkt后，PacketQueue的serial值会加1，
         * 而送入的flush_pkt和PacketQueue的serial值保持一致。
         * 所以如果有“过时”Packet，过滤后，取到的第一个pkt将是flush_pkt。
         * 所以这里就是取出过时的packet
         */
        if (pkt.data == flush_pkt.data) {
            VIKTOR_LOGD("decoder_decode_frame will use this AVPacket to decode,but this is flush_pkt");
            avcodec_flush_buffers(d->avctx);
            d->finished = 0;
            d->next_pts = d->start_pts;
            d->next_pts_tb = d->start_pts_tb;
        } else {
            VIKTOR_LOGD("decoder_decode_frame avcodec_send_packet,finally to decode");
            if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
                /**
                 * input is not accepted in the current state - user
                 * must read output with avcodec_receive_frame() (once
                 * all output is read, the packet should be resent, and
                 * the call will not fail with EAGAIN)
                 * 说明输出端应该及时调用avcodec_receive_frame读取数据，然后再重新发送一次该AVPacket
                 */
                VIKTOR_LOGD("Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                /**
                 * 在方法的上面"取一个packet"中重新发送pkt
                 */
                d->packet_pending = 1;

            }
            VIKTOR_LOGD("decoder_decode_frame avcodec_send_packet end");
            if (d->decode_state > 0) {
                av_packet_move_ref(&d->pkt, &pkt);
            }

            av_packet_unref(&pkt);
            long pkt_pts_d = d->pkt.pts * av_q2d(d->avctx->pkt_timebase) * 1000000;
            VIKTOR_LOGD("decoder_decode_frame avcodec_send_packet pkt_pts_d:%ld,pkt:%p", pkt_pts_d, pkt);
        }
    }
}

void IViktorDecode::release() {

}