/*
 *  yuvpixelgraph.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
 *
** <h3>Pixel Graph</h3>
**<p> produces a text file of the chroma channels of a single pixel.</p>
** <p> I have been using this to analyse some chroma-luma bleeding (rainbow) I've found on commercial DVDs.
** In order to write a filter to remove this rainbow effect.</p>
** <p>The pixel location is hard coded.</p>
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


#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include "utilyuv.h"

#define YUV_VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuv\n"
			);
}


static void filter(  int fdIn  , y4m_stream_info_t  *inStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	int                read_error_code ;
	int                write_error_code ;
	
	int c=0;
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			
			printf ("%d %d %d\n",c++,get_pixel(324,176,1,yuv_data,inStrInfo),get_pixel(324,176,2,yuv_data,inStrInfo));
			//filterframe(yuv_odata,yuv_data,inStrInfo);
			//write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );
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
	const static char *legal_flags = "hv:";
	
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
		}
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
	
	// Information output
	mjpeg_info ("yyuvpixelgraph (version " YUV_VERSION ") is a general deinterlace/interlace utility for yuv streams");
	mjpeg_info ("(C) 2010 Mark Heath <mjpeg0 at silicontrip.org>");
	//mjpeg_info ("yuv -h for help");
	
    
	y4m_write_stream_header(fdOut,&in_streaminfo);
	/* in that function we do all the important work */
	filter(fdIn, &in_streaminfo);
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
