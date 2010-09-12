/*
  *  yuvyshot.c
  * attempts to remove temporal impulse noise.
  *
  *  modified from yuvhsync.c by
  *  Copyright (C) 2010 Mark Heath
  *  http://silicontrip.net/~mark/lavtools/
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
  
gcc -O3 -L/sw/lib -I/sw/include/mjpegtools -lmjpegutils utilyuv.o yuvhsync.c -o yuvhsync
ess: gcc -O3 -L/usr/local/lib -I/usr/local/include/mjpegtools -lmjpegutils utilyuv.o yuvhsync.c -o yuvhsync

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
#include "utilyuv.h"


typedef uint8_t pixelvalue;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); } 
#define PIX_SWAP(a,b) { pixelvalue temp=(a);(a)=(b);(b)=temp; } 


#define YUVRFPS_VERSION "0.3"

static void print_usage() 
{
  fprintf (stderr,
		   "usage: yuvtshot -m <mode> -v <level> -c -y\n"
		   "\n"
		   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
		   "\t -a process all pixels. Do not adaptively select noise pixels\n"
		   "\t -c process chroma only\n"
		   "\t -y process luma only\n"
		   "\t -m modes: OR'd flags together\n"
		   "\t 1: forward and backward pixels\n"
		   "\t 2: left and right pixels\n"
		   "\t 4: noninterlace up and down pixels\n"
		   "\t 8: interlace up and down pixels\n"
		);
}

int median (int *p,int l) {

	switch (l) {
		case 3:
			PIX_SORT(p[0],p[1]) ; PIX_SORT(p[1],p[2]) ; PIX_SORT(p[0],p[1]) ; 
			return(p[1]) ; 
			break;
		case 5:
			PIX_SORT(p[0],p[1]) ; PIX_SORT(p[3],p[4]) ; PIX_SORT(p[0],p[3]) ; 
			PIX_SORT(p[1],p[4]) ; PIX_SORT(p[1],p[2]) ; PIX_SORT(p[2],p[3]) ; 
			PIX_SORT(p[1],p[2]) ; return(p[2]) ; 
			break;
		case 7:
			PIX_SORT(p[0], p[5]) ; PIX_SORT(p[0], p[3]) ; PIX_SORT(p[1], p[6]) ; 
			PIX_SORT(p[2], p[4]) ; PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[5]) ; 
			PIX_SORT(p[2], p[6]) ; PIX_SORT(p[2], p[3]) ; PIX_SORT(p[3], p[6]) ; 
			PIX_SORT(p[4], p[5]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[1], p[3]) ; 
			PIX_SORT(p[3], p[4]) ; return (p[3]) ; 
			break;
		case 9:
			PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ; 
			PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ; 
			PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ; 
			PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ; 
			PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ; 
			PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ; 
			PIX_SORT(p[4], p[2]) ; return(p[4]) ; 
			break;
		case 25:
			PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[2], p[4]) ; 
			PIX_SORT(p[2], p[3]) ; PIX_SORT(p[6], p[7]) ; PIX_SORT(p[5], p[7]) ; 
			PIX_SORT(p[5], p[6]) ; PIX_SORT(p[9], p[10]) ; PIX_SORT(p[8], p[10]) ; 
			PIX_SORT(p[8], p[9]) ; PIX_SORT(p[12], p[13]) ; PIX_SORT(p[11], p[13]) ; 
			PIX_SORT(p[11], p[12]) ; PIX_SORT(p[15], p[16]) ; PIX_SORT(p[14], p[16]) ; 
			PIX_SORT(p[14], p[15]) ; PIX_SORT(p[18], p[19]) ; PIX_SORT(p[17], p[19]) ; 
			PIX_SORT(p[17], p[18]) ; PIX_SORT(p[21], p[22]) ; PIX_SORT(p[20], p[22]) ; 
			PIX_SORT(p[20], p[21]) ; PIX_SORT(p[23], p[24]) ; PIX_SORT(p[2], p[5]) ; 
			PIX_SORT(p[3], p[6]) ; PIX_SORT(p[0], p[6]) ; PIX_SORT(p[0], p[3]) ; 
			PIX_SORT(p[4], p[7]) ; PIX_SORT(p[1], p[7]) ; PIX_SORT(p[1], p[4]) ;
			PIX_SORT(p[11], p[14]) ; PIX_SORT(p[8], p[14]) ; PIX_SORT(p[8], p[11]) ; 
			PIX_SORT(p[12], p[15]) ; PIX_SORT(p[9], p[15]) ; PIX_SORT(p[9], p[12]) ; 
			PIX_SORT(p[13], p[16]) ; PIX_SORT(p[10], p[16]) ; PIX_SORT(p[10], p[13]) ; 
			PIX_SORT(p[20], p[23]) ; PIX_SORT(p[17], p[23]) ; PIX_SORT(p[17], p[20]) ; 
			PIX_SORT(p[21], p[24]) ; PIX_SORT(p[18], p[24]) ; PIX_SORT(p[18], p[21]) ; 
			PIX_SORT(p[19], p[22]) ; PIX_SORT(p[8], p[17]) ; PIX_SORT(p[9], p[18]) ; 
			PIX_SORT(p[0], p[18]) ; PIX_SORT(p[0], p[9]) ; PIX_SORT(p[10], p[19]) ; 
			PIX_SORT(p[1], p[19]) ; PIX_SORT(p[1], p[10]) ; PIX_SORT(p[11], p[20]) ; 
			PIX_SORT(p[2], p[20]) ; PIX_SORT(p[2], p[11]) ; PIX_SORT(p[12], p[21]) ; 
			PIX_SORT(p[3], p[21]) ; PIX_SORT(p[3], p[12]) ; PIX_SORT(p[13], p[22]) ; 
			PIX_SORT(p[4], p[22]) ; PIX_SORT(p[4], p[13]) ; PIX_SORT(p[14], p[23]) ; 
			PIX_SORT(p[5], p[23]) ; PIX_SORT(p[5], p[14]) ; PIX_SORT(p[15], p[24]) ; 
			PIX_SORT(p[6], p[24]) ; PIX_SORT(p[6], p[15]) ; PIX_SORT(p[7], p[16]) ; 
			PIX_SORT(p[7], p[19]) ; PIX_SORT(p[13], p[21]) ; PIX_SORT(p[15], p[23]) ; 
			PIX_SORT(p[7], p[13]) ; PIX_SORT(p[7], p[15]) ; PIX_SORT(p[1], p[9]) ; 
			PIX_SORT(p[3], p[11]) ; PIX_SORT(p[5], p[17]) ; PIX_SORT(p[11], p[17]) ; 
			PIX_SORT(p[9], p[17]) ; PIX_SORT(p[4], p[10]) ; PIX_SORT(p[6], p[12]) ; 
			PIX_SORT(p[7], p[14]) ; PIX_SORT(p[4], p[6]) ; PIX_SORT(p[4], p[7]) ; 
			PIX_SORT(p[12], p[14]) ; PIX_SORT(p[10], p[14]) ; PIX_SORT(p[6], p[7]) ; 
			PIX_SORT(p[10], p[12]) ; PIX_SORT(p[6], p[10]) ; PIX_SORT(p[6], p[17]) ; 
			PIX_SORT(p[12], p[17]) ; PIX_SORT(p[7], p[17]) ; PIX_SORT(p[7], p[10]) ; 
			PIX_SORT(p[12], p[18]) ; PIX_SORT(p[7], p[12]) ; PIX_SORT(p[10], p[18]) ; 
			PIX_SORT(p[12], p[20]) ; PIX_SORT(p[10], p[20]) ; PIX_SORT(p[10], p[12]) ; 
			return (p[12]); 
			break;
		default:
			fprintf (stderr,"unssuported number of pixels (%d)\n",l);
			break;
	}
}

//how easy is it to make this for all planes
uint8_t get_pixel(int x, int y, int plane, uint8_t *m[3],y4m_stream_info_t *si)
{

	int w,h;

	h = y4m_si_get_plane_height(si,plane);
	w = y4m_si_get_plane_width(si,plane);

	if (x < 0) {x=0;}
	if (x >= w) {x=w-1;}
	if (y < 0) {y=0;}
	if (y >= h) {y=h-1;}
	
	return 	*(m[plane]+x+y*w);
	
}

#define MEANSIZE 5
int mean_check (uint8_t *n[3],y4m_stream_info_t *si,int x, int y,int plane)
{
	
	int i,j;
	
	int tpix = 0,apix;
	int cpix;
	int l,m;
	
	l=y+MEANSIZE/2;
	m=x+MEANSIZE/2;
	
	cpix = get_pixel(x,y,plane,n,si);
	
	for (j=y-MEANSIZE/2;j<=l;j++) {
		for (i=x-MEANSIZE/2;i<=m;i++) {
			tpix += get_pixel(i,j,plane,n,si);
		}
	}

	apix = tpix / 25;
	// need standard deviation.
	// and a sigma threshhold
	
	return (abs(cpix-apix)>10); // temporary threshold.
	
}
void clean (uint8_t *l[3],uint8_t *m[3], uint8_t *n[3],y4m_stream_info_t *si,int t,int t1, int adp)
{

	int w,h,x,y,le;
	int cw,ch;
	int pix[7];
	int dif,diff,difa,difb;
	
	h = y4m_si_get_plane_height(si,t1);
	w = y4m_si_get_plane_width(si,t1);

	ch = y4m_si_get_plane_height(si,t1);
	cw = y4m_si_get_plane_width(si,t1);

	
	for(y=0;y<h;y++) {
		for (x=0;x<w;x++) {
			
			
			if (adp || mean_check(m,si,x,y,t1)) {
				// do some case around here.
				
				le=1;
				pix[0] = get_pixel(x,y,t1,m,si);
				
				if (t & 1) {
					pix[le++] = get_pixel(x,y,t1,l,si);
					pix[le++] = get_pixel(x,y,t1,n,si);
				}
				if (t & 2) {
					pix[le++] = get_pixel(x-1,y,t1,m,si);
					pix[le++] = get_pixel(x+1,y,t1,m,si);
				}
				if (t & 4) {
					pix[le++] = get_pixel(x,y-1,t1,m,si);
					pix[le++] = get_pixel(x,y+1,t1,m,si);
				}
				if (t & 8) {
					pix[le++] = get_pixel(x,y-2,t1,m,si);
					pix[le++] = get_pixel(x,y+2,t1,m,si);
				}
				//fprintf (stderr,"length : %d\n",le);
				*(m[t1]+x+y*w) = median(pix,le);
			} else {
				*(m[t1]+x+y*w) =  get_pixel(x,y,t1,m,si);
			}
			/*
			 //simple median filter.
			if ((pixa < pix && pix < pixb) || (pixb < pix && pix < pixa)) { 
				;
			}
			
			if ((pixa < pixb && pixb < pix) || (pix < pixb && pixb < pixa)) { 
				*(m[0]+x+y*w)= pixb;
			}
			
			if ((pix < pixa && pixa < pixb) || (pixb < pixa && pixa < pix)) { 
				*(m[0]+x+y*w)= pixa;
			}
			*/

		}
	}
		
}

