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

#include "libavutil/opt.h"


#include <strings.h>
#include <fcntl.h>

/* time
 10 April 06:58 - 07:38
 
 */

typedef struct
{
    const AVClass *class;

    
	FILE  *fh;
    char *filename;
    
    int chromah;
    int chromaw;
    int fc;
    
    int fs;
    int cfs;
    
} valuesContext;



#define OFFSET(x) offsetof(valuesContext, x)

static const AVOption values_options[]= {
    {"filename", "set output file", OFFSET(filename), AV_OPT_TYPE_STRING, {.str=NULL},  CHAR_MIN, CHAR_MAX},
    {"f", "set output file", OFFSET(filename), AV_OPT_TYPE_STRING, {.str=NULL},  CHAR_MIN, CHAR_MAX},
    {NULL}
};

AVFILTER_DEFINE_CLASS(values);


// static av_cold int init(AVFilterContext *ctx, const char *args, void *opaque)
static av_cold int init(AVFilterContext *ctx, const char *args)
{
    valuesContext *values = ctx->priv;
    int ret;

    av_log(ctx, AV_LOG_DEBUG, ">>> init().\n");

    values->class = &values_class;

    av_opt_set_defaults(values);

    av_log(ctx, AV_LOG_DEBUG, "    init() av_set_options_string.\n");

    
    if ((ret = av_set_options_string(values, args, "=", ":")) < 0)
        return ret;

    values->fc = 0;
    values->fh = stdout;
    
    if (values->filename != NULL)
    {
        values->fh = fopen (values->filename,"w");
    }
    
    av_log(ctx, AV_LOG_DEBUG, "<<< init().\n");

    
	return 0;
}

static av_cold void uninit(AVFilterContext *ctx)
{
    valuesContext *values = ctx->priv;
    fclose(values->fh);
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
	valuesContext *values = ctx->priv;
	AVFilterLink *inlink = outlink->src->inputs[0];

    int hsub, vsub;
    
	avcodec_get_chroma_sub_sample(outlink->format, &hsub, &vsub);
	
	outlink->w = inlink->w;
    outlink->h = inlink->h;
	
    // may need to check that this is correct
    values->chromaw = inlink->w >> hsub;
    values->chromah = inlink->h >> vsub;

    values->fs = inlink->w * inlink->h;
    values->cfs = values->chromaw * values->chromah;
    
	return 0;
	
}

// static void draw_slice(AVFilterLink *link, int y, int h, int slice_dir)
static int filter_frame(AVFilterLink *link, AVFrame *in)
{
   
    valuesContext *values = link->dst->priv;
    AVFilterLink *outlink = link->dst->outputs[0];
    AVFrame *out; // = link->dst->outputs[0]->outpic;
    
	int i,j;
	int cw,w;
    int yuv;
    
    
	int miny,minu,minv;
	int toty=0,totu=0,totv=0;
	int maxy,maxu,maxv;

    
    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    av_frame_copy_props(out, in);

    miny = in->data[0][0];
	maxy = in->data[0][0];
	
	minu = in->data[1][0];
	maxu = in->data[1][0];
	
	minv = in->data[2][0];
	maxv = in->data[2][0];

    cw = 0; w = 0;
    for (j=0; j<link->h; j++) {
        for (i=0;i<link->w;i++) {
           
            yuv = in->data[0][w+i];

            if (yuv > maxy) maxy=yuv;
			if (yuv < miny) miny=yuv;
			toty += yuv;

            if (i<values->chromaw && j<values->chromah) {
				yuv = in->data[1][cw+i];
				if (yuv > maxu) maxu=yuv;
				if (yuv < minu) minu=yuv;
				totu += yuv;
				
				yuv = in->data[2][cw+i];
				if (yuv > maxv) maxv=yuv;
				if (yuv < minv) minv=yuv;
				totv += yuv;
			}

            
        }
        
        w += in->linesize[0];
        cw += in->linesize[1];
	}
	
    // should I copy the frame to the output?
    
    fprintf(values->fh,"%d %d %g %d %d %g %d %d %g %d\n",values->fc,
            miny,1.0 * toty / values->fs, maxy,
            minu,1.0 * totu / values->cfs, maxu,
            minv,1.0 * totv / values->cfs, maxv);
    
    values->fc++;
	av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}

static const AVFilterPad avfilter_vf_values_inputs[] = {
    {
        .name            = "default",
        .type            = AVMEDIA_TYPE_VIDEO,
        .min_perms       = AV_PERM_READ,
        .filter_frame = filter_frame,
        
    },
    { NULL }
};

static const AVFilterPad avfilter_vf_values_outputs[] = {
    {
        .name            = "default",
        .config_props     = config_props,
        .type            = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};


AVFilter avfilter_vf_values = {
    .name      = "values",
	.description = ".",
	
	.init = init,
	.uninit    = uninit,
    .query_formats = query_formats,
	
	.priv_size = sizeof(valuesContext),
	
    .inputs    = avfilter_vf_values_inputs,
    .outputs   = avfilter_vf_values_outputs,
    .priv_class = &values_class,

};
