/*
 *  yuvfieldrev.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
 *
 *  based on code:
 *  Copyright (C) 2002 Alfonso Garcia-Patiño Barbolani <barbolani at jazzfree.com>
 *
** <h3> Field reversing filter </h3>
** <p> Works by delaying the fields by 1 field.  May also require audio adjustment to keep the audio in sync</p>
** <p> I've noticed that some broadcast DVBT material is field reversed progressive material.  This filter can
** return the content back to progressive.</p>
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


#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "utilyuv.h"

#define VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvfieldrev\n"
			);
}


static void filter(  int fdIn ,int fdOut  , y4m_stream_info_t  *inStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	uint8_t            *yuv_odata[3] ;

	int interlace;
	int                read_error_code ;
	int                write_error_code ;
	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Couldn't allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,inStrInfo))		
		mjpeg_error_exit1 ("Couldn't allocate memory for the YUV4MPEG data!");

	// fill with black.
	chromaset(yuv_odata,inStrInfo,16,128,128);
	
	/* Initialize counters */
	
	interlace = y4m_si_get_interlace(inStrInfo);
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			copyfield(yuv_odata,yuv_data,inStrInfo,interlace);
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );
			copyfield(yuv_odata,yuv_data,inStrInfo,invert_order(interlace));

		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	}
	chromaset(yuv_data,inStrInfo,16,128,128);
	copyfield(yuv_odata,yuv_data,inStrInfo,interlace);
	write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );

	
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
	int top_field =0, bottom_field = 0,double_height=1;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	y4m_ratio_t frame_rate;
	int interlaced,ilace=0,pro_chroma=0,yuv_interlacing= Y4M_UNKNOWN;
	int height;
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
	mjpeg_info ("yuv (version " VERSION ") is a field reversing utility for yuv streams");
	mjpeg_info ("(C)  Mark Heath <mjpeg0 at silicontrip.org>");	
	
	
	y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
	y4m_si_set_interlace(&out_streaminfo, invert_order(y4m_si_get_interlace(&in_streaminfo)));
	y4m_write_stream_header(fdOut,&out_streaminfo);
	
	/* in that function we do all the important work */
	filter(fdIn, fdOut, &in_streaminfo);
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
