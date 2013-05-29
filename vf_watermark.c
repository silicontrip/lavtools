/*
 * watermark
 * copyright (c) 2010 Mark Heath mjpeg0 @ silicontrip dot net 
 * http://silicontrip.net/~mark/lavtools/
 *
 * Places a transparent static image over the video.
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libavutil/pixdesc.h"
#include "libswscale/swscale.h"
#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"
#include <strings.h>

/* time
2010-02-25
 0735-0740 mask to AVFrame
 1800-1822 mask to AVFrame
 
 2010-02-26
 0818-0853 config file read
 1707-1748 config file and interval display
 2100-2116 config file debug. 
 2116- interval code
*/

typedef struct
{
	
	int printX, printY, printH, printW;
	int printON, printINT;
	int hsub, vsub, mask;
	char *imageName;
	
	int fr_num;
	int fr_dem;
	AVFrame  *pFrame;
// it appears that maskFrame is erased
	AVFrame  *maskFrame;
	
} OverlayContext;

static av_cold void uninit(AVFilterContext *ctx)
{
    OverlayContext *ovl = ctx->priv;
	if (ovl->pFrame)
		av_free(ovl->pFrame);
		
	if (ovl->maskFrame) 
		av_free(ovl->maskFrame);
	
	if (ovl->imageName)
		av_free(ovl->imageName);
		
		
	av_log(ctx, AV_LOG_DEBUG, "uninit().\n");

	ovl->imageName = NULL;
	ovl->pFrame = NULL;
	ovl->maskFrame = NULL;
}


static av_cold int init(AVFilterContext *ctx, const char *args)
{
    OverlayContext *ovl = ctx->priv;
	int argc=0,off;
	char *scale = ovl->imageName;
	
	ovl->printX = 0;
	ovl->printY = 0;
	ovl->printW = -1;
	ovl->printH = -1;
	ovl->printON = -1;
	off = -1;
	
	av_log(ctx, AV_LOG_DEBUG, "init(). %s\n",args);
	
    if (args) { 
	
		for (int i=0; i < strlen(args); i++)
			if (args[i] == ':') 
				argc++;

		if (argc==0) {
		
			if (!strcmp(".cfg",args+(strlen(args)-4))) {
				FILE *fd;
				char val[256];
				char key[256]; // subject to buffer overflows
				
				fd=fopen(args,"r");
				if (fd == NULL) {
					av_log(ctx, AV_LOG_ERROR, "Config file not found.\n");
					return -1;
				}
				
				fseek(fd,0,SEEK_END);
				// a cheap and quick way to get the filesize, avoiding an fstat struct
				ovl->imageName=(char*)av_malloc(ftell(fd));
				rewind(fd);
				
				while(!feof(fd)) {
				
					fscanf(fd,"%s %s\n",key,val);
					
					av_log(ctx,AV_LOG_INFO," read: %s %s\n",key,val);
					
					//scan through all lines
					if (!strcasecmp("filename",key)){
						strcpy(ovl->imageName,val);
					} else 
					if (!strcasecmp("x",key)){
						ovl->printX=atoi(val);
					} else
					if (!strcasecmp("y",key)){
						ovl->printY=atoi(val);
					} else
					if (!strcasecmp("w",key)){
						ovl->printW=atoi(val);
					} else
					if (!strcasecmp("h",key)){
						ovl->printH=atoi(val);
					} else
					if (!strcasecmp("ontime",key)){
						ovl->printON=atoi(val);
					} else
					if (!strcasecmp("offtime",key)){
						off=atoi(val);
					} else {
						av_log(ctx,AV_LOG_ERROR,"error in config file.\n");
					}
				}
			}
		} else {
			ovl->imageName = (char *) av_malloc(strlen(args)+1);
			strcpy(ovl->imageName,args);
		}
		if (argc>0) {
		// a hacky way to split the arguments
			for (int i=0; i < strlen(ovl->imageName); i++)
				if (ovl->imageName[i] == ':') {
					ovl->imageName[i] = '\0';
					scale = ovl->imageName+i+1;
					break;
				}
		
			if (argc==2) {
			
				sscanf(scale, "%d:%d",&ovl->printX, &ovl->printY);
			} else if (argc==4) {
				sscanf(scale, "%d:%d:%d:%d",&ovl->printX, &ovl->printY,&ovl->printW,&ovl->printH);
			} else if (argc==6) {
				sscanf(scale, "%d:%d:%d:%d:%d:%d",&ovl->printX, &ovl->printY,&ovl->printW,&ovl->printH,&ovl->printON,&off);
			} else {
				av_log(ctx, AV_LOG_ERROR, "Incorrect number of parameters passed to watermark.\n");
				return -1;
			}
		}

	 }
	 if (off != -1 && ovl->printON != -1) 
		ovl->printINT = off + ovl->printON;
	 
	av_log(ctx,AV_LOG_DEBUG, "init(). %s X %d, Y %d, H %d, W %d\n",ovl->imageName, ovl->printX,ovl->printY,ovl->printH,ovl->printW);

	 //  check incorrect combination of parameters
	 if (ovl->printW < 0 && ovl->printH > 0) {
		av_log(ctx, AV_LOG_ERROR, "Height without Width specified.\n");
		return -1;
	}
	if (ovl->printH < 0 && ovl->printW > 0) {
		av_log(ctx, AV_LOG_ERROR, "Width without Height specified.\n");
		return -1;
	}

	if (ovl->printON < 0 && off > 0) {
		av_log(ctx, AV_LOG_ERROR, "Off time without On time specified.\n");
		return -1;
	}
	if (ovl->printON > 0 && off < 0) {
		av_log(ctx, AV_LOG_ERROR, "On time without Off time specified.\n");
		return -1;
	}

	 	 
	return 0;
}

