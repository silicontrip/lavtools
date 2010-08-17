/*
  *  yuvyshot.c
  * attempts to remove temporal shot noise.
  *
  *  modified from yuvhsync.c by
  *  Copyright (C) 2010 Mark Heath
  *  http://silicontrip.net/~mark/lavtools/
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
  
gcc -O3 -L/sw/lib -I/sw/include/mjpegtools -lmjpegutils utilyuv.o yuvhsync.c -o yuvhsync
ess: gcc -O3 -L/usr/local/lib -I/usr/local/include/mjpegtools -lmjpegutils utilyuv.o yuvhsync.c -o yuvhsync

  */

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

#define YUVRFPS_VERSION "0.3"

static void print_usage() 
{
  fprintf (stderr,
	   "usage: yuvtshot -m <mode>\n"
		"\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
		   "\t -m modes:\n\t 0: forward and backward pixels\n"
		   "\t 1: forward, backward, left and right pixels\n"
		   "\t 2: forward, backward, interlace up and down pixels\n"
		   "\t 3: forward, backward, left, right interlace up and down pixels\n"
			"\t 4: forward, backward up and down pixels\n"
		);
}

int median (int *a,int l) {
	
	int x,y;
	int min,minp;
	
	for(x=0;x<=(l/2);x++){
		minp=x; min = a[x];
		for(y=x;y<l;y++)
			if(a[y]<min) {
				min=a[y];
				minp=y;
			}
		a[minp] = a[x];
		a[x] = min;
	}
	
	min = a[l/2];

	return min;
}

//how easy is it to make this for all planes
get_pixel(int x, int y, int plane, uint8_t *m[3],y4m_stream_info_t *si)
{

	int w,h;

	h = y4m_si_get_plane_height(si,plane);
	w = y4m_si_get_plane_width(si,plane);

	if (x < 0) {x=0;}
	if (x >= w) {x=w-1;}
	if (y < 0) {y=0;}
	if (y >= h) {y=h-1;}
	
	return 	*(m[plane]+x+y*w);
	
}

void clean (uint8_t *l[3],uint8_t *m[3], uint8_t *n[3],y4m_stream_info_t *si,int t,int t1)
{

	int w,h,x,y,le;
	int cw,ch;
	int pix[7];
	int dif,diff,difa,difb;
	
	h = y4m_si_get_plane_height(si,t1);
	w = y4m_si_get_plane_width(si,t1);

	ch = y4m_si_get_plane_height(si,t1);
	cw = y4m_si_get_plane_width(si,t1);

	
	for(y=0;y<h;y++) {
		for (x=0;x<w;x++) {
			
			// do some case around here.
			
			switch (t) {
				case 0:
					pix[0] = get_pixel(x,y,t1,l,si);
					pix[1] = get_pixel(x,y,t1,m,si);
					pix[2] = get_pixel(x,y,t1,n,si);
					le=3;
					break;
				case 1:
					pix[0] = get_pixel(x,y,t1,l,si);
					pix[1] = get_pixel(x-1,y,t1,m,si);
					pix[2] = get_pixel(x,y,t1,m,si);
					pix[3] = get_pixel(x+1,y,t1,m,si);
					pix[4] = get_pixel(x,y,t1,n,si);
					le=5;
					break;
				case 2:
					pix[0] = get_pixel(x,y,t1,l,si);
					pix[1] = get_pixel(x,y-2,t1,m,si);
					pix[2] = get_pixel(x,y,t1,m,si);
					pix[3] = get_pixel(x,y+2,t1,m,si);
					pix[4] = get_pixel(x,y,t1,n,si);
					le=5;
					break;
				case 3:
					pix[0] = get_pixel(x,y,t1,l,si);
					pix[1] = get_pixel(x-1,y,t1,m,si);
					pix[2] = get_pixel(x,y-2,t1,m,si); // 2 for interlace
					pix[3] = get_pixel(x,y,t1,m,si);
					pix[4] = get_pixel(x,y+2,t1,m,si); // 2 for interlace
					pix[5] = get_pixel(x+1,y,t1,m,si);
					pix[6] = get_pixel(x,y,t1,n,si);
					le=7;
					break;
				case 4:
					pix[0] = get_pixel(x,y,t1,l,si);
					pix[1] = get_pixel(x,y-1,t1,m,si);
					pix[2] = get_pixel(x,y,t1,m,si);
					pix[3] = get_pixel(x,y+1,t1,m,si);
					pix[4] = get_pixel(x,y,t1,n,si);
					le=5;
					break;
					

			}
			*(m[t1]+x+y*w) = median(pix,le);
			
			/*
			//simple median filter.
			if ((pixa < pix && pix < pixb) || (pixb < pix && pix < pixa)) { 
				;
			}
			
			if ((pixa < pixb && pixb < pix) || (pix < pixb && pixb < pixa)) { 
				*(m[0]+x+y*w)= pixb;
			}
			
			if ((pix < pixa && pixa < pixb) || (pixb < pixa && pixa < pix)) { 
				*(m[0]+x+y*w)= pixa;
			}
			*/

		}
	}
		
}

