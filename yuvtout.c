/*
 *  generic.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
 *
**<h3>YUV VHS shot noise filter</h3>
**<p> This is what the yuvtshot filter is meant to do for VHS speck noise</p>
**<p> This is a spatial filter which detects and removes the white "speck" noise
** from VHS tapes.</p>
**<p> I have spent a while tuning the detection algorithm, it appears quite effective.
** This filter is more useful at removing VHS noise than the tshot filter.  </p>
**<p>Added a detect only feature which outputs the frame number and 0-1.0 pixel ratio.</p>
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


#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include "utilyuv.h"

#define VERSION "0.1"

#define PRECISION 1024



static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvtout [-hvdt <threshold> ]\n"
			 "\t-d detect only, output frame vs %% of outlier pixels\n"
			 "\t-t threshold, higher values mean less detected. defaults to 4\n"
			 "\t-h help -v verbose <0-2>"
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
	
	// Will make this configurable by command line option.
	return dif>4?1:0;
	
}

static int detectpixel(uint8_t **p,int chan, int i, int j, int w, int h) {
	
	
	if ((i-1 < 0) || (i+1 > w) || (j-1 < 0) || (j+1 >= h)) {
		return 0;
	} else {
		int x;		
		for (x=-1; x<2; x++)
		{
			// detect two pixels above and below (to eliminate interlace artefacts)
			if ((j-2 >=0) && (j+2 < h)) {
				if (!outlier(p[chan][(j-2) * w + i+x], p[chan][j * w + i+x], p[chan][(j+2) * w + i+x]))
					return 0;
			}
			
			if (!outlier(p[chan][(j-1) * w + i+x], p[chan][j * w + i+x], p[chan][(j+1) * w + i+x]))
				return 0;
			
		}
	}
	return 1;
	
}


static void filterpixel(uint8_t **o, uint8_t **p,int chan, int i, int j, int w, int h) {
	
	int pixel_loc;
		
	pixel_loc = j * w + i;	
	
	
	// interpolate
	if (detectpixel(p,chan, i, j,w,h)) {
		unsigned int total = 0;

		total = (p[chan][(j-1) * w + i] + p[chan][(j+1) * w + i] ) / 2;
		
		if ((j-2 >=0) || (j+2 < h)) {
			// interlace
			int x;
			x = ( p[chan][(j-2) * w + i] + p[chan][(j+2) * w + i]) / 2;
			total = (x + total) / 2;
		}
		
		o[chan][pixel_loc] = total;
		
		// test mode for marking
		// o[chan][pixel_loc] = 235;
		
	} else {
		// just copy the pixel
		o[chan][pixel_loc] = p[chan][pixel_loc];
	}
	
}

static void filterframe (uint8_t *m[3], uint8_t **n, y4m_stream_info_t *si)
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
			
			filterpixel(m,n,0,x,y,width,height);
			
			if (x<width2 && y<height2) {
				filterpixel(m,n,1,x,y,width2,height2);
				filterpixel(m,n,2,x,y,width2,height2);
			}
			
		}
	}
	
}


static double detectframe (uint8_t **n, y4m_stream_info_t *si)
{
	
	int x,y;
	int height,width,height2,width2;
	
	int total;
	
	int count = 0; 
	
	
	// these are stored here for speed
	height=y4m_si_get_plane_height(si,0);
	width=y4m_si_get_plane_width(si,0);
	
	// I'll assume that the chroma subsampling is the same for both u and v channels
	height2=y4m_si_get_plane_height(si,1);
	width2=y4m_si_get_plane_width(si,1);
	
	total = height * width + height2 * width2 + height2 * width2;
	
	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			
			count += detectpixel(n,0,x,y,width,height);
			
			if (x<width2 && y<height2) {
				count += detectpixel(n,1,x,y,width2,height2);
				count += detectpixel(n,2,x,y,width2,height2);
			}
			
		}
	}
	
	return 1.0 * count / total;
}



// temporal filter loop
static void filter(  int fdIn ,int fdOut , y4m_stream_info_t  *inStrInfo , int thresh, int detect)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	uint8_t				**temp_yuv;
	int                read_error_code ;
	int                write_error_code ;
	unsigned int counter = 0;
	
	// Allocate memory for the YUV channels
	// may move these off into utility functions
	if(!detect) {
		if (chromalloc(yuv_odata,inStrInfo))		
			mjpeg_error_exit1 ("Could'nt allocate memory for the out YUV4MPEG data!");
	}
	// sanity check!!!
	if(chromalloc(yuv_data,inStrInfo))
		mjpeg_error_exit1 ("Could'nt allocate memory for the in YUV4MPEG data!");
	
	
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	read_error_code = Y4M_OK;

	//y4m_init_frame_info( &in_frame );
	//read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		
		// do work
		if (read_error_code == Y4M_OK) {
			if (detect) {
				printf ("%d %g\n",counter++,detectframe(yuv_data,inStrInfo));
			} else {
				filterframe(yuv_odata,yuv_data,inStrInfo);
				write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );

			}
		}
		
		y4m_fini_frame_info( &in_frame );
				
	}
	
	// Clean-up regardless an error happened or not
	
	
//	y4m_fini_frame_info( &in_frame );
	if (!detect) {
		chromafree(yuv_odata);
	}
	
	//must free yuv temporal buffer
	chromafree(yuv_data);
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR ;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo ;
	int c ;
	int threshold = 4, detect = 0;
	const static char *legal_flags = "?hv:t:d";
	
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
			case 't':
				threshold = atoi(optarg);
				break;
			case 'd':
				detect = 1;
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
	mjpeg_info ("yuvtout (version " VERSION ") is a VHS noise reduction filter for yuv streams");
	mjpeg_info ("(C) 2011 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info (" -h for help");
	
	/* in that function we do all the important work */
	//filterinitialize ();
	// sorry for this conditional logic
	if (!detect) {
		y4m_write_stream_header(fdOut,&in_streaminfo);
	}
	
	filter(fdIn, fdOut, &in_streaminfo,threshold,detect);
	
	if (!detect) {
		y4m_fini_stream_info (&in_streaminfo);
	}
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
