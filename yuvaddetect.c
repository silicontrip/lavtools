/*

** <h3>Advertisement detection</h3>
** <p>Most of the functionality of this code has been replaced by yuvvalues.c any further work on this will be focussed there.</p>
** <h4>INCOMPLETE</h4>
**
** <p> Currently only produces a text file (which can be graphed by
** gnuplot) of the average lightness of the sourceframes.  The idea
** is that there is a black fade before and after advertisements.  To
** produce chapter markers for DVDs from TV.</p>
**
** <h4>RESULTS</h4>
** <p>
** <img src="7hd_ad.png">
** </p>
**
** <p>
** From looking at this above graph it appears that this program is doing a frame difference,
** rather than averaging luma. Which is the same function as yuvdiff.
**
** Now we can quite easily see scene changes, shown as sharp spikes. At about 810 and 1500</p>
** <p>
** We can also tell that the video has gone through a frame rate doubling by
** frame dupliation. Shown by the high frequency component.
** </p>
** <p><i>I should move this image down to YUV diff</i></p>

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
  *   gcc -L/sw/lib -I/sw/include/mjpegtools -lmjpegutils yuvaddetect.c -o yuvaddetect
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
	   "usage: yuvaddetect [-v -h -i|-p]\n"
	   "yuvaddetect produces a 2d graph showing time vs frame difference\n"
           "\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
	   "\t -i Force interlace\n"
	   "\t -p Force progressive\n"
	   "\t -h print this help\n"
         );
}

static void detect(  int fdIn , y4m_stream_info_t  *inStrInfo)
{
  y4m_frame_info_t   in_frame ;
  uint8_t            *yuv_data[3] ;
  uint8_t            *yuv_odata[3] ;

  int                frame_data_size ;
  int                read_error_code ;
  int                write_error_code ;
  int                src_frame_counter ;
  int bri=0,l=0;

  // Allocate memory for the YUV channels
  frame_data_size = y4m_si_get_height(inStrInfo) * y4m_si_get_width(inStrInfo);
  yuv_data[0] = (uint8_t *)malloc( frame_data_size );
  yuv_data[1] = (uint8_t *)malloc( frame_data_size >> 2);
  yuv_data[2] = (uint8_t *)malloc( frame_data_size >> 2);

  yuv_odata[0] = (uint8_t *)malloc( frame_data_size );
  yuv_odata[1] = (uint8_t *)malloc( frame_data_size >> 2);
  yuv_odata[2] = (uint8_t *)malloc( frame_data_size >> 2);


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

	// if progressive
	bri = 0;
	for (l=0; l< frame_data_size; l++)
		bri += abs(yuv_data[0][l]-yuv_odata[0][l]);

	printf ("%d %d\n",src_frame_counter,bri);

	++src_frame_counter ;

	y4m_fini_frame_info( &in_frame );
	y4m_init_frame_info( &in_frame );

	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_odata );

// check for read error

	bri = 0;
	for (l=0; l< frame_data_size; l++)
		bri += abs(yuv_data[0][l]-yuv_odata[0][l]);

	printf ("%d %d\n",src_frame_counter,bri);

	++src_frame_counter ;

	y4m_fini_frame_info( &in_frame );
	y4m_init_frame_info( &in_frame );


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

  int verbose = 4; // LOG_ERROR ;
  int fdIn = 0 ;
  y4m_stream_info_t in_streaminfo;

  const static char *legal_flags = "v:h";
  int c ;

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

  // Prepare output stream

  // Information output
  mjpeg_info ("yuv2fps (version " YUVFPS_VERSION
              ") is a general frame resampling utility for yuv streams");
  mjpeg_info ("(C) 2002 Alfonso Garcia-Patino Barbolani <barbolani@jazzfree.com>");
  mjpeg_info ("yuvaddetect -h for help, or man yuvaddetect");


  /* in that function we do all the important work */
  detect( fdIn,&in_streaminfo);

  y4m_fini_stream_info (&in_streaminfo);

  return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