static void process(  int fdIn , y4m_stream_info_t  *inStrInfo,
	int fdOut, y4m_stream_info_t  *outStrInfo,
	int t,int t1,int adp)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3][3];	
	uint8_t				*yuv_tdata[3];
	int                read_error_code  = Y4M_OK;
	int                write_error_code = Y4M_OK ;
	int                src_frame_counter ;
	int x,y,w,h,cw,ch;

	h = y4m_si_get_plane_height(inStrInfo,0);
	ch = y4m_si_get_plane_height(inStrInfo,1);

	chromalloc(yuv_data[0],inStrInfo);
	chromalloc(yuv_data[1],inStrInfo);
	chromalloc(yuv_data[2],inStrInfo);

	//fill with black
	chromaset(yuv_data[2],inStrInfo,16,128,128);

// initialise and read the first number of frames
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[1]);
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[0]);

	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {

		if (t1 & 2) {
			clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,0,adp);
		}
		if (t1 & 1) {
			clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,1,adp);
			clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,2,adp);
		}
		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[1] );
		y4m_fini_frame_info( &in_frame );
		
		// nothing to see here, move along, move along
		// it might be quicker to move the pointers.
	//	chromacpy(yuv_data[2],yuv_data[1],inStrInfo);
	//	chromacpy(yuv_data[1],yuv_data[0],inStrInfo);
		
		// as long as there are 3 planes
		yuv_tdata[0] = yuv_data[2][0];
		yuv_tdata[1] = yuv_data[2][1];
		yuv_tdata[2] = yuv_data[2][2];
		
		yuv_data[2][0] = yuv_data[1][0];
		yuv_data[2][1] = yuv_data[1][1];
		yuv_data[2][2] = yuv_data[1][2];
		
		yuv_data[1][0] = yuv_data[0][0];
		yuv_data[1][1] = yuv_data[0][1];
		yuv_data[1][2] = yuv_data[0][2];
		
		yuv_data[0][0] = yuv_tdata[0];
		yuv_data[0][1] = yuv_tdata[1];
		yuv_data[0][2] = yuv_tdata[2];

		
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[0] );
		++src_frame_counter ;
	}

	// still have 1 frame to clean and write.
	
	if( read_error_code == Y4M_ERR_EOF ) {
		chromaset(yuv_data[0],inStrInfo,16,128,128);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,0,adp);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,1,adp);
		clean (yuv_data[0],yuv_data[1],yuv_data[2],inStrInfo,t,2,adp);
	
		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[1] );	
	}
  // Clean-up regardless an error happened or not

	y4m_fini_frame_info( &in_frame );

	chromafree( yuv_data[0] );
	chromafree( yuv_data[1] );
	chromafree( yuv_data[2] );
	
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
	const static char *legal_flags = "v:m:s:cya";
	int max_shift = 0, search = 0;
	int cl=3;
	int c,adp;

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
			case 'c':
				cl=1;
				break;
			case 'y':
				cl=2;
				break;
			case 'a':
				adp = 1;
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
  mjpeg_info ("yuvtshot (version " YUVRFPS_VERSION ") is a temporal shot noise remover");
  mjpeg_info ("yuvtshot -? for help");

  /* in that function we do all the important work */
	y4m_write_stream_header(fdOut,&out_streaminfo);

	process( fdIn,&in_streaminfo,fdOut,&out_streaminfo,max_shift,cl,adp);

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
