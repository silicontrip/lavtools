
typedef struct
{

int printX=0, printY=0, printH=-1, printW=-1;
int hsub, vsub;
char *imageName;

AVFrame  *pFrame;
AVFrame  *maskFrame;
PixelFormat maskFrameFormat;

} OvlContext;

static av_cold void uninit(AVFilterContext *ctx)
{
    OvlContext *ovl = ctx->priv;
	av_free(ovl->pFrame);
	av_free(ovl->maskFrame);

	ovl->pFrame = NULL;
	ovl->maskFrame = NULL;
}


static av_cold int init(AVFilterContext *ctx, const char *args, void *opaque)
{
    CropContext *crop = ctx->priv;

    if (args)

    return 0;
}

static int query_formats(AVFilterContext *ctx)
{
    AVFilterFormats *formats;
    enum PixelFormat pix_fmt;
    int ret;

    if (ctx->inputs[0]) {
        formats = NULL;
        for (pix_fmt = 0; pix_fmt < PIX_FMT_NB; pix_fmt++)
            if (   sws_isSupportedInput(pix_fmt)
                && (ret = avfilter_add_colorspace(&formats, pix_fmt)) < 0) {
                avfilter_formats_unref(&formats);
                return ret;
            }
        avfilter_formats_ref(formats, &ctx->inputs[0]->out_formats);
    }
    if (ctx->outputs[0]) {
        formats = NULL;
        for (pix_fmt = 0; pix_fmt < PIX_FMT_NB; pix_fmt++)
            if (    sws_isSupportedOutput(pix_fmt)
                && (ret = avfilter_add_colorspace(&formats, pix_fmt)) < 0) {
                avfilter_formats_unref(&formats);
                return ret;
            }
        avfilter_formats_ref(formats, &ctx->outputs[0]->in_formats);
    }

    return 0;
}



static int config_props(AVFilterLink *outlink)
{
    AVFormatContext *pFormatCtx;
	AVInputFormat *avif = NULL;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
	AVPacket        packet;
	
	int avStream = -1;
	int frameFinished;
	
	AVFilterContext *ctx = outlink->src;
    AVFilterLink *inlink = outlink->src->inputs[0];
	struct SwsContext *sws;
    OvlContext *ovl = ctx->priv;

	// check for scale setting on command line.
	// if none 

	
	/* open overlay image */
	
	if(av_open_input_file(pFormatCtx, ovl->imageName, avif, 0, NULL)!=0) {
	        av_log(ctx, AV_LOG_FATAL, "Cannot open overlay image.\n");
	}

	if(av_find_stream_info(pFormatCtx)<0) {
		av_log(ctx, AV_LOG_FATAL, "Cannot find stream in overlay image.\n");
	}
	
	for(int i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
				avStream=i;
				break;
			}
	
		if(avStream==-1) {
		av_log (ctx,AV_LOG_FATAL,"could not find an image stream in overlay image\n");
	}


	pCodecCtx=pFormatCtx->streams[avStream]->codec;
		
	// Find the decoder for the video stream
	*pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(*pCodec==NULL) {
		av_log(ctx, AV_LOG_FATAL ,"could not find codec for overlay image\n");
	}
	
	// Open codec
	if(avcodec_open(pCodecCtx, *pCodec)<0) {
		av_log(ctx, AV_LOG_FATAL,"could not open codec for overlay image\n");
	}

	// check for appropriate format.
	if (pCodecCtx->pix_fmt != PIX_FMT_ARGB &&
		pCodecCtx->pix_fmt != PIX_FMT_RGBA &&
		pCodecCtx->pix_fmt != PIX_FMT_ABGR &&
		pCodecCtx->pix_fmt != PIX_FMT_BGRA)
	{
		// warn if no alpha channel
		av_log(ctx,AV_LOG_WARNING, "overlay image has no alpha channel (assuming completely opaque)");
		
	}
	
	ovl->pFrame = avcodec_alloc_frame();
	ovl->maskFrame=avcodec_alloc_frame();
	
	// read overlay file
							
	av_read_frame(pFormatCtx, &packet);
	avcodec_decode_video(pCodecCtx, ovl->maskFrame, &frameFinished, packet->data, packet->size);

	ovl->maskFrameFormat = pCodecCtx->pix_fmt;
	// convert to output frame format.
	sws=sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
					ovl->printW, ovl->printH, inlink->format,
					SWS_BILINEAR, NULL, NULL, NULL);

	// convert the image 

	sws_scale(sws, ovl->maskFrame->data, ovl->maskFrame->linesize, 0, pCodecCtx->height, ovl->pFrame->data, ovl->pFrame->linesize);
	
	sws_freeContext(sws);
    // Close the codec
    avcodec_close(pCodecCtx);
	
    // Close the video file
    av_close_input_file(pFormatCtx);

	
}

static void start_frame(AVFilterLink *link, AVFilterPicRef *picref)
{
    OverlayContext *ovl = link->dst->priv;
    AVFilterLink *outlink = link->dst->outputs[0];
    AVFilterPicRef *outpicref;

    ovl->hsub = av_pix_fmt_descriptors[link->format].log2_chroma_w;
    ovl->vsub = av_pix_fmt_descriptors[link->format].log2_chroma_h;

    outpicref = avfilter_get_video_buffer(outlink, AV_PERM_WRITE, outlink->w, outlink->h);
    outpicref->pts = picref->pts;
    outlink->outpic = outpicref;

     avfilter_start_frame(outlink, avfilter_ref_pic(outpicref, ~0));
}


static void draw_slice(AVFilterLink *link, int y, int h)
{
    OvlContext *ovl = link->dst->priv;
    AVFilterPicRef *in  = link->cur_pic;
    AVFilterPicRef *out = link->dst->outputs[0]->outpic;
    uint8_t *inrow, *outrow;
    int i, j, plane;

    /* luma plane */
    inrow  = in-> data[0] + y * in-> linesize[0];
    outrow = out->data[0] + y * out->linesize[0];
    for(i = 0; i < h; i ++) {
        for(j = 0; j < link->w; j ++)
            outrow[j] = 255 - inrow[j] + neg->offY;
        inrow  += in-> linesize[0];
        outrow += out->linesize[0];
    }

    /* chroma planes */
    for(plane = 1; plane < 3; plane ++) {
        inrow  = in-> data[plane] + (y >> neg->vsub) * in-> linesize[plane];
        outrow = out->data[plane] + (y >> neg->vsub) * out->linesize[plane];

        for(i = 0; i < h >> neg->vsub; i ++) {
            for(j = 0; j < link->w >> neg->hsub; j ++)
                outrow[j] = 255 - inrow[j] + neg->offUV;
            inrow  += in-> linesize[plane];
            outrow += out->linesize[plane];
        }
    }

    avfilter_draw_slice(link->dst->outputs[0], y, h);
}




AVFilter avfilter_vf_overlay =
{
    .name      = "overlay",

    .priv_size = sizeof(NegContext),

    .query_formats = query_formats,

    .inputs    = (AVFilterPad[]) {{ .name            = "default",
                                    .type            = AV_PAD_VIDEO,
                                    .draw_slice      = draw_slice,
                                    .config_props    = config_props,
                                    .min_perms       = AV_PERM_READ, },
                                  { .name = NULL}},
    .outputs   = (AVFilterPad[]) {{ .name            = "default",
                                    .type            = AV_PAD_VIDEO, },
                                  { .name = NULL}},
};
