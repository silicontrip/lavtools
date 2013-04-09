/*
 ** <h3>values graph</h3>
 * copyright (c) 2010 Mark Heath mjpeg0 @ silicontrip dot net 
 * http://silicontrip.net/~mark/lavtools/
 *
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
#include <fcntl.h>

/* time
 10 April 06:58 - 07:
 */

typedef struct
{	
	int fd;
    char *filename;
    
    
    int chromah;
    int chromaw;
    
} valuesContext;

static const AVOption values_options[]= {
    {"filename", "set output file", OFFSET(filename), AV_OPT_TYPE_STRING, {.str=NULL},  CHAR_MIN, CHAR_MAX, FLAGS},
    {"f", "set output file", OFFSET(filename), AV_OPT_TYPE_STRING, {.str=NULL},  CHAR_MIN, CHAR_MAX, FLAGS},
    {NULL},
};

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
    valuesContext *valuesCtx = ctx->priv;
	int argc=0;
	
    
    av_opt_set_defaults(valuesCtx);

    if ((ret = av_set_options_string(valuesCtx, args, "=", ":")) < 0)
        return ret;

    valuesCtx->fd = 1;
    
    if (valuesCtx->filename != NULL)
    {
        valuesCtx->fd = open (valuesCtx->filename,O_WRONLY|O_CREAT,0644);
    }
	return 0;
}

static int query_formats(AVFilterContext *ctx)
{
    // will want more
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
	valuesContext *valuesCtx = ctx->priv;
	AVFilterLink *inlink = outlink->src->inputs[0];

    int hsub, vsub;
    
	avcodec_get_chroma_sub_sample(outlink->format, &hsub, &vsub);
	
	outlink->w = inlink->w;
    outlink->h = inlink->h;
	
    // may need to check that this is correct
    valuesCtx->chromaw = inlink->w >> hsub;
    valuesCtx->chromah = inlink->h >> vsub;

    
	return 0;
	
}

// static void draw_slice(AVFilterLink *link, int y, int h, int slice_dir)
static int filter_frame(AVFilterLink *link, AVFrame *in)
{
   
    valuesContext *valuesCtx = link->dst->priv;
    AVFilterLink *outlink = link->dst->outputs[0];
    AVFrame *out; // = link->dst->outputs[0]->outpic;
	AVFilterContext *ctx = link->src;


    int h = outlink->h;
    
	// Broadcast swing levels, min and max
	int min=16;
	int max=235;
	int luma,chromab,chromar;
	int i,j;
	
    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    av_frame_copy_props(out, in);

		
    for (j=0; j<link->h; j++) {
        for (i=0;i<link->w;i++) {
        }
	}
	
    // should I copy the frame to the output?
    
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
