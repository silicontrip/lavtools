/*
 *  yuvdiff.c
 *  produces a 2d graph showing time vs frame difference.
 *  the idea is that 3:2 pulldown has a distinct frame duplication pattern
 *  .
 *  modified from yuvfps.c by
 *  Copyright (C) 2002 Alfonso Garcia-Patiño Barbolani
 *
 
 ** <h3>Frame rate conversion detection</h3>
 ** 
 ** <p> Started life similar to yuvaddetect, may even contain disused
 ** code from ad detect.  This program produces an ASCII file suitable
 ** for plotting in gnuplot of the difference between consecutive frames
 ** or fields.  Suitable for detecting 3-2 pulldown, or any other
 ** pulldown (frame duplication) frame rate conversion.  If the output
 ** is ran through an FFT it can even detect other frequency rate
 ** changes such as 2.39 (25 to 59.94, progressive PAL to interlaced
 ** NTSC) </p>
 ** 
 ** <p> This program cannot remove pulldown.  Try my other program
 ** yuvrfps for frame duplication removal.  I do not know of a way to
 ** remove frame blended frame rate conversion.  </p>
 **
 ** <p> I have used this extensively to detect pulldown material.  </p>
 **
 ** <p>A single comparison frame can be used to compare all frames 
 ** in a stream.  Useful for attempting to find the comparison frame in the stream</p>
 
 
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

#include <fcntl.h>


#include "yuv4mpeg.h"
#include "mpegconsts.h"

#define YUVFPS_VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvdiff [-g -v -h -Ip|b|p]\n"
			 "yuvdiff produces a video showing frame by frame difference\n"
			 "Or specify a file to compare differences from the first frame of that file\n"
			 "\n"
			 "\t -g produce text output suitable for graphing in gnuplot\n"
			 "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
			 "\t -I<pbt> Force interlace mode\n"
			 "\t -i <file> a y4m stream to compare with\n"
			 "\t -h print this help\n"
			 );
}


void luma_sum_diff  (int *bri, int *bro, 	uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t  *in)
{
	int l,x;
	int fds,w;
	
	*bri = 0; *bro=0;
	
	w = y4m_si_get_width(in);
	fds = y4m_si_get_plane_length(in,0);
	// only comparing Luma, less noise, more resolution... blah blah
	
	for (l=0; l< fds; l+=w<<1) {
		for (x=0; x<w; x++) {
			bri += abs(m[0][l+x]-n[0][l+x]);
			bro += abs(m[0][l+x+w]-n[0][l+x+w]);
		}
	}
}

void diff (uint8_t *m[3],uint8_t *n[3],uint8_t *o[3], y4m_stream_info_t  *in)
{
	int l,x,w,cw,ch,frame_data_size;
	
	w = y4m_si_get_width(in);
	
	cw = y4m_si_get_plane_width(in,1);
	ch = y4m_si_get_plane_height(in,1);
	
	frame_data_size = y4m_si_get_plane_length(in,0);
	
	
	for (l=0; l< frame_data_size; l+=w<<1) {
		for (x=0; x<w; x++) {		
			m[0][l+x] = (n[0][l+x]-o[0][l+x]) + 128 ;
			m[0][l+x+w] = (n[0][l+x+w]-o[0][l+x+w]) + 128;
		}
	}
	
	for (l=0; l<ch; l++) {
		for (x=0;x<cw;x++){
			m[1][l*cw+x] = (n[1][l*cw+x] - o[1][l*cw+x]) + 128;
			m[2][l*cw+x] =  (n[2][l*cw+x] - o[2][l*cw+x]) +128;
		}
	}
	
}


static void detect(  int fdIn, int fdOut , y4m_stream_info_t  *inStrInfo, y4m_stream_info_t *outStrInfo ,int interlacing,int graph, uint8_t ***yuv_cdata, int frames)
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
	int w,h,x,cw,ch,n;
	int bri=0, bro=0,l=0;
	int *totali,*totalo;
	
	// Allocate memory for the YUV channels
	
	
	chromalloc(yuv_data,inStrInfo);
	chromalloc(yuv_odata,inStrInfo);
	chromalloc(yuv_wdata,inStrInfo);
	
	if( !yuv_data[0] || !yuv_data[1] || !yuv_data[2] ||
	   !yuv_odata[0] || !yuv_odata[1] || !yuv_odata[2]  )
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	
	
	write_error_code = Y4M_OK ;
	
	src_frame_counter = 0 ;
	y4m_init_frame_info( &in_frame );
	
	
	
	if (frames > 0) {
		totali = (int *) malloc (sizeof(int) * frames);
		totalo = (int *) malloc (sizeof(int) * frames);
		
		if (totali == NULL || totalo ==NULL)
			mjpeg_error_exit1("Cannot allocate memory (totals)");
		
		chromacpy (yuv_odata,yuv_cdata[0],inStrInfo);
	}
	else {
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_odata );
	}
	++src_frame_counter ;
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		
		// check for read error
		
		
		// perform frame difference.
		
		if (graph) {
			n=0;
			
			do {
				luma_sum_diff(&bri,&bro,yuv_data,yuv_odata,inStrInfo);
				if (frames > 0) {
					totali[n] = bri;
					totalo[n] = bro;
					chromacpy (yuv_odata,yuv_cdata[++n],inStrInfo);
				}
			} while (n < frames);
			
			if ( frames == 0) {
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
				if (interlacing == Y4M_ILACE_NONE) {
					printf ("%d ",src_frame_counter);
					for (n=0; n < frames; n++) {
						printf ("%d ",totali[n]+totalo[n]);
					}
					printf ("\n");
				} else if (interlacing == Y4M_ILACE_TOP_FIRST) {
					printf ("%d ",src_frame_counter);
					for (n=0; n < frames; n++) {
						printf ("%d ",totali[n]);
					}
					printf ("\n");
					
					printf ("%d.5 ",src_frame_counter);
					for (n=0; n < frames; n++) {
						printf ("%d ",totalo[n]);
					}
					printf ("\n");
				} else if (interlacing = Y4M_ILACE_BOTTOM_FIRST) {
					printf ("%d ",src_frame_counter);
					for (n=0; n < frames; n++) {
						printf ("%d ",totalo[n]);
					}
					printf ("\n");
					printf ("%d.5 ",src_frame_counter);
					for (n=0; n < frames; n++) {
						printf ("%d ",totali[n]);
					}
					printf ("\n");
				}
			}
			
			
		} else {
			diff(yuv_wdata,yuv_data,yuv_odata,inStrInfo);
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_wdata );
		}
		
		
		++src_frame_counter ;
		
		// do not copy current frame to comparison frame
		// if reading from comparison file.
		if ( frames == 0  ) {
			yuv_tdata = yuv_odata[0];  yuv_odata[0] = yuv_data[0]; yuv_data[0] = yuv_tdata;
			yuv_tdata = yuv_odata[1];  yuv_odata[1] = yuv_data[1]; yuv_data[1] = yuv_tdata;
			yuv_tdata = yuv_odata[2];  yuv_odata[2] = yuv_data[2]; yuv_data[2] = yuv_tdata;
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		
	}
	
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	if (frames > 0) {
		free(totali);
		free(totalo);
	}
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

int read_frame (uint8_t **yuv_frame, char * filename, y4m_stream_info_t in_streaminfo)
{			
	
	y4m_stream_info_t compare_streaminfo;
	y4m_frame_info_t   compare_frame ;
	int fdCompare = 0;
	
	fdCompare = open (filename,O_RDONLY);
	if (fdCompare < 0) 
		return -1; 
	//fprintf(stderr,"y4m_read_stream_header\n");
	
	if (y4m_read_stream_header (fdCompare, &compare_streaminfo) != Y4M_OK)
		return -2;
	
	if (y4m_si_get_framelength(&in_streaminfo) != y4m_si_get_framelength(&compare_streaminfo)) 
		return -3;
	
	
	y4m_init_frame_info( &compare_frame );
	if(y4m_read_frame(fdCompare,&compare_streaminfo,&compare_frame,yuv_frame ) != Y4M_OK) {
		y4m_fini_frame_info( &compare_frame );
		close (fdCompare);
		return -4;
		
	}
	y4m_fini_frame_info( &compare_frame );
	close (fdCompare);
	
	
	
	
}


// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR ;
	int fdIn = 0 , fdOut=1;
	y4m_stream_info_t in_streaminfo,out_streaminfo,compare_streaminfo;
	int src_interlacing = Y4M_UNKNOWN;
	const static char *legal_flags = "gI:v:h";
	int compare_frames = 0;
	int graph = 0;
	int c ;
	
	uint8_t ***yuv_cdata;	
	
	yuv_cdata[0] = NULL;
	
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
	
	/* Process additional command line arguments */ 
	
	// allocate memory for additional match frames
	
	
	
	if (optind < argc)
	{
		
		compare_frames = argc - optind;
		if (temporalalloc(&yuv_cdata,in_streaminfo,compare_frames))
			mjpeg_error_exit1("Cannot allocate memory for comparison frames");
		
		c = 0; 
		while (optind < argc) {
			if (read_frame(yuv_cdata[c++],argv[optind++],in_streaminfo)) {
				perror ("read_frame ");
				mjpeg_error_exit1("error reading comparison frame");
			}
		}
	}
	
	
	
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
	
	if (yuv_cdata[0] != NULL) {
		if (y4m_si_get_framelength(&in_streaminfo) != y4m_si_get_framelength(&compare_streaminfo)) {
			mjpeg_error_exit1 ("Comparison stream frame size doesn't match input stream!");
		}
	}
	
	/* in that function we do all the important work */
	detect( fdIn,fdOut,&in_streaminfo,&out_streaminfo, src_interlacing,graph,yuv_cdata,compare_frames);
	
	y4m_fini_stream_info (&in_streaminfo);
	if (!graph) {
		y4m_fini_stream_info (&out_streaminfo);
	}
	
	if (compare_frames) {
		temporalfree(yuv_cdata,compare_frames);
	}
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
