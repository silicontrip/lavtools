/*
 *  yuvcrop.c
 *  detect/crop/matte  2004 Mark Heath <mjpeg at silicontrip.org>
 *  detects colour matting and then remove it. 
 *  Either by removing the pixels or painting them with a solid colour.

**<h3>Crop, Matte and Matte detection</h3>
**<p>
**Used to detect matting in yuv sources.
**It can also be used to crop and matte a yuv video stream.</p>
**
**<p>
**Cropping makes the destination video frame dimensions smaller, while matting will keep the
**video the same size, but replace pixels with a solid colour.
**</P>
**
**<h4>RESULTS</h4>
**
**<? news::imageinline("lavtools/yuvcrop",-1); ?>
**
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
 * gcc yuvcrop.c -I/sw/include/mjpegtools -L/sw/lib -lmjpegutils -o yuvcrop
 */


// minimum number of pixels needed to detect interlace.
#define DEFAULT_TOLERANCE 6


#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include "utilyuv.h"

#define YUVDE_VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvcrop [-v] [-c|-m -a <x1,y1-x2,y2>] [-C <y,u,v>] [-T <tolerance>] [-h]\n"
			 "yuvcrop automatically determines the amount to crop\n"
			 "to remove matting from the video.\n"
			 "\n"
			 "\t -c crop mode.  Remove pixels from video.\n"
			 "\t -m matting mode.  Paint pixels solid colour (Default mode detect).\n"
			 "\t -C colour.  Use this colour for matting or detection.\n"
			 "\t -a area. x1,y1-x2,y2 can be taken from the detection output.\n"
			 "\t -T detection tolerance. Higher means more pixels match.\n"
			 "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
			 "\t -h print this help\n"
			 );
}


static void detect(int fdIn  , y4m_stream_info_t  *inStrInfo, uint8_t *col, int tol )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	uint8_t				point[3];
	int                read_error_code ;
	int frame = 0;
	int width,height;	
	int cwidth;	
	int atop=0, abottom=0, aleft=0, aright=0;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	/* Initialize counters */
		
	fprintf(stderr,"detecting...\n");
	
	height = y4m_si_get_plane_height(inStrInfo,0) ; width = y4m_si_get_plane_width(inStrInfo,0);
	//cheight = y4m_si_get_plane_height(inStrInfo,1) ; 
	cwidth = y4m_si_get_plane_width(inStrInfo,1);

	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );

	abottom=height; 
	aright=width;	
	
	while( Y4M_ERR_EOF != read_error_code ) {
		int top=height, left=width;

		frame ++;
		if (read_error_code == Y4M_OK) {
			// do work
			int bottom=0, right=0;
			int x;
			int y;
			
			// find the top and bottom crop
			for (y=0; y<height; y++) {
				int cy = y / 2;
				int distance = 0;
				
				for (x=0; x< width; x++ ) {
					
					int cx = x / 2;
					
					point[0]=yuv_data[0][y*width+x];
					point[1]=yuv_data[1][cy*cwidth+cx];
					point[2]=yuv_data[2][cy*cwidth+cx];
					
					distance += abs(point[0]-col[0]) + abs(point[1]-col[1]) + abs(point[2]-col[2]);
					
				}
				if (distance / width > tol) {
					bottom = y;
					if (y < top) {
						top = y;
					}
				}
			}
			
			// find the left and right crop
			
			for (x=0; x<width; x++) {
				int cx = x / 2;
				int distance = 0;
				
				for (y=0; y<height; y++ ) {
					
					int cy = y / 2;
					
					point[0]=yuv_data[0][y*width+x];
					point[1]=yuv_data[1][cy*cwidth+cx];
					point[2]=yuv_data[2][cy*cwidth+cx];
					
					distance += abs(point[0]-col[0]) + abs(point[1]-col[1]) + abs(point[2]-col[2]);
				}
				if (distance / height > tol) {
					right = x;
					if (x < left) {
						left = x;
					}
				}
			}
			atop += top;
			abottom += bottom;
			aleft += left;
			aright += right;
			fprintf (stderr,"%d,%d-%d,%d : ",aleft/frame,atop/frame,aright/frame,abottom/frame);
			fprintf (stderr,"%d,%d-%d,%d\n",left,top,right,bottom);
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	}
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

static void copy_subframe (uint8_t *dst[3], uint8_t *src[3], y4m_stream_info_t *sinfo,
						   y4m_stream_info_t *sonfo,
						   unsigned int *a) 
{

	int y,h,w;
	int cw,ch;
	int ow,oh,ocw,och;
	
	int cx,cy;
	
	w = y4m_si_get_plane_width(sinfo,0);
	h = y4m_si_get_plane_height(sinfo,0);
	cw = y4m_si_get_plane_width(sinfo,1);
	ch = y4m_si_get_plane_height(sinfo,1);
	
	ow = y4m_si_get_plane_width(sonfo,0);
	oh = y4m_si_get_plane_height(sonfo,0);
	ocw = y4m_si_get_plane_width(sonfo,1);
	och = y4m_si_get_plane_height(sonfo,1);

	
	cx = a[0]/(ow/ocw);
	cy = a[1]/(oh/och);
	
	for (y=0; y<h; y++) {
		
		memcpy(dst[0]+y*w,src[0]+a[0]+(y+a[1])*ow,w);
		if (y<ch) {
			memcpy(dst[1]+y*cw,src[1]+cx+(y+cy)*ocw,cw);
			memcpy(dst[2]+y*cw,src[2]+cx+(y+cy)*ocw,cw);
		}
	}


}

static void crop (int fdIn, y4m_stream_info_t  *inStrInfo,
					int fdOut, y4m_stream_info_t  *outStrInfo,
					unsigned int *a) 
{

	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	uint8_t            *yuv_odata[3] ;
	int                read_error_code ;
	int                write_error_code ;


	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	write_error_code = Y4M_OK ;

	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );

	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {

		copy_subframe(yuv_odata,yuv_data,outStrInfo,inStrInfo,a);

		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		
	}

}

