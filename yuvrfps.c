/*
 *  yuvrfps.c
**<h3>Frame Dropping Frame Rate Converter </h3>
**<p>
**Performs frame rate reduction by simple fractional amounts, by
**dropping 1 out of every X frames or fields.
**</p><p>
**This is to work with progressive 3:2 pulldown files performing conversions such as
**29.97 to 23.976 similar to yuvkineco or even 30 to 25 fps.  It
**analyses X frames and finds the least different fram e and drops
**it, in theory removing any duplicated frames.
**</p>
**<p> It is possible that the 3:2 pulldown cadence starts on an interlace frame
**in which the detection will make all frames interlace (field reversed progressive)
**this can be corrected by the -s option to start the sequence on a progressive frame.
**It should be less than the Decimation number.
**</p>
**<h4>Usage</h4>
**<pre>
**-F <Decimation> reduces X frames to X-1
**   Common Decimation ratios
**   5: 29.970 -> 23.976
**   6: 30 -> 25
**-f <X[:Y]> force drop frame X (or X:Y fields)
**-I t|b|p Force interlace mode
**-s <X> skip X frames. Output X frames unchanged before starting detection.
**</pre>

 *
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


#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


#include <yuv4mpeg.h>
#include <mpegconsts.h>

#define YUVRFPS_VERSION "0.1"

static void print_usage()
{
	fprintf (stderr,
			 "usage: yuvrfps [-F <decimation>]  [-I t|b|p] [-v 0|1|2]\n"
			 "yuvrfps reduces frame rate by decimation\n"
			 "\n"
			 "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
			 "\t -I<pbt> Force interlace mode\n"
			 "\t -F <X> Drop 1 frame every X frames\n"
			 "\t -f <X>[:<Y>] force drop frame X (or fields X:Y) rather than detect\n"
			 "\t -s <X> skip X frames. Output X frames before performing drop. To synchronize 3:2 pulldown cadence\n"
			 "\t -h print this help\n"
			 );
}

void copyfield(int which, y4m_stream_info_t *sinfo, unsigned char *input[], unsigned char *output[])
{
	// "which" is 0 for top field, 1 for bottom field
	int r = 0;

	int h,w,cw,ch;

	h = y4m_si_get_plane_height(sinfo,0);
	w = y4m_si_get_plane_width(sinfo,0);
	cw = y4m_si_get_plane_width(sinfo,1);
	ch = y4m_si_get_plane_height(sinfo,1);

	int chromapos = which * cw;
	int lumapos = which * w;



	for (r = which; r < h; r += 2)
	{
		memcpy(&output[0][lumapos], &input[0][lumapos], w);
		if (r < ch) {
			memcpy(&output[1][chromapos], &input[1][chromapos], cw);
			memcpy(&output[2][chromapos], &input[2][chromapos], cw);
			chromapos += cw * 2;
		}
		lumapos += w * 2;
	}
}




// read X frames
// diff each frame (X-1)
// for 1 to X
// if frame != diff write frame


static void detect(  int fdIn , y4m_stream_info_t  *inStrInfo,
				   int fdOut, y4m_stream_info_t  *outStrInfo,
				   int interlacing, int drop_frames,
				   int iforce, int oforce,int skip)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[drop_frames+1][3] ;

	int                frame_data_size ;
	int                read_error_code ;
	int                write_error_code ;
	int *bri, *bro,l=0,f=0,mini=0,mino=0,dropmo=0,dropmi=0,x,w,h;

	// Allocate memory for the YUV channels
	w = y4m_si_get_width(inStrInfo);
	h =  y4m_si_get_height(inStrInfo);
	frame_data_size = h *w;

	bri = (int *)malloc(sizeof(int) * (drop_frames+1));
	bro = (int *)malloc(sizeof(int) * (drop_frames+1));
	// should check for allocation errors here.

	for (f=0; f<= drop_frames; f++) {
		yuv_data[f][0] = (uint8_t *)malloc( frame_data_size );
		yuv_data[f][1] = (uint8_t *)malloc( frame_data_size >> 2);
		yuv_data[f][2] = (uint8_t *)malloc( frame_data_size >> 2);
		if( !yuv_data[f][0] || !yuv_data[f][1] || !yuv_data[f][2])
		    mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	}

	write_error_code = Y4M_OK ;


	// initialise and read the first number of frames
	for (f=0; f <= skip; f++) {
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[0] );

	// we will never drop the first frame of a file
		write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[0] );
	}

	for (f=1; f<=drop_frames && Y4M_ERR_EOF != read_error_code; f++) {
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[f] );
	}

	// f should be drop_frames + 1
	// do some test if fewer frames read and change drop_frames to equal frames read

	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {

		// compare all frames

		if (iforce>=0) {
			dropmi = iforce;
			dropmo = oforce;
		} else {
			for (f=1; f<=drop_frames;f++) {

				// perform frame difference.
				// only comparing Luma, less noise, more resolution... blah blah

				bri[f] = 0; bro[f]=0;

				for (l=0; l< frame_data_size; l+=w<<1) {
					for (x=0; x<w; x++) {
						bri[f] += abs(yuv_data[f][0][l+x]-yuv_data[f-1][0][l+x]);
						bro[f] += abs(yuv_data[f][0][l+x+w]-yuv_data[f-1][0][l+x+w]);
					}
				}


			}

			// find the minimum difference field.

			if (interlacing == Y4M_ILACE_NONE) {

				mini = bri[1] + bro[1]; dropmi = 1;
				for (f=2; f<=drop_frames; f++) {
					if (mini > (bri[f]+bro[f])) {
						mini = bri[f]+bro[f];
						dropmi=f;
					}
				}

			} else {

				//	fprintf(stderr,"field 1: odd: %d even: %d\n",bro[1],bri[1]);

				mini = bri[1]; dropmi=1;
				mino = bro[1]; dropmo=1;
				for (f=2; f<=drop_frames; f++) {
					//		fprintf(stderr,"field %d: odd: %d even: %d\n",f,bro[f],bri[f]);

					if (mini > bri[f]) {
						mini = bri[f];
						dropmi=f;
					}
					if (mino > bro[f]) {
						mino = bro[f];
						dropmo=f;
					}
				}
			}
		}
		// for progressive dropmi and dropmo *should* be the same.

		if (interlacing == Y4M_ILACE_NONE || dropmi == dropmo) {
			mjpeg_info("Dropping frame: %d",dropmi);
			for (f=1; f<=drop_frames;f++) {
				if (f != dropmi) {
					//	fprintf(stderr,"writing %d (drop %d)\n",f,dropmi);
					write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[f] );
				}
			}
		} else {
			mjpeg_info("Dropping fields even: %d odd: %d",dropmi,dropmo);
			for (f=1; f<drop_frames;f++) {


				if (f == dropmi)
					for (l=f; l<drop_frames; l++)
						copyfield (0,inStrInfo, yuv_data[l+1],yuv_data[l]);

				if (f == dropmo)
					for (l=f; l<drop_frames; l++)
						copyfield (1,inStrInfo, yuv_data[l+1],yuv_data[l]);


				write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data[f] );

			}
		}
		// output all but the minimum difference frame.
		//  how do I drop a single field from two different frames

		// the additional frame is for comparing to the next block of frames
		// we do not want to write it twice


		// copy the last frame to the first

		memcpy(yuv_data[0][0],yuv_data[drop_frames][0],frame_data_size);
		// assumes 420 video
		// TODO: read chroma subsampling from stream
		memcpy(yuv_data[0][1],yuv_data[drop_frames][1],frame_data_size>>2);
		memcpy(yuv_data[0][2],yuv_data[drop_frames][2],frame_data_size>>2);

		/*
		 for (l=0; l< frame_data_size; l++) {
		 yuv_data[0][0][l] = yuv_data[drop_frames][0][l];
		 if (!l%2) {
		 yuv_data[0][1][l>>2] = yuv_data[drop_frames][1][l>>2];
		 yuv_data[0][2][l>>2] = yuv_data[drop_frames][2][l>>2];
		 }
		 }
		 */

		// TODO if read fewer than drop_frames main loop will exit.

		for (f=1; f<=drop_frames && Y4M_ERR_EOF != read_error_code; f++) {
			y4m_fini_frame_info( &in_frame );
			y4m_init_frame_info( &in_frame );
			read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data[f] );
		}


    }
	// Clean-up regardless an error happened or not

	free(bro); free(bri);

	y4m_fini_frame_info( &in_frame );

	for (f=0; f<=drop_frames; f++) {
		free( yuv_data[f][0] );
		free( yuv_data[f][1] );
		free( yuv_data[f][2] );
	}

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

