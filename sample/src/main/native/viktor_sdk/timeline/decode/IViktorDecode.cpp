//
// Created by rainmeterLotus on 2022/1/4.
//

#include "IViktorDecode.h"
int IViktorDecode::decoder_decode_frame(ViktorDecoder *d, AVFrame *frame, CClip *clip){
    VIKTOR_LOGD("decoder_decode_frame");
    VIKTOR_LOGD("decoder_decode_frame d:%p",d);
    VIKTOR_LOGD("decoder_decode_frame d->avctx:%p",d->avctx);

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

        VIKTOR_LOGD("decoder_decode_frame d->queue->serial:%d,--d->pkt_serial:%d",d->queue->serial,d->pkt_serial);
        if (d->queue->serial == d->pkt_serial) {
            do {
                if (d->queue->abort_request) {
                    return -1;
                }

                switch (d->avctx->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        VIKTOR_LOGD("avcodec_receive_frame video ret:%d,msg:%s", ret,av_err2str(ret));
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

                long frame_pts = frame->pts * av_q2d(d->avctx->pkt_timebase)* 1000000;
                VIKTOR_LOGD("decoder_decode_frame gogo frame_pts:%ld,d->decode_state:%d",frame_pts,d->decode_state);
                if (frame_pts > clip->m_end_micro_sec && d->decode_state > 0) {
                    VIKTOR_LOGD("decoder_decode_frame bingo wait_decode_cond->notify_one");
                    d->decode_state = 0;
                    d->wait_decode_cond->notify_one();
                    if (clip->isLast){
                        VIKTOR_LOGD("decoder_decode_frame clip->isLast");
                        d->finished = d->pkt_serial;
//                        avcodec_flush_buffers(d->avctx);
                        return 0;
                    }
                }


                if (ret == AVERROR_EOF) {
                    VIKTOR_LOGD("decoder_decode_frame AVERROR_EOF");
                    d->finished = d->pkt_serial;
                    //再次解码之前，必须使用avcodec_flush_buffers重新编码
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }

                if (ret >= 0) {
                    return 1;
                }
            } while (ret != AVERROR(EAGAIN));
        }

        do{
            if (d->queue->nb_packets == 0) {
                /**
                 * 通知唤醒read_thread方法中获取Packet时的is->read_frame_cond 等待
                 */
                VIKTOR_LOGD("decoder_decode_frame do while 11");
                d->empty_queue_cond->notify_one();
            }

            if (d->packet_pending) {//如果有待重发的pkt，则先取待重发的pkt
                VIKTOR_LOGD("decoder_decode_frame do while 22");
                av_packet_move_ref(&pkt, &d->pkt);
                d->packet_pending = 0;
            } else {
                VIKTOR_LOGD("decoder_decode_frame do while 33");
                if (packet_queue_get(d->queue, &pkt,&d->pkt, 1, &d->pkt_serial,d->decode_state) < 0) return -1;
            }

            if (d->queue->serial == d->pkt_serial) {
                break;
            }
            VIKTOR_LOGD("decoder_decode_frame do while 44");
            av_packet_unref(&pkt);
        }while (1);

        long pkt_pts = pkt.pts * av_q2d(d->avctx->pkt_timebase)* 1000000;
        VIKTOR_LOGD("decoder_decode_frame gogo pkt_pts:%ld",pkt_pts);
        /**
         * 往PacketQueue送入一个flush_pkt后，PacketQueue的serial值会加1，
         * 而送入的flush_pkt和PacketQueue的serial值保持一致。
         * 所以如果有“过时”Packet，过滤后，取到的第一个pkt将是flush_pkt。
         * 所以这里就是取出过时的packet
         */
        if (pkt.data == flush_pkt.data) {
            VIKTOR_LOGD("decoder_decode_frame gogo is flush_pkt");
            avcodec_flush_buffers(d->avctx);
            d->finished = 0;
            d->next_pts = d->start_pts;
            d->next_pts_tb = d->start_pts_tb;
        } else {
            VIKTOR_LOGD("decoder_decode_frame avcodec_send_packet 00");
            if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
                VIKTOR_LOGD("Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                /**
                 * 在方法的上面"取一个packet"中重新发送pkt
                 */
                d->packet_pending = 1;

            }
            VIKTOR_LOGD("decoder_decode_frame avcodec_send_packet 11");
            if (d->decode_state > 0){
                av_packet_move_ref(&d->pkt, &pkt);
            }

            av_packet_unref(&pkt);
            long pkt_pts_d = d->pkt.pts * av_q2d(d->avctx->pkt_timebase)* 1000000;
            VIKTOR_LOGD("decoder_decode_frame avcodec_send_packet pkt_pts_d:%ld,pkt:%p",pkt_pts_d,pkt);
        }
    }
}

void IViktorDecode::release(){

}