static void paint_matte ( uint8_t *src[3], y4m_stream_info_t *sinfo, unsigned int *a, uint8_t *col)
{

	int y,h,w;
	int cw,ch;	
	int cy;
	int dh,clw,crw,crx;
	
	w = y4m_si_get_plane_width(sinfo,0);
	h = y4m_si_get_plane_height(sinfo,0);
	cw = y4m_si_get_plane_width(sinfo,1);
	ch = y4m_si_get_plane_height(sinfo,1);
	
	dh = h/ch;
	
	//cx = a[0]/(w/cw);
	cy = a[1]/(h/ch);
	
	clw = a[0]/(w/cw);
	crw = cw-a[2]/(w/cw);
	crx = a[2]/(w/cw);
	
	for (y=0; y<h; y++) {
		if ( y<a[1] ) {
			// top
			memset(src[0]+y*w,col[0],w);
			// chroma
			if (y < cy) {
				memset(src[1]+y*cw,col[1],cw);
				memset(src[2]+y*cw,col[2],cw);
			}
		} else if (y>=a[3]) {
			// bottom
			memset(src[0]+y*w,col[0],w);
			if (y%dh) {
				memset(src[1]+(y/dh)*cw,col[1],cw);
				memset(src[2]+(y/dh)*cw,col[2],cw);
			}
			// no need to do the left or right, if we've done top or bottom
		} else {
			// left and right
			// luma
			memset(src[0]+y*w,col[0],a[0]);
			memset(src[0]+a[2]+y*w,col[0],w-a[2]);
			
			if (y%dh) {
				// chroma left
				memset(src[1]+(y/dh)*cw,col[1],clw);
				memset(src[2]+(y/dh)*cw,col[2],clw);
				// and the right
				memset(src[1]+crx+(y/dh)*cw,col[1],crw);
				memset(src[2]+crx+(y/dh)*cw,col[2],crw);	
			}
		}
	}
	
	
}

static void matte (int fdIn, y4m_stream_info_t  *inStrInfo,
					int fdOut, y4m_stream_info_t  *outStrInfo,
					unsigned int *a, uint8_t *col) 
{

	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	uint8_t            *yuv_odata[3] ;
	int                read_error_code ;
	int                write_error_code ;


	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	write_error_code = Y4M_OK ;

	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );

	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {

		chromacpy(yuv_odata,yuv_data,outStrInfo);
		paint_matte(yuv_odata,outStrInfo,a,col);

		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		
	}

}



