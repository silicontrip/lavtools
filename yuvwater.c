/*
  *  yuvwater.c
  *  water  2004 Mark Heath <mjpeg at silicontrip.org>
  *  calculates a watermark by averaging all frames in a video
  *  stream.
  *  Attempts to remove (or add)  the watermark by using the resulting average
  *  mask.
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

**<h3>Flat Transparent watermark detection and removal</h3>

**<p>Attempts to detect and remove semi-transperant watermarks from
**the source.  Produces a PGM file of the detected watermark which
**is used to remove or reduce the effect.  This is a two pass process,
**the documentation is a little sparse.  </p>

**<p> The first pass produces a grey image file (PGM format) this is
**done by averaging luma of all the frames.  The idea is that the
**watermark is consistantly brighter than the surrounding video.
**The PGM file may need to be edited (blurred) in places as other
**long term artifacts may be visible.  </p>

**<p> The second pass modifies the luma of the video reducing the
**brightness by the amount in the pgm file.  This is not a linear
**reduction.  Trial and error was used to determine the correct
**formula to reduce the luma by.  For flat watermarks, this function
**is ideal.  </p>

**<p> Non flat watermarks cannot be removed by this program.  Colour
**watermarks cannot be removed by this program.  Solid watermarks
**cannot be removed by this program.  </p>

**<h4>EXAMPLE</h4>
**<p> Pass 1:  <tt> | yuvwater -d > watermark.pgm </tt></p>

**<p> May also use a black frame with the watermark showing, extracted
**by other means.</P>

**<p> pass 2 <tt>| yuvwater  yuvwater -m 145 -l 72  -u 384  -i
**watermark.pgm | </tt> -m specifies the amount to remove, the lower
**the number the darker the resulting watermark, good starting values
**are 140.  -l specifies the black level for normalisation, good
**starting values between 32-80. If the black is too light, increase
**this value.  -u specifies the white level, good starting value 384,
**if the white is too dark, decrease this value.  </p>

**<h4>HISTORY</h4>
**<ul> <li>26th April 2005. Fixed a bug which incorrectly detected
**the end of file, creating more frames in the output.</li> <li>17th
**May 2005.  Fixed a bug which incorrectly compared the pgm size and
**the video size.</li> </ul>



  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <strings.h>
#include <fcntl.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"

#define YUVPRO_VERSION "0.1"

/* the scale from 0 to 1 */
#define MUL_SCALE 224

static void print_usage() 
{
  fprintf (stderr,
	   "usage: yuvwater [-v -h]\n"
	   "yuvwater detects constant image watermarks\n"
	   "producing a watermark pgm, which can be used\n"
           "to remove the watermark\n"
           "\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
	   "\t -h print this help\n"
         );
}

static int removewm (int fdWM, int fdIn, y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t *outStrInfo, int bri, int mul,int lr,int ur)
{

	y4m_frame_info_t   in_frame ;
	uint8_t *yuv_data[3] ;
	uint8_t *yuvwm_data ;
	int frame_data_size ;
	int write_error_code ;
	int read_error_code ;
	int w,h,l=0,yuv,yuvmin,yuvmax,lp=0,up=0;
	char buf[80],buf2[80];

	h = y4m_si_get_height(inStrInfo) ; w = y4m_si_get_width(inStrInfo);
	frame_data_size = h * w;

	read(fdWM,buf,3);
	if (strncmp (buf,"P5",2)) {
		perror ("Watermark file is not a binary PGM\n");
		return -1;
	}
	sprintf (buf2,"%d %d\n",w,h);
	read(fdWM,buf,strlen(buf2));
	if (strncmp (buf,buf2,strlen(buf2)-1)) {
		fprintf (stderr, "(%s != %s)",buf,buf2);
		perror ("Watermark file's dimensions do not match video\n");
		return -1;
	}
	read(fdWM,buf,4);
	if (strncmp (buf,"255",3)) {
		perror ("Watermark file is not 8 bit\n");
		return -1;
	}


	yuvwm_data = (uint8_t  *)malloc( frame_data_size );

	read (fdWM,yuvwm_data,frame_data_size);	

	yuv_data[0] = (uint8_t *)malloc( frame_data_size );
	yuv_data[1] = (uint8_t *)malloc( frame_data_size >> 2 );
	yuv_data[2] = (uint8_t *)malloc( frame_data_size >> 2 );

	if( !yuv_data[0] || !yuv_data[1] || !yuv_data[2] || !yuvwm_data )
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");


	write_error_code = Y4M_OK ;

	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );

	yuvmax = yuvmin =
			 (yuv_data[0][l]) + (bri - ((255 - yuv_data[0][l])  * yuvwm_data[l] / mul));

        while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {

		if (read_error_code == Y4M_OK) {

		for (l=0;l<frame_data_size;l++)
		{
			yuv = (yuv_data[0][l]) + (bri - ((255 - yuv_data[0][l])  * yuvwm_data[l] / mul));

		/* find extremes */
			if (yuvmin > yuv) yuvmin = yuv;
			if (yuvmax < yuv) yuvmax = yuv;

		/* use upper and lower scaling */
		if (ur>lr) yuv = (yuv - lr) *  224 / (ur - lr)  + 16;

		/* prevent clipping */
			if (yuv > 240) yuv = 240;
			if (yuv < 16) yuv = 16;
			
			yuv_data[0][l] = yuv;
		}

		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data );
		}
                y4m_fini_frame_info( &in_frame );
                y4m_init_frame_info( &in_frame );
                read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	if (ur==lr) 
		fprintf (stderr, "Range: %d - %d\n",yuvmin,yuvmax);
	else {
		if ((yuvmax > ur) && (yuvmax > up )) {
			fprintf (stderr, "Warning: Max %d > %d upper limit\n",yuvmax,ur);
			up = yuvmax;
			// ur = yuvmax;
		}
		if ((yuvmin < lr) && (yuvmin < lp)) {
			fprintf (stderr, "Warning: Min %d < %d lower limit\n",yuvmin,lr);
			lp = yuvmin;
			// lr = yuvmin;
		}
	}
	// f++;

	}

	y4m_fini_frame_info( &in_frame );

	fprintf (stderr, "Range: %d - %d\n",yuvmin,yuvmax);

	free( yuvwm_data );
	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );

	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	if( write_error_code != Y4M_OK )
		mjpeg_error_exit1 ("Error writing output stream!");

	return 0;
}

