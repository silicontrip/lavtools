/*
 *  generic.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
 *
 *  based on code:
 *  Copyright (C) 2002 Alfonso Garcia-Patiño Barbolani <barbolani at jazzfree.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 gcc yuvdeinterlace.c -I/sw/include/mjpegtools -lmjpegutils  
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "utilyuv.h"

#define VERSION "0.1"

#define PRECISION 1024



static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuv\n"
			 );
}

typedef uint8_t pixelvalue;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); } 
#define PIX_SWAP(a,b) { pixelvalue temp=(a);(a)=(b);(b)=temp; } 

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMAX(a,b) ((a) < (b) ? (b) : (a))
#define FFABS(a) ((a) > 0 ? (a) : (-(a)))

#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)


static void filter_line_c(int p_mode, uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int refs, int parity)
{
    int x;
    uint8_t *prev2= parity ? prev : cur ;
    uint8_t *next2= parity ? cur  : next;
    for(x=0; x<w; x++){
        int c= cur[-refs];
        int d= (prev2[0] + next2[0])>>1;
        int e= cur[+refs];
        int temporal_diff0= FFABS(prev2[0] - next2[0]);
        int temporal_diff1=( FFABS(prev[-refs] - c) + FFABS(prev[+refs] - e) )>>1;
        int temporal_diff2=( FFABS(next[-refs] - c) + FFABS(next[+refs] - e) )>>1;
        int diff= FFMAX3(temporal_diff0>>1, temporal_diff1, temporal_diff2);
        int spatial_pred= (c+e)>>1;
        int spatial_score= FFABS(cur[-refs-1] - cur[+refs-1]) + FFABS(c-e)
		+ FFABS(cur[-refs+1] - cur[+refs+1]) - 1;
		
#define CHECK(j)\
{   int score= FFABS(cur[-refs-1+j] - cur[+refs-1-j])\
+ FFABS(cur[-refs  +j] - cur[+refs  -j])\
+ FFABS(cur[-refs+1+j] - cur[+refs+1-j]);\
if(score < spatial_score){\
spatial_score= score;\
spatial_pred= (cur[-refs  +j] + cur[+refs  -j])>>1;\

	CHECK(-1) CHECK(-2) }} }}
CHECK( 1) CHECK( 2) }} }}

if(p_mode<2){
	int b= (prev2[-2*refs] + next2[-2*refs])>>1;
	int f= (prev2[+2*refs] + next2[+2*refs])>>1;
#if 0
	int a= cur[-3*refs];
	int g= cur[+3*refs];
	int max= FFMAX3(d-e, d-c, FFMIN3(FFMAX(b-c,f-e),FFMAX(b-c,b-a),FFMAX(f-g,f-e)) );
	int min= FFMIN3(d-e, d-c, FFMAX3(FFMIN(b-c,f-e),FFMIN(b-c,b-a),FFMIN(f-g,f-e)) );
#else
	int max= FFMAX3(d-e, d-c, FFMIN(b-c, f-e));
	int min= FFMIN3(d-e, d-c, FFMAX(b-c, f-e));
#endif
	
	diff= FFMAX3(diff, min, -max);
}

if(spatial_pred > d + diff)
spatial_pred = d + diff;
else if(spatial_pred < d - diff)
spatial_pred = d - diff;

dst[0] = spatial_pred;

dst++;
cur++;
prev++;
next++;
prev2++;
next2++;
}
}


static void filterframe (uint8_t *dst[3], uint8_t ***ref, y4m_stream_info_t *si, int yuv_interlacing)
{
	
	int x,y;
	int height,width,height2,width2;
	
	uint8_t *prev, *cur, *next, *dst2;
	
	// constants. should make command line args
	int mode,parity;
	
	parity = 1;
	mode = 1;
	
	int tff = yuv_interlacing == Y4M_ILACE_TOP_FIRST?1:0;
	
	// these are stored here for speed
	height=y4m_si_get_plane_height(si,0);
	width=y4m_si_get_plane_width(si,0);
	
	// I'll assume that the chroma subsampling is the same for both u and v channels
	height2=y4m_si_get_plane_height(si,1);
	width2=y4m_si_get_plane_width(si,1);
	
	
	
	for (y=0; y < height; y++) {
					
		if((y ^ parity) & 1){

			prev= &ref[0][0][y*width];
			cur = &ref[1][0][y*width];
			next= &ref[2][0][y*width];
			dst2= &dst[0][y*width];
			
			
			filter_line_c(mode,dst2, prev, cur, next,width,width,parity ^ tff);
			
			if (y<height2) {
				
				prev= &ref[0][1][y*width2];
				cur = &ref[1][1][y*width2];
				next= &ref[2][1][y*width2];
				dst2= &dst[1][y*width2];
				
				
				filter_line_c(mode,dst2, prev, cur, next,width2,width2,parity ^ tff);
				
				prev= &ref[0][2][y*width2];
				cur = &ref[1][2][y*width2];
				next= &ref[2][2][y*width2];
				dst2= &dst[2][y*width2];
				
				
				filter_line_c(mode,dst2, prev, cur, next,width2,width2,parity ^ tff);

			}
		} else {
			memcpy(&dst[0][y*width], &ref[1][0][y*width], width);
			if (y<height2) {
				memcpy(&dst[1][y*width2], &ref[1][1][y*width2], width2);
				memcpy(&dst[2][y*width2], &ref[1][2][y*width2], width2);
			}
		}
	}
	
}

// temporal filter loop
static void filter(  int fdIn ,int fdOut , y4m_stream_info_t  *inStrInfo, int yuv_interlacing )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            ***yuv_data;
	uint8_t				*yuv_odata[3];
	uint8_t				**temp_yuv;
	int                read_error_code ;
	int                write_error_code ;
	int c,d;
	
	int temporalLength = 3;
	
	// Allocate memory for the YUV channels
	// may move these off into utility functions
	if (chromalloc(yuv_odata,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (temporalalloc(&yuv_data,inStrInfo,temporalLength)) 
		mjpeg_error_exit1 ("Could'nt allocate memory for the temporal data!");
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[temporalLength-1] );
	
	
	//init loop mode
	for (c=temporalLength-1;c>0;c--) {
		chromacpy(yuv_data[c-1],yuv_data[c],inStrInfo);
	}
	
	// if temporalLength is an odd value should it be rounded up or down
	for(d=1;d<temporalLength/2;d++) {
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		// makes assumption that the video is longer than temporalLength frames.
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[temporalLength-1] );		
		
		temporalshuffle(yuv_data,temporalLength);
		
	}
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			filterframe(yuv_odata,yuv_data,inStrInfo,yuv_interlacing);
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[temporalLength-1] );
		// shuffle
		temporalshuffle(yuv_data,temporalLength);
	}
	// finalise loop		
	for(c=0;c<temporalLength/2;c++) {
		if (read_error_code == Y4M_OK) {
			filterframe(yuv_odata,yuv_data,inStrInfo,temporalLength);
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		
		//need to copy frame.
		
		// shuffle
		temporalshuffle(yuv_data,temporalLength);
	}
	
	
	// Clean-up regardless an error happened or not
	
	
	y4m_fini_frame_info( &in_frame );
	chromafree(yuv_odata);
	
	//must free yuv temporal buffer
	temporalfree(yuv_data,temporalLength);
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR ;
	int length=1;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	y4m_ratio_t frame_rate;
	int interlaced,ilace=0,pro_chroma=0,yuv_interlacing= Y4M_UNKNOWN;
	int height;
	float sigma;
	int c ;
	const static char *legal_flags = "?hv:I:";
	
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
				break;
				
			case 'h':
			case '?':
				print_usage (argv);
				return 0 ;
				break;
			case 'I':
				switch (optarg[0]) {
					case 't':  yuv_interlacing = Y4M_ILACE_TOP_FIRST;  break;
					case 'b':  yuv_interlacing = Y4M_ILACE_BOTTOM_FIRST;  break;
					default:
						mjpeg_error("Unknown value for interlace: '%c'", optarg[0]);
						return -1;
						break;
				}
				break;
		}
	}
	
	y4m_accept_extensions(1);
	
	// mjpeg tools global initialisations
	mjpeg_default_handler_verbosity (verbose);
	
	// Initialize input streams
	y4m_init_stream_info (&in_streaminfo);
	
	// ***************************************************************
	// Get video stream informations (size, framerate, interlacing, aspect ratio).
	// The streaminfo structure is filled in
	// ***************************************************************
	// INPUT comes from stdin, we check for a correct file header
	if (y4m_read_stream_header (fdIn, &in_streaminfo) != Y4M_OK)
		mjpeg_error_exit1 ("Could'nt read YUV4MPEG header!");
	
	if (yuv_interlacing==Y4M_UNKNOWN) {
		yuv_interlacing = y4m_si_get_interlace (&in_streaminfo);
	}
	
	if (yuv_interlacing==Y4M_UNKNOWN || yuv_interlacing == Y4M_ILACE_NONE) {
		mjpeg_error_exit1("source material not interlaced");
	}
	
	y4m_si_set_interlace(&in_streaminfo, Y4M_ILACE_NONE);
	y4m_write_stream_header(fdOut,&in_streaminfo);

	
	// Information output
	mjpeg_info ("yuvyadif (version " VERSION ") is an implementation of the yadif filter for yuv streams");
	mjpeg_info ("(C) 2011 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info (" -h for help");
	
	
	/* in that function we do all the important work */
	//filterinitialize ();
	
	filter(fdIn, fdOut, &in_streaminfo,yuv_interlacing);
	
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
