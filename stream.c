#include "definitions.h"

void
ximage_to_frame (struct SwsContext *sws_ctx, XImage *image, AVFrame *frame) {
	int linesize = image->width * 4;
	av_frame_make_writable(frame);
	sws_scale(sws_ctx, (const uint8_t * const *)&(image->data), &linesize, 0, image->height, frame->data, frame->linesize);
}

ost_t *
make_ost (char *filepath) {
	ost_t *ost = malloc(sizeof(ost_t));

	avformat_alloc_output_context2(&ost->fmt_ctx, NULL, NULL, filepath);

	ost->vst = NULL;

	return ost;
}

void
ost_add_video_stream (ost_t *ost, int width, int height) {
	ost->vst = avformat_new_stream(ost->fmt_ctx, NULL);
	ost->vst->id = ost->fmt_ctx->nb_streams - 1;

	ost->vst->time_base.num = 1;
	ost->vst->time_base.den = 20;

	ost->venc = avcodec_alloc_context3( \
			avcodec_find_encoder(ost->fmt_ctx->oformat->video_codec) \
		);

	AVCodecContext *enc = ost->venc;

	ost->frame = av_frame_alloc();
	ost->vpkt = av_packet_alloc();
	AVFrame *frame = ost->frame;

	enc->bit_rate = 400000;
	enc->width = frame->width = width;
	enc->height = frame->height = height;

	enc->time_base = ost->vst->time_base;
	enc->gop_size = 12;
	enc->max_b_frames = 2;
	enc->pix_fmt = frame->format = AV_PIX_FMT_YUV420P;

	if (ost->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (avcodec_open2(enc, NULL, NULL) < 0) {
		fprintf(stderr, "Cannot initialize video codec context");
		exit(1);
	}

	avcodec_parameters_from_context(ost->vst->codecpar, enc);

	av_frame_get_buffer(frame, 0);
	ost->frame_pts = 0;
}

void
ost_init_io (ost_t *ost) {
	av_dump_format(ost->fmt_ctx, 0, ost->fmt_ctx->url, 1);

	if (!(ost->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		if (avio_open(&(ost->fmt_ctx->pb), ost->fmt_ctx->url, AVIO_FLAG_WRITE) < 0) {
			fprintf(stderr, "Could not open file");
			exit(1);
		}
	}

	if (avformat_write_header(ost->fmt_ctx, NULL) < 0) {
		printe("[avformat] Unable to write header");
	}
}

void
ost_encode_frame (ost_t *ost) {
	int ret;

	ost->frame->pts = ost->frame_pts ++;
	ret = avcodec_send_frame(ost->venc, ost->frame);
	if (ret < 0) {
		printf("%s\n", strerror(errno));
		printe("[avcodec] Failed to send frame to encoder");
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(ost->venc, ost->vpkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		} else if (ret < 0) {
			printe("[avcodec] Could not recieve frame from encoder");
		}

		av_packet_rescale_ts(ost->vpkt, ost->venc->time_base, ost->vst->time_base);
		ost->vpkt->stream_index = ost->vst->index;

		av_interleaved_write_frame(ost->fmt_ctx, ost->vpkt);
		av_packet_unref(ost->vpkt);
	}
}

void
ost_free (ost_t *ost) {
	av_write_trailer(ost->fmt_ctx);

	av_frame_free(&ost->frame);
	av_packet_free(&ost->vpkt);

	if (!(ost->fmt_ctx->oformat->flags & AVFMT_NOFILE))
		avio_closep(&ost->fmt_ctx->pb);

	avio_close(ost->fmt_ctx->pb);

	if (ost->vst != NULL) {
		avcodec_free_context(&ost->venc);
	}

	avformat_free_context(ost->fmt_ctx);

	free(ost);
}
