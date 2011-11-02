/*
 *  generic.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/

 ** <h4> yuv values </h4>
 ** <p> prints timecode and min/average/max of the yuv channels for each frame</p>
 ** <p> -d to use NTSC drop frame timecode</p>

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

#define VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvvalues [-d]\n"
			 "\t-d use NTSC drop frame timecode\n"
			);
}

// This function stolen from yuvdiff

void luma_sum_diff  (int *bri, int *bro, 	uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t  *in)
{
	int l,x;
	int fds,w;
	
	//	fprintf(stderr,"trace: luma_sum_diff\n");
	
	
	*bri = 0; *bro=0;
	
	w = y4m_si_get_width(in);
	fds = y4m_si_get_plane_length(in,0);
	// only comparing Luma, less noise, more resolution... blah blah
	
	//fprintf(stderr,"trace: luma_sum_diff w=%d fds=%d\n",w,fds);
	
	
	for (l=0; l< fds; l+=w<<1) {
		for (x=0; x<w; x++) {
			*bri += abs(m[0][l+x]-n[0][l+x]);
			*bro += abs(m[0][l+x+w]-n[0][l+x+w]);
		}
	}
}



static void filterframe (uint8_t *m[3], y4m_stream_info_t *si, int fc,int df, int diff)
{
	
	int x,y;
	int height,width,height2,width2;
	int cw,w;
	int yuv;
	
	int tch,tcm,tcs,tcf;
	
	int miny,minu,minv;
	int toty=0,totu=0,totv=0;
	int maxy,maxu,maxv;
	
	int fs,cfs;
	
	
	height=y4m_si_get_plane_height(si,0);
	width=y4m_si_get_plane_width(si,0);
	
	// I'll assume that the chroma subsampling is the same for both u and v channels
	height2=y4m_si_get_plane_height(si,1);
	width2=y4m_si_get_plane_width(si,1);
	
	fs = y4m_si_get_plane_length(si,0);
	cfs = y4m_si_get_plane_length(si,1);


	miny = m[0][0];
	maxy = m[0][0];
	
	minu = m[1][0];
	maxu = m[1][0];
	
	minv = m[2][0];
	maxv = m[2][0];
	
	cw =0; w = 0;
	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			
			yuv = m[0][w+x];
			if (yuv > maxy) maxy=yuv;
			if (yuv < miny) miny=yuv;
			toty += yuv;
			
			if (x<width2 && y<height2) {
				yuv = m[1][cw+x];
				if (yuv > maxu) maxu=yuv;
				if (yuv < minu) minu=yuv;
				totu += yuv;
				
				yuv = m[2][cw+x];
				if (yuv > maxv) maxv=yuv;
				if (yuv < minv) minv=yuv;
				totv += yuv;
			}
			
		}

		cw += width2;
		w += width;
	}
	
	framecount2timecode(si, &tch,&tcm,&tcs,&tcf, fc, &df);
	
	printf ("%d %02d:%02d:%02d%c%02d %g %d %g %d %d %g %d %d %g %d\n",
			fc,tch,tcm,tcs,df?';':':',tcf,
			1.0*diff/fs,
			miny,1.0*toty/fs,maxy,
			minu,1.0*totu/cfs,maxu,
			minv,1.0*totv/cfs,maxv);
	
}


static void filter(  int fdIn  , y4m_stream_info_t  *inStrInfo, int df )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3], *yuv_odata[3] ;
	uint8_t *yuv_tdata;
	int                read_error_code ;
	int                write_error_code ;
	int odd_diff,even_diff;
	int frame_count = 1;
	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	chromaset(yuv_odata,inStrInfo,16,128,128); // set the compare frame to black.
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			luma_sum_diff (&odd_diff,&even_diff,yuv_data,yuv_odata,inStrInfo);
			filterframe(yuv_data,inStrInfo,frame_count,df,odd_diff+even_diff);
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );

		// swap the double buffer
		yuv_tdata = yuv_odata[0];  yuv_odata[0] = yuv_data[0]; yuv_data[0] = yuv_tdata;
		yuv_tdata = yuv_odata[1];  yuv_odata[1] = yuv_data[1]; yuv_data[1] = yuv_tdata;
		yuv_tdata = yuv_odata[2];  yuv_odata[2] = yuv_data[2]; yuv_data[2] = yuv_tdata;
		
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		frame_count ++;
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
	
	int verbose = 1; // LOG_DEBUG ;
	int top_field =0, bottom_field = 0,double_height=1;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	y4m_ratio_t frame_rate;
	int interlaced,ilace=0,pro_chroma=0,yuv_interlacing= Y4M_UNKNOWN;
	int height;
	int c ;
	int drop_frame=0;
	const static char *legal_flags = "hv:d";
	
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
				break;
			case 'd':
				drop_frame=1;
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
	//mjpeg_info ("yuv (version " VERSION ") is a general deinterlace/interlace utility for yuv streams");
	//mjpeg_info ("(C)  Mark Heath <mjpeg0 at silicontrip.org>");
	// mjpeg_info ("yuvcropdetect -h for help");
	
    
	// y4m_write_stream_header(fdOut,&in_streaminfo);
	/* in that function we do all the important work */
	filter(fdIn, &in_streaminfo,drop_frame);
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
