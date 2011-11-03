
/*
 *  yuv2jpeg.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/

** <p> writes multiple jpeg files from yuvstreams </p>
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
#include <jpeglib.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "utilyuv.h"

#define VERSION "0.1"

/** 

 * put_jpeg_yuv420p_file 
 *      Converts an YUV420P coded image to a jpeg image and writes 
 *      it to an already open file. 
 * 
 * Inputs: 
 * - image is the image in YUV420P format. 
 * - width and height are the dimensions of the image 
 * - quality is the jpeg encoding quality 0-100% 
 * 
 * Output: 
 * - The jpeg is written directly to the file given by the file pointer fp 
 * 
 * Returns nothing 
 */ 
static void put_jpeg_yuv420p_file(FILE *fp, uint8_t *image[3], int width, int height, int quality) 
{ 
    int i, j; 

    JSAMPROW y[16],cb[16],cr[16]; // y[2][5] = color sample of row 2 and pixel column 5; (one plane) 
    JSAMPARRAY data[3]; // t[0][2][5] = color sample 0 of row 2 and column 5 

    struct jpeg_compress_struct cinfo; 
    struct jpeg_error_mgr jerr; 

    data[0] = y; 
    data[1] = cb; 
    data[2] = cr; 

//	fprintf (stderr,"TRACE: put_jpeg_yuv420p_file in\n");

	
    cinfo.err = jpeg_std_error(&jerr);  // Errors get written to stderr 

    jpeg_create_compress(&cinfo); 
    cinfo.image_width = width; 
    cinfo.image_height = height; 
    cinfo.input_components = 3; 
    jpeg_set_defaults(&cinfo); 

    jpeg_set_colorspace(&cinfo, JCS_YCbCr); 

    cinfo.raw_data_in = TRUE; // Supply downsampled data 
#if JPEG_LIB_VERSION >= 70 
#warning using JPEG_LIB_VERSION >= 70 
    cinfo.do_fancy_downsampling = FALSE;  // Fix segfault with v7 
#endif 
    cinfo.comp_info[0].h_samp_factor = 2; 
    cinfo.comp_info[0].v_samp_factor = 2; 
    cinfo.comp_info[1].h_samp_factor = 1; 
    cinfo.comp_info[1].v_samp_factor = 1; 
    cinfo.comp_info[2].h_samp_factor = 1; 
    cinfo.comp_info[2].v_samp_factor = 1; 

    jpeg_set_quality(&cinfo, quality, TRUE); 
    cinfo.dct_method = JDCT_FASTEST; 

    jpeg_stdio_dest(&cinfo, fp);        // Data written to file 
    jpeg_start_compress(&cinfo, TRUE); 

//	fprintf (stderr,"jpeg write for j\n");
	
    for (j = 0; j < height; j += 16) { 
	//	fprintf (stderr,"jpeg write for i\n");
        for (i = 0; i < 16; i++) { 
            y[i] = image[0] + width * (i + j); 
            if (i % 2 == 0) { 
                cb[i / 2] = image[1] + width / 2 * ((i + j) / 2); 
                cr[i / 2] = image[2] + width / 2 * ((i + j) / 2); 
            } 
        } 
	//	fprintf (stderr,"jpeg write raw data\n");

        jpeg_write_raw_data(&cinfo, data, 16); 
	//	fprintf (stderr,"jpeg write raw data finish\n");

    } 

//	fprintf (stderr,"jpeg finish compress\n");

	
    jpeg_finish_compress(&cinfo); 
    jpeg_destroy_compress(&cinfo); 
} 


static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuv2jpeg [-f formatstring] [-q quality]\n"
			);
}

static void filter(  int fdIn  , y4m_stream_info_t  *inStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	int                read_error_code ;
	int                write_error_code ;
	FILE *fh;
	char filename[1024]; // has potential for buffer overflow
	int frame_count=1;

	int width,height;
	
	// to be moved to command line parameters
	char *format = "frame%03d.jpg";
	int qual = 95;
	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	width = y4m_si_get_plane_width(inStrInfo,0);
	height = y4m_si_get_plane_height(inStrInfo,0);
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
		//	fprintf(stderr,"sprintf filename\n");
			sprintf(filename,format,frame_count);
		//	fprintf(stderr,"fopen filename\n");
		//	fprintf(stderr,"filename: %s\n",filename);

			fh = fopen(filename , "w"); // should check for error
		//	fprintf(stderr,"call put_jpeg_yuv420p_file\n");
			
			if (fh != NULL) {
				put_jpeg_yuv420p_file(fh,yuv_data,width,height,qual);
				fclose (fh);
			} else {
				perror ("fopen jpeg file");
			}
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		frame_count++;
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
	mjpeg_info ("yuv (version " VERSION ") is a general deinterlace/interlace utility for yuv streams");
	mjpeg_info ("(C)  Mark Heath <mjpeg0 at silicontrip.org>");
	// mjpeg_info ("yuvcropdetect -h for help");
	
    
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
