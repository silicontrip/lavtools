/*
 *  yuvpulldowndetect.c
 *  produces a 2d graph showing time vs frame difference.
 *  the idea is that 3:2 pulldown has a distinct frame duplication pattern
 *  .
 *  modified from yuvfps.c by
 *  Copyright (C) 2002 Alfonso Garcia-Patiño Barbolani
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"

#define YUVFPS_VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvdiff [-g -v -h -Ip|b|p]\n"
			 "yuvdiff produces a 2d graph showing time vs frame difference\n"
			 "\n"
			 "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
			 "\t -I<pbt> Force interlace mode\n"
			 "\t -h print this help\n"
			 );
}

static void detect(  int fdIn, int fdOut , y4m_stream_info_t  *inStrInfo, y4m_stream_info_t *outStrInfo ,int interlacing,int graph)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;	
	uint8_t            *yuv_odata[3] ;
	uint8_t            *yuv_wdata[3] ;
	
	uint8_t            *yuv_tdata ;
	
	int                frame_data_size ;
	int                read_error_code ;
	int                write_error_code ;
	int                src_frame_counter ;
	int w,h,x,cw,ch;
	int bri=0, bro=0,l=0;
	
	// Allocate memory for the YUV channels
	
	h = y4m_si_get_height(inStrInfo); 
	w = y4m_si_get_width(inStrInfo);
	
	cw = y4m_si_get_plane_width(inStrInfo,1);
	ch = y4m_si_get_plane_height(inStrInfo,1);
	
	
	frame_data_size = y4m_si_get_plane_length(inStrInfo,0);
	
	
	
	chromalloc(yuv_data,inStrInfo);
	chromalloc(yuv_odata,inStrInfo);
	chromalloc(yuv_wdata,inStrInfo);
	
	
	if( !yuv_data[0] || !yuv_data[1] || !yuv_data[2] ||
	   !yuv_odata[0] || !yuv_odata[1] || !yuv_odata[2]  )
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	
	write_error_code = Y4M_OK ;
	
	src_frame_counter = 0 ;
	y4m_init_frame_info( &in_frame );
	
	
	
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_odata );
	++src_frame_counter ;
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		
		// check for read error
		
		
		// perform frame difference.
		// only comparing Luma, less noise, more resolution... blah blah
		
		bri = 0; bro=0;
		for (l=0; l< frame_data_size; l+=w<<1) {
			for (x=0; x<w; x++) {
				bri += abs(yuv_data[0][l+x]-yuv_odata[0][l+x]);
				bro += abs(yuv_data[0][l+x+w]-yuv_odata[0][l+x+w]);
				
				yuv_wdata[0][l+x] = (yuv_data[0][l+x]-yuv_odata[0][l+x]) + 128 ;
				yuv_wdata[0][l+x+w] = (yuv_data[0][l+x+w]-yuv_odata[0][l+x+w]) + 128;
			}
		}
		// so much for optimizing only luma
		for (l=0; l<ch; l++) {
			for (x=0;x<cw;x++){
				yuv_wdata[1][l*cw+x] = (yuv_data[1][l*cw+x] - yuv_odata[1][l*cw+x]) + 128;
				yuv_wdata[2][l*cw+x] =  (yuv_data[2][l*cw+x] - yuv_odata[2][l*cw+x]) +128;
			}
		}
		
		if (graph) {
			if (interlacing == Y4M_ILACE_NONE) {
				printf ("%d %d\n",src_frame_counter,bri+bro);
			} else if (interlacing == Y4M_ILACE_TOP_FIRST) {
				printf ("%d %d\n",src_frame_counter,bri);
				printf ("%d.5 %d\n",src_frame_counter,bro);
			} else if (interlacing = Y4M_ILACE_BOTTOM_FIRST) {
				printf ("%d %d\n",src_frame_counter,bro);
				printf ("%d.5 %d\n",src_frame_counter,bri);
			}
		} else {
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_wdata );
		}
		
		++src_frame_counter ;
		
		yuv_tdata = yuv_odata[0];  yuv_odata[0] = yuv_data[0]; yuv_data[0] = yuv_tdata;
		yuv_tdata = yuv_odata[1];  yuv_odata[1] = yuv_data[1]; yuv_data[1] = yuv_tdata;
		yuv_tdata = yuv_odata[2];  yuv_odata[2] = yuv_data[2]; yuv_data[2] = yuv_tdata;
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		
    }
	
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	chromafree( yuv_data );
	chromafree( yuv_odata );
	chromafree( yuv_wdata );
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	if( write_error_code != Y4M_OK )
		mjpeg_error_exit1 ("Error writing output stream!");
	
}

static int parse_interlacing(char *str)
{
	if (str[0] != '\0' && str[1] == '\0')
	{
		switch (str[0])
		{
			case 'p':
				return Y4M_ILACE_NONE;
			case 't':
				return Y4M_ILACE_TOP_FIRST;
			case 'b':
				return Y4M_ILACE_BOTTOM_FIRST;
		}
	}
	mjpeg_error_exit1("Valid interlacing modes are: p - progressive, t - top-field first, b - bottom-field first");
	return Y4M_UNKNOWN; /* to avoid compiler warnings */
}


// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR ;
	int fdIn = 0 , fdOut=1;
	y4m_stream_info_t in_streaminfo,out_streaminfo;;
	int src_interlacing = Y4M_UNKNOWN;
	const static char *legal_flags = "gI:v:h";
	int graph = 0;
	int c ;
	
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'g':
				graph = 1;
				break;
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
				break;
			case 'I':
				src_interlacing = parse_interlacing(optarg);
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
	mjpeg_info ("yuvdiff (version " YUVFPS_VERSION
				") is a frame/field doubling detector for yuv streams");
	mjpeg_info ("yuvdiff -h for help");
	
	if (src_interlacing == Y4M_UNKNOWN)
		src_interlacing = y4m_si_get_interlace(&in_streaminfo);
	
	if (!graph) {
		y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
		y4m_write_stream_header(fdOut,&out_streaminfo);
	}
	
	/* in that function we do all the important work */
	detect( fdIn,fdOut,&in_streaminfo,&out_streaminfo, src_interlacing,graph);
	
	y4m_fini_stream_info (&in_streaminfo);
	if (!graph) {
		y4m_fini_stream_info (&out_streaminfo);
	}
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
