/*
 ** <h3>broadcast levels</h3>
 * copyright (c) 2010 Mark Heath mjpeg0 @ silicontrip dot net 
 * http://silicontrip.net/~mark/lavtools/
 *
 **<p> Converts full swing levels to broadcast swing for file which have been incorrectly labelled.</p>
 ** <p> Performs clipping, scaling or detection of out of range Luma values (16-235)</P>
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


typedef struct
{	
	int mode;
	int frames;		
	int hsub, vsub;
} levelsContext;

/*
 static av_cold void uninit(AVFilterContext *ctx)
 {
 levelsContext *ovl = ctx->priv;
 if (ovl->pFrame)
 av_free(ovl->pFrame);
 
 if (ovl->maskFrame) 
 av_free(ovl->maskFrame);
 
 if (ovl->imageName)
 av_free(ovl->imageName);
 
 ovl->imageName = NULL;
 ovl->pFrame = NULL;
 ovl->maskFrame = NULL;
 }
 */

// static av_cold int init(AVFilterContext *ctx, const char *args, void *opaque)
static av_cold int init(AVFilterContext *ctx, const char *args)
{
    levelsContext *lctx = ctx->priv;
	int argc=0;
	
	
	char *frames_args;
	
	lctx->frames =-1;
	lctx->mode =-1;
	
    if (args) { 
		
		
		for (int i=0; i < strlen(args); i++)
			if (args[i] == ':') 
				argc++;
		
		if (argc==1) {
			frames_args = (char *) av_malloc(strlen (args));

			for (int i=0; i < strlen(args); i++)
				if (args[i] == ':') {
					frames_args = strcpy(frames_args,args+i+1);
					
				av_log(ctx, AV_LOG_DEBUG, "frames (%s).\n",frames_args);

					break;
				}
			lctx->frames = atoi(frames_args);
			av_free(frames_args);
		}
		
		if (!strncasecmp("clip",args,4)) {
			lctx->mode=1;
		} else if (!strncasecmp("scale",args,5))  {
			lctx->mode=2;
		} else if (!strncasecmp("detect",args,6))  {
			lctx->mode=3;
		} 
		
		
		if (lctx->mode == -1) {
			av_log(ctx, AV_LOG_FATAL, "Unknown Broadcast filter mode. Only clip, scale and detect supported (%s).\n",args);
			return -1;
		}

		
		return 0;
	}
	
	return -1;
}

static int query_formats(AVFilterContext *ctx)
{		
    enum PixelFormat pix_fmts[] = {
        PIX_FMT_YUV444P,  PIX_FMT_YUV422P,  PIX_FMT_YUV420P,
        PIX_FMT_NONE
    };
	
    ff_set_common_formats(ctx, ff_make_format_list(pix_fmts));
    return 0;
}



static int config_props(AVFilterLink *outlink)
{
	
	AVFilterContext *ctx = outlink->src;
	levelsContext *lctx = ctx->priv;
	AVFilterLink *inlink = outlink->src->inputs[0];

	avcodec_get_chroma_sub_sample(outlink->format, &lctx->hsub, &lctx->vsub);
	
	outlink->w = inlink->w;
    outlink->h = inlink->h;

	
	return 0;
	
	
}

