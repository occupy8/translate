#ifndef VDEC_H_
#define VDEC_H_

typedef struct video_decoder_s {
    enum AVCodecID          codec_id;
    char                   *codec_name;
    unsigned                ok:1;
    AVFrame                *frame;
    AVPacket               *packet;
    AVCodecContext         *codec_ctx;
    void                   *extra;
} video_decoder_t;

typedef struct vdec_s {
    void                  *pool;
    ngx_event_t            timer;
    
    //queue
    packet_t               head;
    video_buf_t            buf;

    pthread_t              tid;
    thread_mutex_t         mtx;

    vdec_conf              conf;
    video_decoder_t       *dec;

    uint64_t               create_time;
    unsigned               exit:1;
} vdec_t;

vdec_t *vdec_creat(trans_t *trans, trans_conf_t *conf);
void vdec_destroy(vdec_t *vdec);

int vdec_input(vdec_t *vdec, packet_t *packet);
video_decoder_t *video_decoder_create(const int id, const char* name);
void video_decoder_destroy(video_decoder_t *dec);

int video_decoder_decode(video_decoder_t *dec, uint8_t *data, int size, uint32_t dts, uint32_t pts);

void dec_time_handler(ngx_event_t *ev);
void *vdec_thread_cycle(void *data);

int vdec_decode_loop(vdec_t *vdec);
int vdec_thread_init(vdec_t *vdec);
int vdec_thread_exit(vdec_t *vdec);

#endif