static void process(  int fdIn , y4m_stream_info_t  *inStrInfo,
	int fdOut, y4m_stream_info_t  *outStrInfo,
	int t,int t1)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3][3];	
	uint8_t				*yuv_tdata[3];
	int                read_error_code  = Y4M_OK;
	int                write_error_code = Y4M_OK ;
	int                src_frame_counter ;
	int x,y,w,h,cw,ch;

	h = y4m_si_get_plane_height(inStrInfo,0);
	ch = y4m_si_get_plane_height(inStrInfo,1);

	chromalloc(yuv_data[0],inStrInfo);
	chromalloc(yuv_data[1],inStrInfo);
	chromalloc(yuv_data[2],inStrInfo);

	//fill with black
	chromaset(yuv_data[2],inStrInfo,16,128,128);

// initialise and read the first number of frames
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[1]);
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[0]);

	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {

		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,0);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,1);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,2);

		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[1] );
		y4m_fini_frame_info( &in_frame );
		
		// nothing to see here, move along, move along
		// it might be quicker to move the pointers.
	//	chromacpy(yuv_data[2],yuv_data[1],inStrInfo);
	//	chromacpy(yuv_data[1],yuv_data[0],inStrInfo);
		
		// as long as there are 3 planes
		yuv_tdata[0] = yuv_data[2][0];
		yuv_tdata[1] = yuv_data[2][1];
		yuv_tdata[2] = yuv_data[2][2];
		
		yuv_data[2][0] = yuv_data[1][0];
		yuv_data[2][1] = yuv_data[1][1];
		yuv_data[2][2] = yuv_data[1][2];
		
		yuv_data[1][0] = yuv_data[0][0];
		yuv_data[1][1] = yuv_data[0][1];
		yuv_data[1][2] = yuv_data[0][2];
		
		yuv_data[0][0] = yuv_tdata[0];
		yuv_data[0][1] = yuv_tdata[1];
		yuv_data[0][2] = yuv_tdata[2];

		
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[0] );
		++src_frame_counter ;
	}

	// still have 1 frame to clean and write.
	
	if( read_error_code == Y4M_ERR_EOF ) {
		chromaset(yuv_data[0],inStrInfo,16,128,128);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,0);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,1);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,2);
	
		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[1] );	
	}
  // Clean-up regardless an error happened or not

	y4m_fini_frame_info( &in_frame );

	chromafree( yuv_data[0] );
	chromafree( yuv_data[1] );
	chromafree( yuv_data[2] );
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	if( write_error_code != Y4M_OK )
		mjpeg_error_exit1 ("Error writing output stream!");

}

// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{

	int verbose = 1 ; // LOG_ERROR ?
	int drop_frames = 0;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo,out_streaminfo;
	int src_interlacing = Y4M_UNKNOWN;
	y4m_ratio_t src_frame_rate;
	const static char *legal_flags = "v:m:s:";
	int max_shift = 0, search = 0;
	int c;

  while ((c = getopt (argc, argv, legal_flags)) != -1) {
    switch (c) {
      case 'v':
        verbose = atoi (optarg);
        if (verbose < 0 || verbose > 2)
          mjpeg_error_exit1 ("Verbose level must be [0..2]");
        break;
    case 'm':
	    max_shift = atof(optarg);
		break;
	case 's':
		search = atof(optarg);
		break;
	case '?':
          print_usage (argv);
          return 0 ;
          break;
    }
  }
  
  
  // mjpeg tools global initialisations
  mjpeg_default_handler_verbosity (verbose);

  // Initialize input streams
  y4m_init_stream_info (&in_streaminfo);
  y4m_init_stream_info (&out_streaminfo);

  // ***************************************************************
  // Get video stream informations (size, framerate, interlacing, aspect ratio).
  // The streaminfo structure is filled in
  // ***************************************************************
  // INPUT comes from stdin, we check for a correct file header
	if (y4m_read_stream_header (fdIn, &in_streaminfo) != Y4M_OK)
		mjpeg_error_exit1 ("Could'nt read YUV4MPEG header!");

	src_frame_rate = y4m_si_get_framerate( &in_streaminfo );
	y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
	

  // Information output
  mjpeg_info ("yuvadjust (version " YUVRFPS_VERSION ") is a simple luma and chroma adjustments for yuv streams");
  mjpeg_info ("yuvadjust -? for help");

  /* in that function we do all the important work */
	y4m_write_stream_header(fdOut,&out_streaminfo);

	process( fdIn,&in_streaminfo,fdOut,&out_streaminfo,max_shift,search);

  y4m_fini_stream_info (&in_streaminfo);
  y4m_fini_stream_info (&out_streaminfo);

  return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
