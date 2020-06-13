#include "vdec.h"

vdec_t *vdec_creat(trans_t *trans, trans_conf_t *conf)
{
    int        rc;
    vdec_t    *vdec = NULL;
    
    vdec = (vdec_t *)malloc(sizeof(vdec_t));
    if (vdec == NULL) {
        goto failed;
    }

    memset(vdec, 0, sizeof(vdec_t));

    vdec->create_time = current_time;
    vdec->exit = 0;
    vdec->conf = conf;

    packet_t *h = &vdec->head;
    h->prev = h->next = h;

    if (vdec->conf.codec == CODEC_H264) {
        vdec->dec = video_decoder_create(0, "h264");
    } else if (vdec->conf.codec == CODEC_H265) {
        videc->dec = video_decoder_create(0, "hevc");
    } else {
        goto failed;
    }

    if (vdec->dec == NULL) {
        goto failed;
    }

    vdec->dec->extra = vdec;

    rc = vdec_thread_init(vdec);
    if (rc != 0) {
        goto failed;
    }

    vdec->timer.handler = vdec_timer_handler;
    vdec->timer.data = vdec;

    ngx_add_timer(&vdec->timer, 50);
}

void vdec_destroy(vdec_t *vdec)
{
    if (vdec == NULL) {
        return;
    }

    if (vdec && vdec->timer.timer_set) {
    	ngx_del_timer(&vdec->timer);
    }

    //realse

    vdec->exit = 1;
}

int vdec_input(vdec_t *vdec, packet_t *pkt)
{
    if (vdec->exit) {
        return -1;
    }

    packet_t *packet = packet_alloc(pkt->len);
    if (packet == NULL) {
        return -1;
    }

    uint8_t *data = pkt->data;
    *packet = *pkt;
    memcpy(packet->data, pkt->data, pkt->len);

    mutex_lock(&vdec->mtx);
    //insert queue
    //
    mutex_unlock(&vdec->mtx);
}

video_decoder_t *video_decoder_create(const int id, const char* name)
{
    const AVCodec           *codec;
    video_decoder_t         *dec = NULL;

    dec = (video_decoder_t *)malloc(sizeof(video_decoder_t));
    if (dec == NULL) {
        return NULL;
    }

    memset(dec, 0, sizeof(video_decoder_t));
    dec->codec_id = (enum AVCodecID)id;

    //init by id
    codec = avcodec_find_decoder(dec->codec_id);
    if (codec == NULL) {
        goto failed;
    }
    //init by name
    //
    dec->codec_ctx = avcodec_alloc_context3(codec);
    if (dec->codec_ctx) {
        goto failed;
    }

    if (avcodec_open2(dec->codec_ctx, codec, NULL) < 0) {
        goto failed;
    }

    dec->frame = av_frame_alloc();
    if (dec->frame == NULL) {
        goto failed;
    }

    dec->packet = av_packet_alloc();
    if (dec->packet == NULL) {
        goto failed;
    }

    return dec;

failed:
    video_decoder_destroy(dec);
    return NULL;
}

void video_decoder_destroy(video_decoder_t *dec)
{
    if (dec) {
        if (dec->codec_name) {
	  //free
	}

	if (dec->codec_ctx) {
	    avcodec_free_context(&dec->codec_ctx);
	    dec->codec_ctx = NULL;
	}

	if (dec->frame) {
	    av_frame_free(&dec->frame);
	    dec->frame = NULL;
	}

	if (dec->packet) {
	    av_packet_free(&dec->packet);
	    dec->packet = NULL;
	}

	free(dec);
    }
}

int video_decoder_decode(video_decoder_t *dec, uint8_t *data, int size, uint32_t dts, uint32_t pts)
{
    int     ret = 0;
    vdec_t *vdec = NULL;

    AVCodecContext *ctx = dec->codec_ctx;

    dec->packet->data = data;
    dec->packet->size = size;
    dec->packet->dts = dts;
    dec->packet->pts = pts;

    ret = avcodec_send_packet(ctx, dec->packet);
    if (ret < 0) {
        return ret;
    }

    dec->ok = 0;

    while (ret >= 0) {
        ret = avcodec_receive_frame(ctx, dec->frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
	    return 0;
	} else if (ret < 0) {
	    return ret;
	}

	dec->ok = 1;

	if (dec->extra) {
	    vdec = (vdec_t *)dec->extra;

	    vdec->buf.dts = dec->frame->pkt_dts;
	    vdec->buf.pts = dec->frame->pts;
	    vdec->buf.width = dec->frame->width;
	    vdec->buf.height = dec->frame->height;
	    vdec->buf.fmt = dec->codec_ctx->pix_fmt;
	    vdec->buf.linesize[0] = dec->frame->linesize[0];
	    vdec->buf.linesize[1] = dec->frame->linesize[1];
	    vdec->buf.linesize[2] = dec->frame->linesize[2];

	    memcpy(vdec->buf.y, dec->frame->data[0], dec->frame->linesize[0] * dec->frame->height);
	    memcpy(vdec->buf.u, dec->frame->data[1], dec->frame->linesize[1] * dec->frame->height / 2);
	    memcpy(vdec->buf.v, dec->frame->data[2], dec->frame->linesize[2] * dec->frame->height / 2);
	}
    }

    return 0;
}

void dec_time_handler(ngx_event_t *ev)
{

}

void *vdec_thread_cycle(void *data)
{
    int             err;
    sigset_t        set;
    int             rc;
    vdec_t         *vdec = NULL;

    if (data == NULL) {
    	return NULL;
    }

    vdec = (vdec_t *)data;

    sigfillset(&set);
    sigdelset(&set, SIGILL);
    sigdelset(&set, SIGFPE);
    sigdelset(&set, SIGSEGV);
    sigdelset(&set, SIGBUS);
    err = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (err) {
        return NULL;
    }

    while(1) {
        rc = vdec_decode_loop(vdec);
	if (rc == NGX_ABORT) {
	    break;
	}
    }

    vdec_thread_exit(vdec);

    return NULL;
}

int vdec_decode_loop(vdec_t *vdec)
{
    //output queue
    //decode
    //input 
}

int vdec_thread_init(vdec_t *vdec)
{
    int              err;
    pthread_attr_t   attr;

    if (vdec == NULL) {
    	return -1;
    }

    //create mutex

    err = pthread_attr_init(&attr);
    if (err) {
    	return -1;
    }

    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err) {
    	return -1;
    }

    err = pthread_create(&vdec->tid, &attr, vdec_func, vdec);
    if (err) {
        return -1;
    }

    pthread_attr_destroy(&attr);

    return 0;
}

int vdec_thread_exit(vdec_t *vdec)
{
    //release queue
    
    if (vdec->dec) {
        vdec->dec->extra = NULL;
	video_decoder_destroy(vdec->dec);
	vdec->dec = NULL;
    }

    //release mutex

    pthread_exit(0);

    return 0;
}
