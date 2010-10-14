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


#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "utilyuv.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


struct subhead {
	int entries;
	struct subtitle subs[];
};

struct subtitle {
	
	//in frame counts
	int on;
	int off;
//	char *text;
	char text[256]; // for testing
};
	
#define VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuv\n"
			);
}


static void filterframe (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si)
{
	
	int x,y;
	int height,width,height2,width2;
	
	height=y4m_si_get_plane_height(si,0);
	width=y4m_si_get_plane_width(si,0);
	
	// I'll assume that the chroma subsampling is the same for both u and v channels
	height2=y4m_si_get_plane_height(si,1);
	width2=y4m_si_get_plane_width(si,1);
	
	
	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			
			filterpixel(m[0],n[0],x,y,width,height);
			
			if (x<width2 && y<height2) {
				filterpixel(m[1],n[1],x,y,width2,height2);
				filterpixel(m[2],n[2],x,y,width2,height2);
			}
			
		}
	}
	
}


char * get_sub (struct subhead s, int fc) {

	int n;
	
	for (n=0; n < s.entries; n++){
	
		if (s.subs[n].on <= fc && s.subs[n].off >= fc) 
			return s.subs[n].text;
		
	}
	return '\0';
	
}


static void filter(  int fdIn  , y4m_stream_info_t  *inStrInfo, FT_Face     face, struct subhead subs )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	int                read_error_code ;
	int                write_error_code ;
	FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
	FT_UInt       glyph_index;
	int           pen_x, pen_y, n;	
	int framecounter=0;
	char *text;
	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
		
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	framecounter++;
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			text=get_sub(subs,framecounter);
			if (text != '\0') { 
				filterframe(yuv_data,inStrInfo,face,text);
			}
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_data );
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

void read_subs(struct subhead s) 
{

	s.entries =1 ;

// how to identify the number of entries?
	s.subs = (struct subtitle) malloc(sizeof(struct subtitle));
	
	s.on = 0;
	s.off = 150;
	strcpy(s.text,"This is a test string.");
	
	
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
	int height=16;
	int c ;
	const static char *legal_flags = "hv:f:";
	FT_Library  library;
	FT_Face     face;
	struct subhead subs;
	
	if (FT_Init_FreeType(&library)) 
		mjpeg_error_exit1("Cannot initialise the freetype library");
	

	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
				break;
			case 'f':
				c = FT_New_Face( library,
								optarg,
								0,
								&face );
				if ( c == FT_Err_Unknown_File_Format )
				{
					mjpeg_error_exit1("Do not recognise the font file");
				}
				else if ( c )
				{
					mjpeg_error_exit1("Error reading the font file");
				}
				break;
			case 's':
				height = atoi(optarg);
				break;
			case 'h':
			case '?':
				print_usage (argv);
				return 0 ;
				break;
		}
	}
	
	FT_Set_Pixel_Sizes( face,		/* handle to face object */
						0,			/* pixel_width           */
						height );   /* pixel_height          */
	
	
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
	mjpeg_info ("yuvsubtitle (version " VERSION ") is a subtitle rendering utility for yuv streams");
	mjpeg_info ("(C)  Mark Heath <mjpeg0 at silicontrip.org>");
	// mjpeg_info ("yuvcropdetect -h for help");
	
    
	y4m_write_stream_header(fdOut,&in_streaminfo);
	
	// read the subtitle file
	read_subs(subs);
	
	/* in that function we do all the important work */
	filter(fdIn, &in_streaminfo,face,subs);
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
