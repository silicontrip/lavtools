/*
 *  yuvdiag.c
 *  diagnostics  2008 Mark Heath <mjpeg at silicontrip.org>
 *  Technical display routines 
 *  Converts a YUV stream for technical purposes.
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
gcc -O3 yuvdiag.c -L/sw/lib -I/sw/include/mjpegtools -lmjpegutils -o yuvdiag
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

#define YUVDI_VERSION "0.5"

static void print_usage() 
{
	fprintf (stderr,
		"usage: yuvdiag [-v] [-ycl] [-h]\n"
		"yuvdiag converts the yuvstream for technical viewing\n"
		"\n"
		"\t -y copy yuv channels into the luma channel\n"
		"\t -c chroma scope\n" 
		"\t -l luma scope\n" 
		"\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
		"\t -h print this help\n"
	);
}

// Allocate a uint8_t frame
int chromalloc(uint8_t *m[3],y4m_stream_info_t *sinfo)
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	m[0] = (uint8_t *)malloc( fs );
	m[1] = (uint8_t *)malloc( cfs);
	m[2] = (uint8_t *)malloc( cfs);
	
	if( !m[0] || !m[1] || !m[2]) {
		return -1;
	} else {
		return 0;
	}
	
}

//Copy a uint8_t frame
int chromacpy(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo)
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	memcpy (m[0],n[0],fs);
	memcpy (m[1],n[1],cfs);
	memcpy (m[2],n[2],cfs);
	
}

// returns the opposite field ordering
int invert_order(int f)
{
	switch (f) {
			
		case Y4M_ILACE_TOP_FIRST:
			return Y4M_ILACE_BOTTOM_FIRST;
		case Y4M_ILACE_BOTTOM_FIRST:
			return Y4M_ILACE_TOP_FIRST;
		case Y4M_ILACE_NONE:
			return Y4M_ILACE_NONE;
		default:
			return Y4M_UNKNOWN;
	}
}

chromaset(uint8_t *m[], y4m_stream_info_t  *sinfo, int y, int u, int v )
{

        int fs,cfs;
        
        fs = y4m_si_get_plane_length(sinfo,0);
        cfs = y4m_si_get_plane_length(sinfo,1);

		memset (m[0],y,fs);
		memset (m[1],u,cfs);
		memset (m[2],v,cfs);

}


/**************************************************************************************
** generic function to give the chroma pixel coordinate from the luma pixel coordinate
***************************************************************************************/
void chroma_coord (y4m_stream_info_t  *sinfo, int *cx, int *cy, int lx, int ly) {

	int rheight, rwidth;
	
	
	rheight = y4m_si_get_plane_height(sinfo,0) / y4m_si_get_plane_height(sinfo,1);
	rwidth = y4m_si_get_plane_width(sinfo,0) / y4m_si_get_plane_width(sinfo,1);

	*cx = lx * y4m_si_get_plane_width(sinfo,1) / y4m_si_get_plane_width(sinfo,0);

	if (rheight == 1) {
		*cy = ly;
	} else if (rheight == 2) {
		if (y4m_si_get_interlace(sinfo) == Y4M_ILACE_NONE) {
				*cy = ly >> 1;
		} else {
				*cy = ((ly >> 2) << 1) + (ly%2);
		}
	} else {
		// I've never seen anything other than 1 or 2
		mjpeg_error_exit1("Chroma height ratio not 1 or 2"); 
	}
	

	
}

// Would love to make this a generic vector image to overlay

void draw_luma (uint8_t *m[], y4m_stream_info_t  *sinfo) 
{

	int x,y,y1,x1,height,width,cwidth;
	int cx, cy;

	// fprintf(stderr,"draw_luma: enter\n");


	height = y4m_si_get_plane_height(sinfo,0);	
	width = y4m_si_get_plane_width(sinfo,0);	
	cwidth = y4m_si_get_plane_width(sinfo,1);	

	
	for (y1=0 ; y1 < 256; y1+=8) {
	
		// fprintf(stderr,"draw_luma: y1=%d\n",y1);

	
		y = (height -1) - y1;
	
		for (x1=0; x1 < 8; x1++) {
			
			x = x1;
			chroma_coord(sinfo, &cx, &cy, x, y);
			m[0][y * width + x] = 240;
			m[1][cy * cwidth + cx] = 240;
			m[2][cy * cwidth + cx] = 128;

			x = (width - 1) - x1;
			chroma_coord(sinfo, &cx, &cy, x, y);
			m[0][y * width + x] = 240;
			m[1][cy * cwidth + cx] = 240;
			m[2][cy * cwidth + cx] = 128;

		
		}
	
	}
		// fprintf(stderr,"draw_luma: exit\n");


}