//
// Returns the greatest common divisor (GCD) of the input parameters.
//
static int gcd(int a, int b)
{
	if (b == 0)
		return a;
	else
		return gcd(b, a % b);
}


// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{

	int verbose = 4; // LOG_ERROR ;
	int drop_frames = 0;
	int oforce = -1;
	int iforce = -1;
	int skip = 0;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo,out_streaminfo;
	int src_interlacing = Y4M_UNKNOWN;
	y4m_ratio_t src_frame_rate;
	const static char *legal_flags = "s:F:f:I:v:h";
	int c ;

	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
				break;
			case 'I':
				src_interlacing = parse_interlacing(optarg);
				break;
			case 'F':
				drop_frames = atoi(optarg);
				break;
			case 'f':
				//force = atof(optarg);
				sscanf (optarg,"%d:%d",&iforce,&oforce);
				//	fprintf (stderr,"Full: %d Even: %d Odd: %d\n",force,iforce,oforce);
				break;
			case 's':
				skip = atoi(optarg);
				break;
			case 'h':
			case '?':
				print_usage (argv);
				return 0 ;
				break;
		}
	}

	if (drop_frames < 2)
		mjpeg_error_exit1("Minimum 2 frames to compare");


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
	y4m_accept_extensions(1); // because we handle different chroma subsampling
	if (y4m_read_stream_header (fdIn, &in_streaminfo) != Y4M_OK)
		mjpeg_error_exit1 ("Could'nt read YUV4MPEG header!");

	src_frame_rate = y4m_si_get_framerate( &in_streaminfo );
	y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );

	mjpeg_info ("Input framerate %d:%d",src_frame_rate.n,src_frame_rate.d);


	src_frame_rate.d = src_frame_rate.d * drop_frames;
	src_frame_rate.n = src_frame_rate.n * (drop_frames-1);

	mjpeg_info( "Output framerate %d:%d\n",src_frame_rate.n,src_frame_rate.d);

	y4m_si_set_framerate( &out_streaminfo, src_frame_rate );


	// Information output
	mjpeg_info ("yuvrfps (version " YUVRFPS_VERSION ") is a frame rate converter which drops doubled frames for yuv streams");
	mjpeg_info ("yuvrfps -h for help");

	if (src_interlacing == Y4M_UNKNOWN)
		src_interlacing = y4m_si_get_interlace(&in_streaminfo);


	/* in that function we do all the important work */
	y4m_write_stream_header(fdOut,&out_streaminfo);

	detect( fdIn,&in_streaminfo,fdOut,&out_streaminfo,src_interlacing,drop_frames,iforce,oforce,skip);

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
