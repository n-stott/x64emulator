extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <cassert>

int main(int argc, char* argv[]) {
    if(argc != 2) {
        puts("Usage: exe path-to-mp4");
        return 1;
    }
    AVFormatContext* avcontext = avformat_alloc_context();
    if(avcontext == nullptr) {
        puts("alloc_context failed");
        return 1;
    }
    if(avformat_open_input(&avcontext, argv[1], NULL, NULL) < 0) {
        puts("open_input failed");
        return 1;
    }
    if(avformat_find_stream_info(avcontext, NULL) < 0) {
        puts("find_stream_info failed");
        return 1;
    }

    AVPacket pkt;

    memset(&pkt, 0, sizeof(AVPacket));
    if(av_read_frame(avcontext, &pkt) < 0) {
        puts("read_frame failed");
        return 1;
    }

    int video_stream_id = -1;
    for(size_t i=0; i<avcontext->nb_streams; i++) {
        if(avcontext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_id = i;
            break;
        }
    }
    if(video_stream_id < 0) {
        puts("find video code type failed");
        return 1;
    }

    const AVCodec* codec = avcodec_find_decoder(avcontext->streams[video_stream_id]->codecpar->codec_id);
    if(!codec) {
        puts("find_decoder failed");
        return 1;
    }

    AVCodecContext* cocontext = avcodec_alloc_context3(codec);
    if(!cocontext) {
        puts("alloc_context3 failed");
        return 1;
    }

    if(avcodec_parameters_to_context(cocontext, avcontext->streams[video_stream_id]->codecpar) < 0) {
        puts("parameters to context failed");
        return 1;
    }

    if(avcodec_open2(cocontext, codec, NULL) < 0) {
        puts("open2 failed");
        return 1;
    }

    if(avcodec_send_packet(cocontext, &pkt) < 0) {
        puts("send_packet failed");
        return 1;
    }

    AVFrame* frame = av_frame_alloc();

    assert(frame->data[0] == nullptr);

    if(avcodec_receive_frame(cocontext, frame) < 0) {
        puts("receive_frame failed");
        return 1;
    }

    if(frame->data[0]) {
        uint32_t hash = 0;
        for(int idx = 0; idx < frame->buf[0]->size; ++idx) hash += frame->buf[0]->data[idx];
        printf("frame hash=%u\n", hash);
    }

    AVPacket pkt2;

    memset(&pkt2, 0, sizeof(AVPacket));
    if(av_read_frame(avcontext, &pkt2) < 0) {
        puts("read_frame failed");
        return 1;
    }

	if(avcodec_send_packet(cocontext, &pkt2) < 0) {
        puts("avcodec_send_packet failed");
        return 1;
    }

    if(avcodec_receive_frame(cocontext, frame) < 0) {
        puts("avcodec_receive_frame failed");
        return 1;
    }

    if(frame->data[0]) {
        uint32_t hash = 0;
        for(int idx = 0; idx < 4*frame->linesize[0]; ++idx) {
            hash += frame->data[0][idx];
        }
        printf("frame hash=%u\n", hash);
    }

    return 0;
}