static void detectwm(int fdIn , y4m_stream_info_t  *inStrInfo, int frames )
{
	y4m_frame_info_t in_frame ;
	uint8_t *yuv_data[3] ;
	unsigned int	*yuvf_data;
	int frame_data_size ;
	int read_error_code ;
	int w,h,y,l,f=0;

// Allocate memory for the YUV channels
	h = y4m_si_get_height(inStrInfo) ; w = y4m_si_get_width(inStrInfo);
	frame_data_size = h * w;
	yuv_data[0] = (uint8_t *)malloc( frame_data_size );
	yuv_data[1] = (uint8_t *)malloc( frame_data_size >> 2 );
	yuv_data[2] = (uint8_t *)malloc( frame_data_size >> 2 );
	yuvf_data = (unsigned int *)malloc( frame_data_size * sizeof(unsigned int) );


	if( !yuv_data[0] || !yuv_data[1] || !yuv_data[2] || !yuvf_data )
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

/* Initialize counters */

	for (l=0; l < frame_data_size; l++)
		yuvf_data[l] = 0;

	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );

	while((Y4M_ERR_EOF != read_error_code ) && (f != frames)) {

// calculate average.
		if (read_error_code == Y4M_OK) {

		int l = 0;
		for (y=0; y<h; y++) 
		{
			int x;
			for (x=0; x<w; x++) 
			{
				yuvf_data[l]  += yuv_data[0][l];
				l++;
			}

		}

		f++;
	
		}

		y4m_fini_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );

	}

  // Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );

	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );

	// Output resulting PGM mask

	fprintf (stdout,"P5\n%d %d\n255\n",w,h);
	for (l=0; l < frame_data_size; l++)
		fprintf (stdout,"%c",(yuvf_data[l] / f));
	

	free( yuvf_data );

	if( read_error_code != Y4M_ERR_EOF )
    mjpeg_error_exit1 ("Error reading from input stream!");

}

// ***************************************************************************
// MAIN
// ***************************************************************************
int main (int argc, char *argv[])
{

	int detect=0,verbose = 4; // LOG_ERROR ;
	int fdIn = 0 ;
	int fdWM = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	int brightness=128;
	int multiple=MUL_SCALE;
	int upper=0,lower=0,frames=-1;

	const static char *legal_flags = "v:dhi:m:u:l:f:";
	int c ;

	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					  mjpeg_error_exit1 ("Verbose level must be [0..2]");
			break;
			case 'd': detect=1; break;
			case 'u': upper = atoi(optarg); break;
			case 'l': lower = atoi(optarg); break;
			case 'm': multiple = atoi(optarg); break;
			case 'f': frames = atoi(optarg); break;
			case 'i': fdWM = open (optarg,O_RDONLY,0); break;
			case 'h':
			case '?':
				print_usage (argv);
				return 0 ;
			break;
		}
	}
  // mjpeg tools global initialisations
  mjpeg_default_handler_verbosity (verbose);

	if ((!fdWM) && (!detect))  {
		perror ("Could not open Watermark file\n");
		exit (-1);
	}

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
  mjpeg_info ("yuv2fps (version " YUVPRO_VERSION
              ") is a general frame resampling utility for yuv streams");
  mjpeg_info ("(C) 2002 Alfonso Garcia-Patino Barbolani <barbolani@jazzfree.com>");
  mjpeg_info ("yuvfps -h for help, or man yuvfps");



    
  /* in that function we do all the important work */
	if (detect) 
		detectwm( fdIn,&in_streaminfo, frames );
	else {
		y4m_init_stream_info (&out_streaminfo);
		y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
		y4m_write_stream_header(fdOut,&out_streaminfo);
		removewm(fdWM,fdIn,&in_streaminfo,fdOut,&out_streaminfo,brightness,multiple,lower,upper);
		y4m_fini_stream_info (&out_streaminfo);
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
