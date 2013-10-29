/*
  *

**<h3>Convolution matrix for YUV streams</h3>
**
**<p>Performs a generic convolution filter on the video.  Quite slow.
**Support any odd dimension matrix (3x3, 5x5, 7x7...) uses the command
**line argument -m 1,2,3,4,5,6,7,8,9.  </p>
**
**<p> I am thinking about adding support for predefined matricies,
**such as blur, sharpen, edge detection, emboss.</P>
**
**<p>I now know that it's not wise to run sharpening on low bitrate
**mpeg files, as it highlights the macroblocks.  </p>
**
**<h4>RESULTS</h4>
**
**<? news::imageinline("lavtools/yuvconvolve",-1); ?>
**
**<h4>HISTORY</h4>
**<ul>
**
**<li> 3 Mar 2008.  Found a bug where the first matrix element was
**undefined, which caused the first matrix entry to 0 and sometimes
**caused bus errors. (dont you hate those sometimes bugs...)</li>
**</ul>

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
#include <string.h>
#include <math.h>

#include <yuv4mpeg.h>
#include <mpegconsts.h>

#define YUVRFPS_VERSION "0.1"

/* some example kernels */
/* one day I might make these automatically selectable */

#define blur1 "1,1,1,1,1,1,1,1,1"
#define blur2 "1,2,1,2,4,2,1,2,1"
#define sharpen1 "-1,-1,-1,-1,9,-1,-1,-1,-1"
#define sharpen2 "0,-1,0,-1,5,-1,0,-1,0"
#define edgeenhance1 "0,0,0,-1,1,0,0,0,0"
#define edgeenhance2 "0,-1,0,0,1,0,0,0,0"
#define edgeenhance3 "-1,0,0,0,1,0,0,0,0"
#define edgefind1 "0,1,0,1,-4,1,0,1,0"
#define edgefind2 "-1,-1,-1,-1,8,-1,-1,-1,-1"
#define edgefind3 "1,-2,1,-2,4,-2,1,-2,1"
#define emboss "-2,-1,0,-1,1,1,0,1,2"
#define gaussian "1,4,7,4,1,4,16,26,16,26,4,7,26,41,26,7,4,16,26,16,4,1,4,7,4,1"

static void print_usage()
{
  fprintf (stderr,
	   "usage: yuvconvolve [-d <divisor> -m <matrix> -v <verbose>]\n"
	   "yuvconvolve performs convolution matrix to yuv streams\n"
           "\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
	   "\t -m <matrix> convolution matrix seperated by commas\n"
	   "\t -d <divisor> defaults to sum of matrix or 1 if sum == 0 \n"
         );
}

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

int sum_matrix(int *marr, int len)
{

	int digits = 0;
	int x,y;
	int sum = 0;

	for (x=0; x <len; x++)
		for (y=0; y<len; y++)
			sum += marr[digits++];

	return sum;
}

int parse_matrix (char *mstr, int *marr)
{

	int digits=1;
	//float dim;
	char *tok;

		fprintf (stderr,"parse_matrix (%s)\n",mstr);


	// I'll make the assumption that the number of characters in the matrix string is longer than the number of digits
	digits = 0;
			fprintf (stderr,"parse_matrix strtok\n");

	tok = strtok (mstr,",");
				fprintf (stderr,"parse_matrix marr[0]=\n");

	marr[digits]= atoi(tok);

	fprintf (stderr,"M: %d ",marr[0]);

				//	fprintf (stderr,"parse_matrix while\n");


	while (tok=strtok(NULL,",")) {
				//		fprintf (stderr,"parse_matrix while digits++\n");

			digits ++;
		marr[digits]= atoi(tok);
	fprintf (stderr,"%d ",marr[digits]);

	}
	fprintf (stderr,"\nparse_matrix dim = \n");

	//dim = sqrt (digits);

	// if (dim != integer) {mjpeg_warn("not a square number of matrix points"); return 0 }

	// if (dim != odd) { mjpeg_warn("Not an odd matrix dimention"); return 0 }


			fprintf (stderr,"exit parse_matrix\n");


	return (int)sqrt(digits+1);

}

static void convolve(  int fdIn , y4m_stream_info_t  *inStrInfo,
int fdOut, y4m_stream_info_t  *outStrInfo,
int *mat, int div, int mlen)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3],*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int                src_frame_counter ;
	float vy,vu,vv;
	int x,y,w,h,cw,ch,mx,my,count;


	w = y4m_si_get_plane_width(inStrInfo,0);
	h = y4m_si_get_plane_height(inStrInfo,0);
	cw = y4m_si_get_plane_width(inStrInfo,1);
	ch = y4m_si_get_plane_height(inStrInfo,1);

	if (chromalloc(yuv_data, inStrInfo))
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	if (chromalloc(yuv_odata, inStrInfo))
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");


	write_error_code = Y4M_OK ;
	src_frame_counter = 0 ;