static int query_formats(AVFilterContext *ctx)
{
/*
    AVFilterFormats *formats;
    enum PixelFormat pix_fmt;
    int ret;
*/	
		
    enum PixelFormat pix_fmts[] = {
        PIX_FMT_YUV444P,  PIX_FMT_YUV422P,  PIX_FMT_YUV420P,
        PIX_FMT_YUVJ444P, PIX_FMT_YUVJ422P, PIX_FMT_YUVJ420P,	
        PIX_FMT_NONE
    };

	av_log(ctx, AV_LOG_DEBUG, ">>> query_formats().\n");

    ff_set_common_formats(ctx, ff_make_format_list(pix_fmts));

	av_log(ctx, AV_LOG_DEBUG, "<<< query_formats().\n");
    return 0;
}



static int config_props(AVFilterLink *outlink)
{

	AVFilterContext *ctx = outlink->src;
	OverlayContext *ovl = ctx->priv;

    AVFilterLink *inlink = outlink->src->inputs[0];
	
    AVFormatContext *pFormatCtx;
	AVInputFormat *avif = NULL;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
	AVPacket        packet;
	AVFrame *overlay, *tempMask;
	
	int avStream = -1;
	int frameFinished;
	
	struct SwsContext *sws;
	
	uint8_t *data;
	uint8_t *maskData;
	uint8_t *tempData;
	
    av_log(ctx, AV_LOG_DEBUG, ">>> config_props().\n");

    
    
	// make sure Chroma planes align.
	avcodec_get_chroma_sub_sample(outlink->format, &ovl->hsub, &ovl->vsub);
	
//	av_log(ctx,AV_LOG_INFO,"hsub: %d vsub: %d iformat: %d oformat %d\n",ovl->hsub,ovl->vsub,inlink->format,outlink->format);

	if ((ovl->printX % (1<<ovl->hsub) && ovl->hsub!=1)||(ovl->printY % (1<<ovl->vsub) && ovl->vsub!=1)) {
			av_log(ctx, AV_LOG_ERROR, "Cannot use this position with this chroma subsampling. Chroma plane will not align. (continuing with unaligned chroma planes, your watermark may look distorted)\n");
	}

    av_log(ctx, AV_LOG_DEBUG, "    config_props() avformat_open_input(%s).\n",ovl->imageName);

    pFormatCtx = avformat_alloc_context();
    
	// open overlay image 
	// avformat_open_input
	if(avformat_open_input(&pFormatCtx, ovl->imageName, avif, NULL)!=0) {
		av_log(ctx, AV_LOG_FATAL, "Cannot open overlay image (%s).\n",ovl->imageName);
		return -1;
		
	}
	
    av_log(ctx, AV_LOG_DEBUG, "    config_props() avformat_find_stream_info.\n");

    
	if(avformat_find_stream_info(pFormatCtx,NULL)<0) {
		av_log(ctx, AV_LOG_FATAL, "Cannot find stream in overlay image.\n");
		return -1;
		
	}
	
    av_log(ctx, AV_LOG_DEBUG, "    config_props() pFormatCtx->streams.\n");

    
	for(int i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			avStream=i;
			break;
		}
	
	if(avStream==-1) {
		av_log (ctx,AV_LOG_FATAL,"could not find an image stream in overlay image\n");
		return -1;
	}
	
    av_log(ctx, AV_LOG_DEBUG, "    config_props() avcodec_find_decoder.\n");

	
	pCodecCtx=pFormatCtx->streams[avStream]->codec;
	
	// Find the decoder for the video stream
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	
	if(pCodec==NULL) {
		av_log(ctx, AV_LOG_FATAL ,"could not find codec for overlay image\n");
		return -1;
		
	}
	
    av_log(ctx, AV_LOG_DEBUG, "    config_props() avcodec_open2.\n");

    
	// Open codec
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0) {
		av_log(ctx, AV_LOG_FATAL,"could not open codec for overlay image\n");
		return -1;
		
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
	
    av_log(ctx, AV_LOG_DEBUG, "    config_props() avcodec_alloc_frame.\n");

    
	overlay = avcodec_alloc_frame();
	
	// read overlay file into overlay AVFrame
	
	av_read_frame(pFormatCtx, &packet);
	avcodec_decode_video2(pCodecCtx, overlay, &frameFinished, &packet);

	// will always be GRAY8
	
	// should be all or nothing, so no real need to test both
	// testing both incase one was missed.
	if (ovl->printW == -1 || ovl->printH == -1)
	{
		ovl->printW = pCodecCtx->width;
		ovl->printH = pCodecCtx->height;
	}
	
	// Allocate AVFrames and image buffers
	ovl->pFrame=avcodec_alloc_frame();
	ovl->maskFrame=avcodec_alloc_frame();
	tempMask = avcodec_alloc_frame();
	
	data = (uint8_t *) av_malloc(avpicture_get_size(inlink->format, ovl->printW, ovl->printH));
	maskData = (uint8_t *) av_malloc(avpicture_get_size(PIX_FMT_GRAY8, ovl->printW, ovl->printH));
	tempData = (uint8_t *) av_malloc(avpicture_get_size(PIX_FMT_GRAY8, pCodecCtx->width, pCodecCtx->height));

	avpicture_fill((AVPicture *)tempMask, tempData, PIX_FMT_GRAY8, pCodecCtx->width, pCodecCtx->height);
	avpicture_fill((AVPicture *)ovl->maskFrame, maskData, PIX_FMT_GRAY8, ovl->printW, ovl->printH);
	avpicture_fill((AVPicture *)ovl->pFrame, data, inlink->format, ovl->printW, ovl->printH); 
	
	
	av_log(ctx,AV_LOG_DEBUG,"mask linesize %d\n",ovl->maskFrame->linesize[0]);
	
	// copy the alpha mask, it appears to be getting lost during sws_scale
	/*	copy the alpha mask, it appears to be getting lost during sws_scale
		copy the alpha if it exists and then scale it. */
	ovl->mask=0;
	if (pCodecCtx->pix_fmt == PIX_FMT_ARGB ||
		pCodecCtx->pix_fmt == PIX_FMT_RGBA ||
		pCodecCtx->pix_fmt == PIX_FMT_ABGR ||
		pCodecCtx->pix_fmt == PIX_FMT_BGRA)
	{

		// copy the alpha if it exists and then scale it.
		int alpha = 0;
		if (pCodecCtx->pix_fmt == PIX_FMT_RGBA || pCodecCtx->pix_fmt == PIX_FMT_BGRA) { alpha = 3; }
		
		for (int y=0; y < pCodecCtx->height; y++) {
			// memcpy((tempMask->data[0] + y * tempMask->linesize[0]),
			for (int x=0; x < pCodecCtx->width; x++) {
				*(tempMask->data[0] + y * tempMask->linesize[0] + x ) = *(overlay->data[0] + y * overlay->linesize[0] + x* 4 + alpha);
			}
		}
		// scale and copy
		
		av_log(ctx,AV_LOG_DEBUG," in: %dx%d, out %dx%d\n",pCodecCtx->width, pCodecCtx->height,ovl->printW, ovl->printH);
		
		// scale & copy, even if we don't scale, we still need to copy
				
		sws=sws_getContext(pCodecCtx->width, pCodecCtx->height, PIX_FMT_GRAY8, 
						   ovl->printW, ovl->printH, PIX_FMT_GRAY8,
						   SWS_BILINEAR, NULL, NULL, NULL);
		
		sws_scale(sws, (const uint8_t * const *)tempMask->data, tempMask->linesize, 0, pCodecCtx->height,
				  ovl->maskFrame->data, ovl->maskFrame->linesize);
				  
				  ovl->mask = 1;

	}
	
	av_log(ctx,AV_LOG_DEBUG, "config_props() sws_getContext\n");

	
	av_log(ctx,AV_LOG_DEBUG,"inlink format %d, png format %d\n",inlink->format,pCodecCtx->pix_fmt);
	
	// convert to output frame format.

	
	sws=sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
					   ovl->printW, ovl->printH, inlink->format,
					   SWS_BILINEAR, NULL, NULL, NULL);
	
	// set the output filter frame size to the input frame size.
	
	outlink->w = inlink->w;
    outlink->h = inlink->h;
	
	// convert the image 

	sws_scale(sws, (const uint8_t * const *)overlay->data, overlay->linesize, 0, pCodecCtx->height,
				ovl->pFrame->data, ovl->pFrame->linesize);
	

	av_free(tempMask);
	av_free(overlay);
	sws_freeContext(sws);
    // Close the codec
    avcodec_close(pCodecCtx);
	
    // Close the video file
    avformat_close_input(&pFormatCtx);
	
    av_log(ctx, AV_LOG_DEBUG, "<<< config_props().\n");

    
    return 0;
	
	
}