static void channel(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,height,width;
	int oheight,owidth;
	int y;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	cheight = y4m_si_get_plane_height(inStrInfo,1);
	cwidth = y4m_si_get_plane_width(inStrInfo,1);

	width = y4m_si_get_plane_width(inStrInfo,0);
	height = y4m_si_get_plane_height(inStrInfo,0);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
				chromaset (yuv_odata,outStrInfo,16,128,128);
			
			// this makes assumptions that we are in 420
			
			for (y=0; y< cheight; y++) {
				// luma copy
				memcpy(yuv_odata[0] + y * owidth,  yuv_data[0] + y * width, width);
				memcpy(yuv_odata[0] + (y + cheight) * owidth,  yuv_data[0] + (y + cheight) * width, width);

				
				memcpy(yuv_odata[0] + y * owidth + width, yuv_data[1] + y * cwidth, cwidth);
				memcpy(yuv_odata[0] + (y + cheight) * owidth + width, yuv_data[2] + y * cwidth, cwidth);
				
			}
			
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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

static void chroma_scope(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,ocheight,ocwidth;
	int oheight,owidth;
	int y,x;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	cheight = y4m_si_get_plane_height(inStrInfo,1);
	cwidth = y4m_si_get_plane_width(inStrInfo,1);

	ocwidth = y4m_si_get_plane_width(outStrInfo,1);
	ocheight = y4m_si_get_plane_height(outStrInfo,1);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			chromaset (yuv_odata,outStrInfo,16,128,128);
			
			for (x=0; x< cwidth; x++) 
				for (y=0; y< cheight; y++) {
					// fprintf (stderr,"U: %d V: %d\n",yuv_data[1][y*cwidth+x],yuv_data[2][y*cwidth+x]);
					/* if ( y< ocheight && x < ocwidth) {
						yuv_odata[1][y*ocwidth+x] = x*2;
						yuv_odata[2][y*ocwidth+x] = y*2;
					}
					*/
					if (yuv_odata[0][yuv_data[1][y*cwidth+x] + yuv_data[2][y*cwidth+x] * owidth] < 255) 
					yuv_odata[0][yuv_data[1][y*cwidth+x] + yuv_data[2][y*cwidth+x] * owidth] ++;
				}
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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

static void luma_scope(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,height,width;
	int oheight,owidth;
	int y,x;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	cheight = y4m_si_get_plane_height(inStrInfo,1);
	cwidth = y4m_si_get_plane_width(inStrInfo,1);

	width = y4m_si_get_plane_width(inStrInfo,0);
	height = y4m_si_get_plane_height(inStrInfo,0);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			chromaset (yuv_odata,outStrInfo,16,128,128);
			chromacpy (yuv_odata,yuv_data,inStrInfo);
			
			for (x=0; x< width; x++) 
				for (y=0; y< height; y++) {
					// fprintf (stderr,"U: %d V: %d\n",yuv_data[1][y*cwidth+x],yuv_data[2][y*cwidth+x]);
					if ( yuv_odata[0][((oheight-1) -  yuv_data[0][y*width+x] ) * owidth + x] < 255 )
						yuv_odata[0][((oheight-1) -  yuv_data[0][y*width+x] ) * owidth + x] +=2;
				}
				draw_luma(yuv_odata,outStrInfo);
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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

void acc_hist(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,height,width;
	int oheight,owidth;
	int y,x;
	int hist[256],max,choice;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	width = y4m_si_get_plane_width(inStrInfo,0);
	height = y4m_si_get_plane_height(inStrInfo,0);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	for (x=0; x<256; x++) 
		hist[x] = 0;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			chromaset (yuv_odata,outStrInfo,16,128,128);
			// chromacpy (yuv_odata,yuv_data,inStrInfo);
			
			for (x=0; x< width; x++) 
				for (y=0; y< height; y++) {
					// fprintf (stderr,"U: %d V: %d\n",yuv_data[1][y*cwidth+x],yuv_data[2][y*cwidth+x]);
					hist[yuv_data[0][y*width+x]] ++;
				}
			max = hist[0]; choice = 0;
			for (x=0; x< owidth; x++) {
				if (hist[x]>max) {
					max = hist[x];
					choice = x;
				}
			}
		//	mjpeg_debug("max: %d choice %d",max,choice);
			for (x=0; x<owidth; x++) {
				for (y=0; y < (255.0 * hist[x]) / max; y++)
					yuv_odata[0][(oheight-y) * owidth + x] = 224;
			}
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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


#define MODE_CHANNEL 0
#define MODE_CHROMA 1
#define MODE_LUMA 2
#define MODE_HIST 3


// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR ;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	int width, mode, c;
	const static char *legal_flags = "vyiclh";
	
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
				case 'i':
					mode = MODE_HIST;
					break;
				case 'y':
					mode = MODE_CHANNEL;
					break;
				case 'c':
					mode = MODE_CHROMA;
					break;
				case 'l':
					mode = MODE_LUMA;
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
	mjpeg_info ("yuvdiag (version " YUVDI_VERSION ") is a video diagnostic for yuv streams");
	mjpeg_info ("(C) 2008 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info ("yuvdiag -h for help");
	
	y4m_init_stream_info (&out_streaminfo);
	y4m_copy_stream_info(&out_streaminfo, &in_streaminfo);

	
	/* in that function we do all the important work */
	if (mode == MODE_CHANNEL) {
		y4m_si_set_width (&out_streaminfo, y4m_si_get_plane_width(&in_streaminfo,1) + y4m_si_get_plane_width(&in_streaminfo,0));
		
		y4m_write_stream_header(fdOut,&out_streaminfo);
		channel(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
		
	}
	if (mode == MODE_CHROMA) {
		y4m_si_set_width (&out_streaminfo,256);
		y4m_si_set_height (&out_streaminfo,256);

		y4m_write_stream_header(fdOut,&out_streaminfo);
		chroma_scope(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
	}
	
	if (mode == MODE_LUMA) {
		y4m_si_set_height (&out_streaminfo,256 + y4m_si_get_plane_height(&in_streaminfo,0));

		y4m_write_stream_header(fdOut,&out_streaminfo);
		luma_scope(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
	}

	if (mode == MODE_HIST) {
		y4m_si_set_width (&out_streaminfo,256);
		y4m_si_set_height (&out_streaminfo,256);

		y4m_write_stream_header(fdOut,&out_streaminfo);
		acc_hist(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
	}
	
	
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
