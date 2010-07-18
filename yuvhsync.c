/*
  *  yuvhsync.c
  * attempts to align horizontal sync drift.
  *
  *  modified from yuvadjust.c by
  *  Copyright (C) 2010 Mark Heath
  *
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
  
gcc -O3 -L/sw/lib -I/sw/include/mjpegtools -lmjpegutils yuvhsync.c -o yuvhsync
ess: gcc -O3 -L/usr/local/lib -I/usr/local/include/mjpegtools -lmjpegutils yuvhsync.c -o yuvhsync

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

#define YUVRFPS_VERSION "0.3"

static void print_usage() 
{
  fprintf (stderr,
	   "usage: yuvadjust -m <max shift> -s <search length>\n"
		"\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
		);
}

void shift_video (int s, int line, uint8_t *yuv_data[3],y4m_stream_info_t *sinfo)
{
	
	int x,w,cw,cs;
	int vss;
	
	w = y4m_si_get_plane_width(sinfo,0);
	cw = y4m_si_get_plane_width(sinfo,1);
	
	vss = y4m_si_get_plane_height(sinfo,0) / y4m_si_get_plane_height(sinfo,1);
	
	cs = s * cw / w;
	
	// could memcpy() do this? 
	for (x=w; x>=s; x--)
		*(yuv_data[0]+x+(line*w))= *(yuv_data[0]+(x-s)+(line*w));

	// one day I should start catering for more or less than 3 planes.
	if (line % vss) {
		for (x=cw; x>=cs; x--) {
			*(yuv_data[1]+x+(line*cw))= *(yuv_data[1]+(x-cs)+(line*cw));
			*(yuv_data[2]+x+(line*cw))= *(yuv_data[2]+(x-cs)+(line*cw));
		}
	}
}

int search_video (int m, int s, int res[], int line, uint8_t *yuv_data[3],y4m_stream_info_t *sinfo) 
{

	int w,h;
	int x1,x2;
	int max,shift,tot;
	h = y4m_si_get_plane_height(sinfo,0);

	// these two should be more than just warnings
	if (s+m > w) {
		mjpeg_warn("search + shift > width");
	}
	
	if (line >= h) {
		mjpeg_warn("line > height");
	}
	shift = 0;
	for (x1=0;x1<m;x1++) 
	{
		tot = 0;
		for(x2=0; x2<s;x2++) 
		{
			tot = tot + abs ( *(yuv_data[0]+x2+(line*w)) - *(yuv_data[0]+x2+x1+(line+1*w)));
		}
		if (res != NULL)
			res[x1]=tot;
		// ok it wasn't max afterall, it was min.
		if (x1==0) max = tot;
		if (tot < max) { max = tot; shift = x1;}
	}
	return shift;
}

void chromalloc(uint8_t *m[3],y4m_stream_info_t *sinfo)
{	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	
	m[0] = (uint8_t *)malloc( fs );
	m[1] = (uint8_t *)malloc( cfs);
	m[2] = (uint8_t *)malloc( cfs);
	
	mjpeg_debug("alloc yuv_data: %x,%x,%x",m[0],m[1],m[2]);
	
	
}


static void process(  int fdIn , y4m_stream_info_t  *inStrInfo,
	int fdOut, y4m_stream_info_t  *outStrInfo,
	int max,int search)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3],*yuv_odata[3];	
	int result[720]; // will change to malloc based on max shift

	int                y_frame_data_size, uv_frame_data_size ;
	int                read_error_code  = Y4M_OK;
	int                write_error_code = Y4M_OK ;
	int                src_frame_counter ;
	int x,y,w,h,cw,ch;

	h = y4m_si_get_plane_height(inStrInfo,0);
	ch = y4m_si_get_plane_height(inStrInfo,1);

	chromalloc(yuv_data,inStrInfo);
	
// initialise and read the first number of frames
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		for (y=0; y<h-1; y++) {
			x = search_video(max,search,result,y,yuv_data,inStrInfo);
			shift_video(x,y,yuv_data,inStrInfo);
			/* graphing this would be nice
			printf ("%d",y);
			for (x=0; x < max; x++)
				printf (", %d",result[x]);
			printf("\n");
			 */
		}
		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data );
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );
		++src_frame_counter ;
	}

  // Clean-up regardless an error happened or not

y4m_fini_frame_info( &in_frame );

		free( yuv_data[0] );
		free( yuv_data[1] );
		free( yuv_data[2] );
	
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