// static void draw_slice(AVFilterLink *link, int y, int h, int slice_dir)
static int filter_frame(AVFilterLink *link, AVFrame *in)
{
   
    levelsContext *lctx = link->dst->priv;
    AVFilterLink *outlink = link->dst->outputs[0];
    AVFrame *out; // = link->dst->outputs[0]->outpic;
	AVFilterContext *ctx = link->src;


    int y = 0;
    int h = outlink->h;
    
	// Broadcast swing levels, min and max
	int min=16;
	int max=235;
	int luma,chromab,chromar;
	int i,j;
	
    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    av_frame_copy_props(out, in);

    
	if (lctx->frames == 0) {
		// nothing detected, shut off
		memcpy((out->data[0]+y*out->linesize[0]),(in->data[0]+y*in->linesize[0]),in->linesize[0]*h);
		memcpy((out->data[1]+(y>>lctx->vsub)*out->linesize[1]),(in->data[1]+(y>>lctx->vsub)*in->linesize[1]),in->linesize[1]*(h>>lctx->vsub));
		memcpy((out->data[2]+(y>>lctx->vsub)*out->linesize[2]),(in->data[2]+(y>>lctx->vsub)*in->linesize[2]),in->linesize[2]*(h>>lctx->vsub));
	} else {
		
		if (lctx->frames > 0) { 
				av_log(ctx, AV_LOG_DEBUG, "frames %d.\n", lctx->frames);
				lctx->frames--;
		}
		// run filter, either in detect mode 
		// or permanently
		
		// copy chroma
		// now scaling chroma too
		//memcpy((out->data[1]+(y>>lctx->vsub)*out->linesize[1]),(in->data[1]+(y>>lctx->vsub)*in->linesize[1]),in->linesize[1]*(h>>lctx->vsub));
		//memcpy((out->data[2]+(y>>lctx->vsub)*out->linesize[2]),(in->data[2]+(y>>lctx->vsub)*in->linesize[2]),in->linesize[2]*(h>>lctx->vsub));
		
		
		for (j=0; j<h; j++) {
			for (i=0;i<link->w;i++) {
				luma = *((in->data[0])+(j+y)*in->linesize[0]+i);
				 if (!(j%(1<<lctx->vsub) && i%(1<<lctx->hsub))) {
					 chromab=*((in->data[1])+((j+y)>>lctx->vsub)*in->linesize[1]+(i>>lctx->hsub));
					 chromar=*((in->data[2])+((j+y)>>lctx->vsub)*in->linesize[2]+(i>>lctx->hsub));
				 }
				
				// should put some strategy here rather than this conditional logic
				
				if (lctx->mode == 1) { // clip
					if (luma >max) {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",luma);
							lctx->frames = -1;  // continue permanently
						}
						luma = max;

					}
					if (luma < min) {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",luma);
							lctx->frames = -1;  // continue permanently
						}
						luma = min;

					}
					if (chromab >max) {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",chromab);
							lctx->frames = -1;  // continue permanently
						}
						chromab = max;

					}
					if (chromab < min) {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",chromab);
							lctx->frames = -1;  // continue permanently
						}
						chromab = min;

					}
					if (chromar >max) {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",chromar);
							lctx->frames = -1;  // continue permanently
						}
						chromar = max;

					}
					if (chromar < min) {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",chromar);
							lctx->frames = -1;  // continue permanently
						}
						chromar = min;

					}
					
					
				} else if (lctx->mode == 2) { // scale
					
					if (luma >max || luma < min)  {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",luma);
							lctx->frames = -1;  // continue permanently
						}
					
						luma= luma * (max-min) / 255 + min  ;
					}
					if (chromab >max || chromab < min)  {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",chromab);
							lctx->frames = -1;  // continue permanently
						}
					
						chromab= chromab * (max-min) / 255 + min  ;
					}
					if (chromar >max || chromar < min)  {
						if (lctx->frames >0) {
							av_log(ctx, AV_LOG_WARNING, "Out of range level detected. Filter locked. (%d)\n",chromar);
							lctx->frames = -1;  // continue permanently
						}
					
						chromar= chromar * (max-min) / 255 + min  ;
					}
					
				} else if (lctx->mode == 1) { // detect
					// scan, we will stop after frames number of frames
					// not continue if detected
					
					if (luma >max || luma < min)
						av_log(ctx, AV_LOG_WARNING, "Out of range level detected. (%d)\n",luma);
					
				}
				
				
				*((out->data[0])+(j+y)*out->linesize[0]+i) = luma;
				
				// do I need to perform Chroma compression?
				
				 if (!(j%(1<<lctx->vsub) && i%(1<<lctx->hsub))) {
					 *((out->data[1])+((j+y)>>lctx->vsub)*out->linesize[1]+(i>>lctx->hsub))=chromab;
					 *((out->data[2])+((j+y)>>lctx->vsub)*out->linesize[2]+(i>>lctx->hsub))=chromar;
				 }
			}
		}
	}
	
	
	
	av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}

static const AVFilterPad avfilter_vf_broadcast_inputs[] = {
    {
        .name            = "default",
        .type            = AVMEDIA_TYPE_VIDEO,
        .min_perms       = AV_PERM_READ,
        .filter_frame = filter_frame,
        
    },
    { NULL }
};

static const AVFilterPad avfilter_vf_broadcast_outputs[] = {
    {
        .name            = "default",
        .config_props     = config_props,
        .type            = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};


AVFilter avfilter_vf_broadcast = {
    .name      = "broadcast",
	.description = "clips or scales full swing levels to broadcast swing.",
	
	.init = init,
	//    .uninit    = uninit,
    .query_formats = query_formats,
	
	.priv_size = sizeof(levelsContext),
	
    .inputs    = avfilter_vf_broadcast_inputs,
    .outputs   = avfilter_vf_broadcast_outputs,
};
