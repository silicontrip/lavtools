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

int outlier(pixelvalue x, pixelvalue y, pixelvalue z)
{

	int dif;
	
	dif =  ((abs(x - y) + abs (z - y) ) / 2) - abs(z-x);
	
	//fprintf(stderr,"dif: %d %d\n",dif2,dif1);
	
	return (dif > 6) ?1: 0;
	
}


static void filterpixel(uint8_t **o, uint8_t ***p,int chan, int i, int j, int w, int h, int temporalLength) {
	
	unsigned int sum =0;
	unsigned int total = 0;
	int ol=1;
	int z,y,x;
	int pixel_loc;
	
	// Arg! don't want to temporarily allocate this memory.
	
	uint8_t diff[1024];
	uint8_t med[1024];

	pixel_loc = j * w + i;	

	
	if (  (i-1 < 0) || (i+1 > w) || (j-1 < 0) || (j+1 >= h)) {
		o[chan][pixel_loc] = p[1][chan][pixel_loc];
	} else {
	
		for (x=-1; x<2; x++)
		{

			if ((j-2 >=0) || (j+2 < h)) {
				if (!outlier(p[1][chan][(j-2) * w + i+x], p[1][chan][j * w + i+x], p[1][chan][(j+2) * w + i+x]))
					ol = 0;
			}

			
			if (!outlier(p[1][chan][(j-1) * w + i+x], p[1][chan][j * w + i+x], p[1][chan][(j+1) * w + i+x]))
				ol = 0;
			/*
			if (!outlier(p[0][chan][j * w + i+x], p[1][chan][j * w + i+x], p[2][chan][j * w + i+x]))
				ol = 0;
			 */

		}
		if (ol) {
			if ((j-2 >=0) || (j+2 < h)) {
				
				o[chan][pixel_loc] = (p[1][chan][(j-1) * w + i] + p[1][chan][(j+1) * w + i]  + p[1][chan][(j-2) * w + i] + p[1][chan][(j+2) * w + i] ) / 4;
				
			}
			o[chan][pixel_loc] = (p[1][chan][(j-1) * w + i] + p[1][chan][(j+1) * w + i] ) / 2;
			
			//o[chan][pixel_loc] = 235;
			
		} else {
			o[chan][pixel_loc] = p[1][chan][pixel_loc];
		}
	}
	
}

static void filterframe (uint8_t *m[3], uint8_t ***n, y4m_stream_info_t *si,int temporalLength)
{
	
	int x,y;
	int height,width,height2,width2;
	
	// these are stored here for speed
	height=y4m_si_get_plane_height(si,0);
	width=y4m_si_get_plane_width(si,0);
	
	// I'll assume that the chroma subsampling is the same for both u and v channels
	height2=y4m_si_get_plane_height(si,1);
	width2=y4m_si_get_plane_width(si,1);
	
	
	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
					
			filterpixel(m,n,0,x,y,width,height,temporalLength);
			
			if (x<width2 && y<height2) {
				filterpixel(m,n,1,x,y,width2,height2,temporalLength);
				filterpixel(m,n,2,x,y,width2,height2,temporalLength);
			}
			
		}
	}
	
}

// temporal filter loop
static void filter(  int fdIn ,int fdOut , y4m_stream_info_t  *inStrInfo, int temporalLength )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            ***yuv_data;
	uint8_t				*yuv_odata[3];
	uint8_t				**temp_yuv;
	int                read_error_code ;
	int                write_error_code ;
	int c,d;
	
	// Allocate memory for the YUV channels
	// may move these off into utility functions
	if (chromalloc(yuv_odata,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	// sanity check!!!
	if(temporalalloc(&yuv_data,inStrInfo,3))
		mjpeg_error_exit1 ("Could'nt allocate memory for the temporal YUV4MPEG data!");

	
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[temporalLength-1] );
	
	// what can I call this utility function
	//init loop mode
	for (c=temporalLength-1;c>0;c--) {
		chromacpy(yuv_data[c-1],yuv_data[c],inStrInfo);
	}
	
	// if temporalLength is an odd value should it be rounded up or down
	for(d=1;d<temporalLength/2;d++) {
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[temporalLength-1] );
		// shuffle
		temporalshuffle(yuv_data,temporalLength);
	}
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			filterframe(yuv_odata,yuv_data,inStrInfo,temporalLength);
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
	const static char *legal_flags = "?hv:l:";
	
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
			case 'l':
				length = atoi(optarg);
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
	
	// Information output
	mjpeg_info ("yuvtemporal (version " VERSION ") is a temporal filter for yuv streams");
	mjpeg_info ("(C) 2010 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info (" -h for help");
	
	
	/* in that function we do all the important work */
	//filterinitialize ();
	y4m_write_stream_header(fdOut,&in_streaminfo);

	filter(fdIn, fdOut, &in_streaminfo,3);
	
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