// *************************************************************************************
// MAIN
// *************************************************************************************

#define MODE_DETECT 0
#define MODE_CROP 1
#define MODE_MATTE 2

#define ARGLEN_AREA 24  // I hope 5 digits per dimension is enough but who knows how large frames might grow... 16384,16384...
#define ARGLEN_COLOUR 12 // 3 colour values, maximum 3 digits each plus commas, this program is only designed to handle 8 bit colour (however 9 bit colour will fit in this string length)

int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR ;
	int fdIn = 0 , fdOut = 1;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	int c, mode = MODE_DETECT;
	uint8_t colour[3];
	unsigned int area[4];
	int tolerance= DEFAULT_TOLERANCE;
	const static char *legal_flags = "cma:C:T:v:h?";
	int i; 

	// default colour (black)
	colour[0]=16;
	colour[1]=128;
	colour[2]=128;
	
	area[0] = 0;
	area[1] = 0;
	area[2] = -1;
	area[3] = -1;
	
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
			break;
			case 'c':
				if (mode) 
					mjpeg_error_exit1 ("Cannot use both crop and matte mode");
				mode = MODE_CROP;
				break;
			case 'm':
				if (mode) 
					mjpeg_error_exit1 ("Cannot use both crop and matte mode");
				mode = MODE_MATTE;
				break;
			case 'a':
				i = sscanf(optarg,"%i,%i-%i,%i",&area[0],&area[1],&area[2],&area[3]);
				if (i != 4)
					mjpeg_error_exit1 ("Area parse error, input doesn't match x1,y1-x2,y2");
				break;
			case 'C':
				i = sscanf(optarg,"%hhi,%hhi,%hhi",&colour[0],&colour[1],&colour[2]);
				if (i != 3)
					mjpeg_error_exit1 ("Colour parse error, input doesn't match y,u,v");
				break;
			case 'T':
					tolerance = atoi(optarg);
			break;
			case 'h':
			case '?':
				print_usage (argv);
				return 0 ;
			break;
		}
	}
		

	if (mode) {
		if ((area[2] == -1) && (area[3] == -1)) 
		{
			mjpeg_error_exit1 ("No area defined for crop or matte.");
		}
		if ((area[2] < area[0]) || (area[3] < area[1])) {
			mjpeg_error_exit1 ("Illegal Area dimensions.");
		}
		// I should also check that the area isn't outside the input streams dimensions
	}
		
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
	
	// setup output streams if mode isn't detect
	if (mode) {

			if (area[0] > y4m_si_get_plane_width(&in_streaminfo,0) ||
				area[2] > y4m_si_get_plane_width(&in_streaminfo,0) ||
				area[1] > y4m_si_get_plane_height(&in_streaminfo,0) ||
				area[3] > y4m_si_get_plane_height(&in_streaminfo,0))
					mjpeg_error_exit1 ("Area outside of streams dimensions!");

			y4m_init_stream_info (&out_streaminfo);
			y4m_copy_stream_info(&out_streaminfo, &in_streaminfo);
			
			// reduce the dimensions
			if (mode == MODE_CROP) {
				y4m_si_set_height (&out_streaminfo, area[3]-area[1]);
				y4m_si_set_width (&out_streaminfo, area[2]-area[0]);
			}
			y4m_write_stream_header(fdOut,&out_streaminfo);
	}
	// Information output
	mjpeg_info ("yuvcropdetect (version " YUVDE_VERSION ") is a general deinterlace/interlace utility for yuv streams");
	mjpeg_info ("(C) 2005 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info ("yuvcropdetect -h for help");
	
	
	
	/* in that function we do all the important work */
	if (mode == MODE_DETECT)
		detect(fdIn, &in_streaminfo,colour,tolerance);
	
	if (mode == MODE_CROP) 
		crop (fdIn,&in_streaminfo, fdOut,&out_streaminfo, area);
		
	if (mode == MODE_MATTE) 
		matte (fdIn,&in_streaminfo, fdOut,&out_streaminfo, area, colour);

	
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
