/*
 *  yuvfade.c
 
 *  black-fade  2004 Mark Heath <mjpeg at silicontrip.org>
 *  will fade a yuv4 stream to black after a specified number of frames
 
** <h3>Black Fade</h3>
**
** <p> Will fade the video to black after X number of frames. Uses a
** trial and error method of fading to black, could be better.</p>
** 
** <P>Use this program to give a more professional feel to the
** downloaded internet videos for converting</p>
**<pre>
**usage: yuvfps -c Count -f fadeCount [-v -h]
**      -c skip this number of frames
**      -f fade to black for this many frames
**</pre>
 
 *  based on code:
 *  Copyright (C) 2002 Alfonso Garcia-Pati<F1>o Barbolani <barbolani at jazzfree.com>
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
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <yuv4mpeg.h>
#include <mpegconsts.h>

#define YUVFPS_VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvfps -c Count -f fadeCount [-v -h]\n"
			 "yuvfade fades the last f frames of a yuv video stream read from stdin\n"
			 "\n"
			 "\t -c skip this number of frames\n"
			 "\t -f fade to black for this many frames\n"
			 "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
			 "\t -h print this help\n"
			 );
}

static void resample(  int fdIn 
					 , y4m_stream_info_t  *inStrInfo
					 , int fdOut
					 , y4m_stream_info_t  *outStrInfo
					 , int after
					 , int count
					 )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	int                frame_data_size ;
	int                read_error_code ;
	int                write_error_code ;
	int                src_frame_counter ;
	int w,h,x,y;
	float mul,val;
	
	// Allocate memory for the YUV channels
	h = y4m_si_get_height(inStrInfo) ;
	w = y4m_si_get_width(inStrInfo);
	frame_data_size =  w * h;
	yuv_data[0] = (uint8_t *)malloc( frame_data_size );
	yuv_data[1] = (uint8_t *)malloc( frame_data_size >> 2);
	yuv_data[2] = (uint8_t *)malloc( frame_data_size >> 2);
	
	if( !yuv_data[0] || !yuv_data[1] || !yuv_data[2] )
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	src_frame_counter = 0 ;
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );
	++src_frame_counter ;
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		if (src_frame_counter > after) {
			// fade frame
			mul =   1.0 * (after + count - src_frame_counter)  / count;
			if (mul < 0)
				mul = 0;
			for (y=0; y<h; y++)
				for (x=0; x<w; x++)
				{
					val = (yuv_data[0][x+y*w] - 16) * mul + 16;
					yuv_data[0][x+y*w] = val;
					/* I think chroma fading needs to be started after luma fading */
					if ( (mul * 1.5 < 1.0) && (!(y%2) && !(x%2)) )
					{
						val = (yuv_data[1][x/2+y/2*w/2] - 128) * 1.5 * mul + 128;
						yuv_data[1][x/2+y/2*w/2] = val;
						val = (yuv_data[2][x/2+y/2*w/2] - 128) * 1.5 * mul + 128;
						yuv_data[2][x/2+y/2*w/2] = val;
					}
				}
		}
		
		
		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data );
		++src_frame_counter ;
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
	if( write_error_code != Y4M_OK )
		mjpeg_error_exit1 ("Error writing output stream!");
	
}

// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 1 ;
	int count = 0 ;
	int frames = 0;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	y4m_ratio_t  src_frame_rate ;
	
	const static char *legal_flags = "f:c:v:h";
	int c ;
	
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
				break;
			case 'c':
				count =  atoi (optarg);
				break;
			case 'f':
				frames = atoi (optarg);
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
	y4m_init_stream_info (&out_streaminfo);
	
	// ***************************************************************
	// Get video stream informations (size, framerate, interlacing, aspect ratio).
	// The streaminfo structure is filled in
	// ***************************************************************
	// INPUT comes from stdin, we check for a correct file header
	if (y4m_read_stream_header (fdIn, &in_streaminfo) != Y4M_OK)
		mjpeg_error_exit1 ("Could'nt read YUV4MPEG header!");
	
	// Prepare output stream
	src_frame_rate = y4m_si_get_framerate( &in_streaminfo );
	y4m_ratio_t frame_rate = src_frame_rate ;
	y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
	
	
	// Information output
	mjpeg_info ("yuvfade (version " YUVFPS_VERSION
				") is a general black fade utility for yuv streams");
	
	y4m_write_stream_header(fdOut,&out_streaminfo);
    
	/* in that function we do all the important work */
	resample( fdIn,&in_streaminfo,  fdOut,&out_streaminfo,
			 count, frames );
	
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