/*
static void start_frame(AVFilterLink *link, AVFilterPicRef *picref)
{
    AVFilterLink *outlink = link->dst->outputs[0];
    AVFilterPicRef *outpicref;
		
    outpicref = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    outpicref->pts = picref->pts;
		
    outlink->outpic = outpicref;
	
	avfilter_start_frame(outlink, avfilter_ref_pic(outpicref, ~0));
}
*/
// replaced by filter_frame...
// need refactoring
// static void draw_slice(AVFilterLink *link, int y, int h, int slice_dir)
static int filter_frame(AVFilterLink *link, AVFrame *in)
{
    OverlayContext *ovl = link->dst->priv;
    AVFilterLink *outlink = link->dst->outputs[0];

    AVFrame *out;
    
    int i, j;
	int lumaFrame, lumaPrint, lumaMask,uPrint,vPrint;
	int py;
	int h = link->h;
    int y = 0;
    
    av_log(link->src, AV_LOG_DEBUG, "filter_frame().\n");

    
	py = ovl->printY;
	
	// use py to disable the watermark for interval display
	
	if (ovl->printINT > 0) {
		i = (in->pts / AV_TIME_BASE) % ovl->printINT ;
		if (i >= ovl->printON) py = y+h+1; // hack to turn off water mark 
	}
	
    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    av_frame_copy_props(out, in);


	lumaMask=255;

	for (j=0; j<h; j++) {
		if (y+j<py || y+j>=py+ovl->printH) {
			memcpy((out->data[0])+(j+y)*out->linesize[0],(in->data[0])+(y+j)*in->linesize[0],link->w);

			if (j%(1<<ovl->vsub)) {
				memcpy((out->data[1])+((j+y)>>ovl->vsub)*out->linesize[1],(in->data[1])+((j+y)>>ovl->vsub)*in->linesize[1],link->w >> ovl->hsub);
				memcpy((out->data[2])+((j+y)>>ovl->vsub)*out->linesize[2],(in->data[2])+((j+y)>>ovl->vsub)*in->linesize[2],link->w >> ovl->hsub);
			}
		} else {
		
			for (i=0;i<link->w;i++) {
			// check X to see if we are over the overlay
				if (i<ovl->printX || i>=ovl->printX+ovl->printW) {
					*((out->data[0])+(j+y)*out->linesize[0]+i) = *((in->data[0])+(j+y)*in->linesize[0]+i);
				 if (!(j%(1<<ovl->vsub) && i%(1<<ovl->hsub))) {
						*((out->data[1])+((j+y)>>ovl->vsub)*out->linesize[1]+(i>>ovl->hsub))=*((in->data[1])+((j+y)>>ovl->vsub)*in->linesize[1]+(i>>ovl->hsub));
						*((out->data[2])+((j+y)>>ovl->vsub)*out->linesize[2]+(i>>ovl->hsub))=*((in->data[2])+((j+y)>>ovl->vsub)*in->linesize[2]+(i>>ovl->hsub));
				}
				} else {
					// draw image

					lumaFrame = *((in->data[0])+(j+y)*in->linesize[0]+i); // input luma
					lumaPrint = *((ovl->pFrame->data[0])+(j+y-ovl->printY)*ovl->pFrame->linesize[0]+i-ovl->printX);  // overlay luma
					uPrint = *((ovl->pFrame->data[1])+((j+y-ovl->printY)>>(ovl->vsub))*ovl->pFrame->linesize[1]+((i-ovl->printX)>>(ovl->hsub))); // overlay chroma
					vPrint = *((ovl->pFrame->data[2])+((j+y-ovl->printY)>>(ovl->vsub))*ovl->pFrame->linesize[2]+((i-ovl->printX)>>(ovl->hsub))); // overlay chroma

					if (ovl->mask) {
						lumaMask = *(ovl->maskFrame->data[0] + (j+y - ovl->printY) * ovl->maskFrame->linesize[0] + i-ovl->printX ); // mask intensity
					}
					
					if (lumaMask == 255) { 	// optimize for completely opaque
						*((out->data[0])+(j+y)*out->linesize[0]+i) = lumaPrint;
						if (!(j%(1<<ovl->vsub) && i%(1<<ovl->hsub))) {
							*((out->data[1])+((j+y)>>ovl->vsub)*out->linesize[1]+(i>>ovl->hsub))= uPrint;
							*((out->data[2])+((j+y)>>ovl->vsub)*out->linesize[2]+(i>>ovl->hsub))= vPrint;
						}
					} else if (lumaMask == 0) { // optimize for completely transparent

						*((out->data[0])+(j+y)*out->linesize[0]+i) = *((in->data[0])+(j+y)*in->linesize[0]+i);
						if (!(j%(1<<ovl->vsub) && i%(1<<ovl->hsub))) {
							*((out->data[1])+((j+y)>>ovl->vsub)*out->linesize[1]+(i>>ovl->hsub))=*((in->data[1])+((j+y)>>ovl->vsub)*in->linesize[1]+(i>>ovl->hsub));
							*((out->data[2])+((j+y)>>ovl->vsub)*out->linesize[2]+(i>>ovl->hsub))=*((in->data[2])+((j+y)>>ovl->vsub)*in->linesize[2]+(i>>ovl->hsub));
						}
					} else {
						*((out->data[0])+(j+y)*out->linesize[0]+i) = (lumaFrame* (255 - lumaMask) + lumaPrint * lumaMask) / 255;
					if (!(j%(1<<ovl->vsub) && i%(1<<ovl->hsub))) {
							*((out->data[1])+((j+y)>>ovl->vsub)*out->linesize[1]+(i>>ovl->hsub))= (*((in->data[1])+((j+y)>>ovl->vsub)*in->linesize[1]+(i>>ovl->hsub)) * (255-lumaMask) + uPrint *lumaMask) / 255;
							*((out->data[2])+((j+y)>>ovl->vsub)*out->linesize[2]+(i>>ovl->hsub))=(*((in->data[2])+((j+y)>>ovl->vsub)*in->linesize[2]+(i>>ovl->hsub)) * (255-lumaMask) + vPrint *lumaMask) / 255;
						}
					}
				}
			}
		}
	}
	av_frame_free(&in);
    return ff_filter_frame(outlink, out);

}


static const AVFilterPad avfilter_vf_watermark_inputs[] = {
    {
    .name            = "default",
    .type            = AVMEDIA_TYPE_VIDEO,
    .min_perms       = AV_PERM_READ,
    .filter_frame = filter_frame,

    },
    { NULL}
};

static const AVFilterPad avfilter_vf_watermark_outputs[] = {
    {
        .name            = "default",
        .config_props     = config_props,
        .type            = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter avfilter_vf_watermark = {
    .name      = "watermark",
	.description = "Add an overlay picture onto the video.",
	
	.init      = init,
    .uninit    = uninit,
    .query_formats = query_formats,
	
	.priv_size = sizeof(OverlayContext),

    .inputs    = avfilter_vf_watermark_inputs,
    .outputs   = avfilter_vf_watermark_outputs
};
