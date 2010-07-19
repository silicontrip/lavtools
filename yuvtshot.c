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
	   "usage: yuvadjust -t <thresh>\n"
		"\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
		);
}

void clean (uint8_t *l[3],uint8_t *m[3], uint8_t *n[3],y4m_stream_info_t *si)
{

	int w,h,x,y;
	int pix,pixa,pixb;
	int dif,diff,difa,difb;
	
	h = y4m_si_get_plane_height(si,0);
	w = y4m_si_get_plane_width(si,0);

	for(y=0;y<h;y++) {
		for (x=0;x<w;x++) {
			pixa = *(l[0]+x+y*w);
			pixb = *(n[0]+x+y*w);
			pix = *(m[0]+x+y*w);
			
			diff=abs(pixa-pixb);
			difa=abs(pixa-pix);
			difb=abs(pixb-pix);
			
			dif = (difa + difb) / 2;
			
			if (dif > 32 && diff < 8) {
				
				*(m[0]+x+y*w) =  (pixa + pixb) / 2;
	//			*(m[0]+x+y*w) =  0;
			}
		}
	}
	
	//will look at chroma later...
	
}

static void process(  int fdIn , y4m_stream_info_t  *inStrInfo,
	int fdOut, y4m_stream_info_t  *outStrInfo,
	int max,int search)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3][3];	
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

		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo);
		
		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[1] );
		y4m_fini_frame_info( &in_frame );
		
		// nothing to see here, move along, move along
		// it might be quicker to move the pointers.
		chromacpy(yuv_data[2],yuv_data[1],inStrInfo);
		chromacpy(yuv_data[1],yuv_data[0],inStrInfo);
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[0] );
		++src_frame_counter ;
	}

	// still have 1 frame to clean and write.
	
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