// initialise and read the first number of frames
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );

	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {

		for (x=0; x<w; x++) {
			for (y=0; y<h; y++) {
			// perform magic

				vy = 0; count = 0;
				// need to be handled differently for interlace
				for (my=-mlen/2;my <=mlen/2; my++) {
					for (mx=-mlen/2;mx <=mlen/2; mx++) {

					//	fprintf (stderr," x %d - y %d\n",mx,my);

						if ((x + mx >=0) && (x + mx <w) &&
						(y + my  >=0) && (y + my  <h) ) {
					//	fprintf (stderr,"matrix: %d => %d\n", count,mat[count]);
							vy += *(yuv_data[0]+x+mx+(y+my)*w) * mat[count];
						}
						count++;

					}
				}
				vy /= div;
				if (vy < 16) vy = 16;
				if (vy > 240) vy= 240;
				*(yuv_odata[0]+x+y*w) = vy;

				if ((x < cw) && (y<ch)) {

				vu = 0;
				vv = 0;
				count = 0;
				// may need to be handled differently for interlace
				for (my=-mlen/2;my <=mlen/2; my++) {
					for (mx=-mlen/2;mx <=mlen/2; mx++) {

						if ((x + mx >=0) && (x + mx <cw) &&
						(y + my  >=0) && (y + my  <ch) ) {
							vu += (*(yuv_data[1]+x+mx+(y+my)*cw) -128) * mat[count];
							vv += (*(yuv_data[2]+x+mx+(y+my)*cw) -128) * mat[count];

						}
						count ++;

					}
				}
				vu /= div;
				vv /= div;

				if (vu < -112) vu = -112;
				if (vu > 112) vu = 112;

				if (vv < -112) vv = -112;
				if (vv > 112) vv = 112;


				*(yuv_odata[1]+x+y*cw) = vu + 128;
				*(yuv_odata[2]+x+y*cw) = vv + 128;

				}
			}
		}
	write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );
		++src_frame_counter ;

	}

  // Clean-up regardless an error happened or not

	y4m_fini_frame_info( &in_frame );

	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );
	free( yuv_odata[0] );
	free( yuv_odata[1] );
	free( yuv_odata[2] );


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

	int verbose = 4; // LOG_ERROR ;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo,out_streaminfo;
	const static char *legal_flags = "d:m:V:";
	int c, *matrix,matlen;
	float divisor=0;

  while ((c = getopt (argc, argv, legal_flags)) != -1) {
    switch (c) {
      case 'V':
        verbose = atoi (optarg);
        if (verbose < 0 || verbose > 2)
          mjpeg_error_exit1 ("Verbose level must be [0..2]");
        break;
    case 'd':
	    divisor = atof(optarg);
		if (divisor == 0) {
			mjpeg_error_exit1 ("Divisor must not be 0");
		}

		break;
	case 'm':
		// strlen should be longer than the
		matrix = (int *) malloc (sizeof(int) * strlen(optarg));
		matlen = parse_matrix(optarg,matrix);
		if (matlen == 0) {
			mjpeg_error_exit1 ("Invalid matrix");
		}
		break;

	case '?':
          print_usage (argv);
          return 0 ;
          break;
    }
  }

	if (divisor == 0) {
		divisor = sum_matrix(matrix,matlen);
	}

	if (divisor == 0) {
		mjpeg_warn("divisor defaulting to 1\n");
		divisor = 1;
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

	y4m_ratio_t src_frame_rate = y4m_si_get_framerate( &in_streaminfo );
	y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );

  // Information output
  mjpeg_info ("yuvconvolve (version " YUVRFPS_VERSION ") performs a convolution matrix on yuv streams");
  mjpeg_info ("yuvconvolve -? for help");

	y4m_write_stream_header(fdOut,&out_streaminfo);

  /* in that function we do all the important work */

  fprintf (stderr,"matrix square: %d\n",matlen);

	convolve( fdIn,&in_streaminfo,fdOut,&out_streaminfo,matrix,divisor,matlen);

  y4m_fini_stream_info (&in_streaminfo);
  y4m_fini_stream_info (&out_streaminfo);

  return 0;
}
