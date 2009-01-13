/*
  *  yuvafps.c
  *  Copyright (C) 2002 Alfonso Garcia-Patiño Barbolani <barbolani at jazzfree dot com>
  *  Linear Frame averaging modification 2008 Mark Heath <mjpeg1 at silicontrip dot org>
  *
  *
  *  Upsamples or downsamples a yuv stream to a specified frame rate
  *  Interlace modification 6 Sep 2006
  *  Interlace fixes 12 Sep 2007
  *  Chroma subsampling modification 12 Sep 2007
  *  Major bugfixes and code cleanup 13 Sep 2007
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

gcc yuvaifps.c -L/sw/lib -I/sw/include/mjpegtools -lmjpegutils -o yuvaifps
  
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

#define YUVFPS_VERSION "0.1"

// using 16 bits for the percentage precision, can go to 24 with uint32_t 
// or even to 56 bit with uint64_t ?

#define PRECISION 16


static void print_usage() 
{
  fprintf (stderr,
	   "usage: yuvfps [-Ib|t] -r NewFpsNum:NewFpsDen [-s [InputFpsNum:InputFpsDen]] [-c] [-v -h]\n"
	   "yuvfps resamples a yuv video stream read from stdin to a new stream, identical\n"
           "to the source with frames repeated/copied/removed written to stdout.\n"
           "\n"
           "\t -r Frame rate for the resulting stream (in X:Y fractional form)\n"
           "\t -s Assume this source frame rate ignoring source YUV header\n"
           "\t -c Change only the output header frame rate, does not modify stream\n"
		   "\t -I Force interlace conversion\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
	   "\t -h print this help\n"
         );
}

// Resamples the video stream coming from fdIn and writes it
// to fdOut.
// There are two variations of the same theme:
//
//   Upsampling: frames are duplicated when needed
//   Downsampling: frames from the original are skipped

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


// Mix the source frame into the destination uint32_t  frame.
// does top field, bottom field or both fields (progressive)

// percent is a 16 bit integer (65535 == 100%) Hopefully this gives us enough precesion
// see the PRECISION define

void
mix(y4m_stream_info_t  *sinfo, uint8_t * const input[], uint32_t *output[], uint32_t percent, int field)
{

// ilace: 0 progressive. 1 bottom first. 2 top first

	int x,y;
	uint32_t c;
	int yinc, ystart;
	int width,height;	
	int cwidth,cheight;	
	int row_length,crow_length;

	height = y4m_si_get_plane_height(sinfo,0) ; width = y4m_si_get_plane_width(sinfo,0);
	cheight = y4m_si_get_plane_height(sinfo,1) ; cwidth = y4m_si_get_plane_width(sinfo,1);


	if (field == Y4M_ILACE_NONE) { yinc = 1; ystart = 0; }
	if (field == Y4M_ILACE_TOP_FIRST) { yinc = 2; ystart = 0; }
	if (field == Y4M_ILACE_BOTTOM_FIRST) { yinc = 2; ystart = 1; }

	for (y=ystart; y< height; y+=yinc) {
		row_length = y * width;
		crow_length = y * cwidth;
		
		for (x=0; x<width; x++)
		{
			output[0][x + row_length] += input[0][x + row_length] * percent;
			if ((x<cwidth) && (y<cheight)) {
				output[1][x + crow_length] += input[1][x + crow_length] * percent;
				output[2][x + crow_length] += input[2][x + crow_length] * percent;
			}	
		}
	}

	//fprintf (stderr,"Max output: %d    ",max);

}

// Convert the double precision frame into uint8_t
void
intise(uint8_t *output[], uint32_t * const input[], y4m_stream_info_t  *sinfo )
{
	int x;
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);

	for (x=0; x<fs; x++)
	{
		output[0][x] = (input[0][x] >> PRECISION);
		if (x<cfs) {
			output[1][x] = (input[1][x] >> PRECISION);
			output[2][x] = (input[2][x] >> PRECISION);
		}	
	}
}

// Black fill a double precision frame
void
black(uint32_t *output[], y4m_stream_info_t  *sinfo )
{

	int x;
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);

// could I use memset here?

		for (x=0; x<fs; x++)
		{
			output[0][x] = 0;
			if (x<cfs) {
				output[1][x] = 128;
				output[2][x] = 128;
			}	
		}

}

// compute greatest common divisor
// algorithm found on wikipedia.

int gcd (int a, int b) {

	int f,g;
	
	if ( a == 0 || b == 0) 
		return 1;

	g = b;
	f  =  a % b;
	
	while (f != 0) {
		b = f;
		f = g % f;
		g = b;
	}

	return g;
}

// calculate percent based on source and destination frame counters and frame lengths
// actual percent is calculated by result / 1 << PRECISION * 100

int calc_per_fc (int sc, int sld, int sln, int dc, int dld, int dln, int il) 
{

	uint32_t ss,se,ds,de;	

// prevent factors of 1 checked in parent function
/*
	if ((sld * dln == 1) || (dld * sln == 1)) {
		sld = sld << 1;
		dld = dld << 1;
	}
*/
		
// fprintf (stderr,"sc: %d sld: %d sln: %d dc: %d dld: %d dln: %d \n",sc,sld,sln,dc,dld,dln);

		ss = sld * dln * sc;
		se = sld * dln * (sc+1);
		ds = dld * sln * dc;
		de = dld * sln * (dc+1);

// if interlaced advance by half a frame

	if (il != Y4M_ILACE_NONE) {
		ss += (sld * dln) >> 1;
		se += (sld * dln) >> 1;
		ds += (dld * sln) >> 1;
		de += (dld * sln) >> 1;
	}

// do sanity checking

	if (se < ss) {
	// this is probably more fatal than warning
		fprintf (stderr,"WARNING: Source end is before start\n");
	}
	if (de < ds) {
	// this is probably more fatal than warning
		fprintf (stderr,"WARNING: Destination end is before start\n");
	}

//	fprintf (stderr,"per = src: %d-%d dst: %d-%d\n",ss,se,ds,de);

	if ((ss>ds) && (se<de)) 
		return (1<<PRECISION) * (se-ss)/(de-ds); 
		
	if ((ss<=ds) && (se<de) && (se>=ds)) 
		return (1<<PRECISION) * (se-ds)/(de-ds);
		
	if ((ss>ds) && (se>=de) && (ss<=de))
		return (1<<PRECISION) * (de-ss)/(de-ds);
		
	if ((ss<=ds) && (se>=de)) 
		return (1<<PRECISION);

	return 0;

}

int equals(int sc, int sld, int sln, int dc, int dld, int dln) {
	return sld * dln * sc == dld * sln * dc;
}

int greater(int il, int sc, int sic, int sld, int sln, int dc, int dld, int dln) {

//fprintf (stderr,"greater: %d*%d:%d %d*%d:%d\n",sc,sln,sld,dc,dln,dld);

// fprintf (stderr,"greater: %d >= %d\n",(sld +  sc * sld) * mul / sln,(dld +  dc * dld) * mul / dln);

	uint32_t ss,se,ds,de;

// prevent factors of 1 checked in parent function
/*
	if ((sld * dln == 1) || (dld * sln == 1)) {
		sld = sld << 1;
		dld = dld << 1;
	}
*/
	ss = sld * dln * sc;
	ds = dld * sln * dc;

// if interlaced advance by half a frame

	if (il != Y4M_ILACE_NONE) {
//	  fprintf (stderr,"greater: interlace ");
		se = sld * dln * sic + ((sld * dln) >> 1);
		de = ds + ((dld * sln) >> 1);
		
		return ss>= ds && se >= de;
	}

// fprintf (stderr,"greater: %d >= %d\n",ss,ds);
	
		return ss >= ds;
}

int smaller (int il,int sc, int sld, int sln, int dc, int dld, int dln) {


	uint32_t ss,se,ds,de;


// prevent factors of 1
// checked in parent function
/*
	if ((sld * dln == 1) || (dld * sln == 1)) {
		sld = sld << 1;
		dld = dld << 1;
	}
*/

	ss = sld * dln * sc;
	ds = dld * sln * dc;

// if interlaced advance by half a frame

	if (il != Y4M_ILACE_NONE) {
//		 fprintf (stderr,"smaller: interlace ");

		ss = ss + (sld * dln >> 1);
		ds = ds + (dld * sln >> 1);
	}

// fprintf (stderr,"smaller: %d <= %d\n",ss,ds);


		return ss <= ds;

}


static void resample(  int fdIn 
                      , y4m_stream_info_t  *inStrInfo
                      , y4m_ratio_t src_frame_rate
                      , int fdOut
                      , y4m_stream_info_t  *outStrInfo
                      , y4m_ratio_t dst_frame_rate
                      , int not_normalize 
                    )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	uint8_t            *yuv_idata[3] ;
	uint8_t		*yuv_odata[3] ;
	uint32_t		*yuv_fdata[3];
	int                frame_data_size, chroma_frame_data_size  ;
	int                read_error_code ;
	int                write_error_code ;
	int                src_frame_counter ;
	int                src_iframe_counter ;
	int                dst_frame_counter ;

	uint32_t ssrcf,sdstf,esrcf,edstf,per;
	uint32_t osrcf,odstf,nper,iper;
	int w,h,g,sc,dc;
	int interlaced;
	int sd,sn,dd,dn;

// Allocate memory for the YUV channels
	interlaced = y4m_si_get_interlace(inStrInfo);
	h = y4m_si_get_height(inStrInfo) ; 
	w = y4m_si_get_width(inStrInfo);
	
	frame_data_size = y4m_si_get_plane_length(inStrInfo,0);
	chroma_frame_data_size = y4m_si_get_plane_length(inStrInfo,1);

	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	if (chromalloc(yuv_odata,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");


	if (interlaced != Y4M_ILACE_NONE) {
		if (chromalloc(yuv_idata,inStrInfo))		
			mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	}
	
	yuv_fdata[0] = (uint32_t *)malloc(sizeof(uint32_t) * frame_data_size);
	yuv_fdata[1] = (uint32_t *)malloc(sizeof(uint32_t) * chroma_frame_data_size);
	yuv_fdata[2] = (uint32_t *)malloc(sizeof(uint32_t) * chroma_frame_data_size);

	if( !yuv_fdata[0] || !yuv_fdata[1] || !yuv_fdata[2] )
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	// source frame length, destination frame length.
//	srcfl = src_frame_rate.d  /  src_frame_rate.n;
//	dstfl = dst_frame_rate.d  /  dst_frame_rate.n;

  /* Initialize counters */

	write_error_code = Y4M_OK ;
	read_error_code = Y4M_OK ;

	src_frame_counter = 0 ;
	src_iframe_counter = 0 ;
	dst_frame_counter = 0 ;

	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo, &in_frame, yuv_data );

// reduce the rate by the greatest common divisor
// as we are interested in the ratios between the frame rates
// this keeps the resulting multiples within a managable range
// and not overflowing a uint32_t 

	sd = src_frame_rate.d;
	sn = src_frame_rate.n;
	dd = dst_frame_rate.d;
	dn = dst_frame_rate.n;
	

	g = gcd(sn,dn);
	if (g > 1) {
	//	fprintf (stderr,"Reducing rate by %d\n",g);
		sn /= g;
		dn /= g;
	}

	sc = sn * dd;
	dc = dn * sd;

	// although we've reduce to the lowest common divisor we don't want multipliers of 1
	// as we like to add half for interlaced streams...
	
	if (interlaced != Y4M_ILACE_NONE) {
		if (sc == 1 || dc == 1) {
			sn = sn << 1;
			dn = dn << 1;
		}
	}

	g = gcd (sc,dc);
	if (g > 1) {
		fprintf (stderr,"Reducing loop by %d\n",g);
		sc /= g;
		dc /= g;
	}
	
	fprintf (stderr,"integer frame loop: src: %d dst: %d\n",sc,dc);
	
	
	if (interlaced != Y4M_ILACE_NONE) 
		chromacpy (yuv_idata,yuv_data,inStrInfo);
	
	black(yuv_fdata,inStrInfo);
	nper = 0; iper = 0;

	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		//	fprintf (stderr,"frame counter %d,%d (iframe==%d)\n",sc,dc,src_iframe_counter);
		// reset counters back to 0 when they reach the calculated loop point
		
		if (src_frame_counter == sc && dst_frame_counter == dc) {
		//	fprintf (stderr,"Counter reset %d,%d (iframe==%d)\n",sc,dc,src_iframe_counter);
			src_iframe_counter -= src_frame_counter;
			src_frame_counter = 0;
			dst_frame_counter = 0;
		}

	// Do all types of frames here Interlaced or progressive


		if ((per = calc_per_fc(src_frame_counter,sd, sn,dst_frame_counter,dd, dn,0)) > 0 && (nper < (1<<PRECISION)-1)) {
			mix (inStrInfo,yuv_data,yuv_fdata,per,interlaced);
			nper += per;
		//	fprintf (stderr,"P NPER %d += PER %d\n",nper,per);

		}
			
			
	// for interlace frames, change the time offset and copy the remaining field
		if (interlaced != Y4M_ILACE_NONE) {
					
			if ((per = calc_per_fc(src_iframe_counter, sd, sn, dst_frame_counter, dd, dn, interlaced)) > 0 && (iper < (1<<PRECISION)-1)) {
	//				fprintf (stderr,"I PER %d += IPER %d\n",iper,per);
					
				if (interlaced == Y4M_ILACE_TOP_FIRST) 
					mix (inStrInfo,yuv_idata,yuv_fdata,per,Y4M_ILACE_BOTTOM_FIRST);
				else
					mix (inStrInfo,yuv_idata,yuv_fdata,per,Y4M_ILACE_TOP_FIRST);
							
				iper += per;

			}
		// mjpeg_warn ("Add percent: %f  src: %f-%f dst: %f-%f  ",per,ssrcf,esrcf,sdstf,edstf);
		}

// if the end of the source frame is passed the end of the destination frame, 
// then we write the destination frame

 // greater (src_frame_counter,src_frame_rate.d, src_frame_rate.n,dst_frame_counter,dst_frame_rate.d, dst_frame_rate.n)

	//	g = gcd ( src_frame_counter+1, dst_frame_counter+1);
	//	sc = (src_frame_counter+1) / g;
	//	dc = (dst_frame_counter+1) / g;

		if (greater(interlaced,src_frame_counter+1,src_iframe_counter+1, sd, sn, dst_frame_counter+1, dd,dn) ) {
		//		fprintf (stderr,"writing frame %d (%d) %d               \n",src_frame_counter,src_iframe_counter, dst_frame_counter);
				intise(yuv_odata,yuv_fdata,inStrInfo);
				write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );
				mjpeg_info( "Writing source frame %d at dest frame %d", src_frame_counter,dst_frame_counter );
		//		fprintf (stderr,"WRITING FRAME %d %g %g\n", dst_frame_counter,nper,iper);

				black(yuv_fdata,inStrInfo);
				nper = 0; iper = 0;
				dst_frame_counter++;
			
		} else { // Do not read any new frames if we have written a frame
		
			if (interlaced != Y4M_ILACE_NONE) { 
				
			// I'd like to investigate the occurance of interlace reads and copies vs progressive reads or copies
			// read the interlace frame if the end of the source frame is before the end of the destination
			
				if (smaller(interlaced,src_iframe_counter+1, sd, sn, dst_frame_counter+1, dd,dn)) {
					// copy the already read frame if we have it
					if (src_frame_counter > src_iframe_counter) {
						chromacpy (yuv_idata,yuv_data,inStrInfo);
					//	 fprintf (stderr,"COPYING I FRAME %d\n", src_iframe_counter);
					} else { // read the frame ourselves
						y4m_fini_frame_info( &in_frame );
						y4m_init_frame_info( &in_frame );
						read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_idata );
					//	fprintf (stderr,"READING I FRAME %d\n", src_iframe_counter);
					}
					src_iframe_counter++ ;
				}
			}
			 
			// read the frame if the end of the source frame is before the end of the destination

			if (smaller(Y4M_ILACE_NONE, src_frame_counter+1, sd, sn, dst_frame_counter+1, dd,dn)) {
			// copy the frame if the interlaced routine has read it already
				if ((interlaced != Y4M_ILACE_NONE) && (src_frame_counter < src_iframe_counter)) {
					chromacpy (yuv_data,yuv_idata,inStrInfo);
					// fprintf (stderr,"COPYING P FRAME %d\n", src_frame_counter);
				} else {
					y4m_fini_frame_info( &in_frame );
					y4m_init_frame_info( &in_frame );
					read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
				//	 fprintf (stderr,"READING P FRAME %d\n", src_frame_counter);
				}
			
				src_frame_counter++ ;
			}
		}
	}
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );

	if (interlaced != Y4M_ILACE_NONE) {
		free( yuv_idata[0] );
		free( yuv_idata[1] );
		free( yuv_idata[2] );
	}

	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );
	free( yuv_odata[0] );
	free( yuv_odata[1] );
	free( yuv_odata[2] );
	free( yuv_fdata[0] );
	free( yuv_fdata[1] );
	free( yuv_fdata[2] );

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
  int change_header_only = 0 ;
  int not_normalize = 0;
  int have_src_framerate = 0;
  int have_framerate = 0;
  int yuv_interlacing = Y4M_UNKNOWN;
  int fdIn = 0 ;
  int fdOut = 1 ;
  y4m_stream_info_t in_streaminfo, out_streaminfo ;
  y4m_ratio_t frame_rate, src_frame_rate ;

  const static char *legal_flags = "I:r:s:cnv:h";
  int c ;

  while ((c = getopt (argc, argv, legal_flags)) != -1) {
    switch (c) {
      case 'v':
        verbose = atoi (optarg);
        if (verbose < 0 || verbose > 2)
          mjpeg_error_exit1 ("Verbose level must be [0..2]");
        break;
		case 'I':
			switch (optarg[0]) {
				case 'p':  yuv_interlacing = Y4M_ILACE_NONE;  break;
				case 't':  yuv_interlacing = Y4M_ILACE_TOP_FIRST;  break;
				case 'b':  yuv_interlacing = Y4M_ILACE_BOTTOM_FIRST;  break;
                      default:
                        mjpeg_error("Unknown value for interlace: '%c'", optarg[0]);
                        return -1;
                        break;
                }
				break;
        case 'h':
        case '?':
          print_usage (argv);
          return 0 ;
          break;
        // New frame rate
        case 'r':
          if( Y4M_OK != y4m_parse_ratio(&frame_rate, optarg) )
              mjpeg_error_exit1 ("Syntax for frame rate should be Numerator:Denominator");
	mjpeg_warn( "New Frame rate %d:%d", frame_rate.n,frame_rate.d  );
	have_framerate =1 ;
          break;
        // Assumed frame rate for source (useful when the header contains an
        // invalid frame rate)
        case 's':
          if( Y4M_OK != y4m_parse_ratio(&src_frame_rate,optarg) )
              mjpeg_error_exit1 ("Syntax for frame rate should be Numerator:Denominator");
		have_src_framerate = 1;
          break ;
        // Only change header frame-rate, not the stream itself
        case 'c':
          change_header_only = 1 ;
        case 'n':
          not_normalize = 1;
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

  // Prepare output stream
	if (!have_src_framerate)
		src_frame_rate = y4m_si_get_framerate( &in_streaminfo );
	if (!have_framerate)
		frame_rate = src_frame_rate ;
  y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );

  optind = 0;
  
  // Information output
  mjpeg_info ("yuvafps (version " YUVFPS_VERSION
              ") is a linear interpolating frame resampling utility for yuv streams");
  mjpeg_info ("(C) 2007 Mark Heath mjpeg1 at silicontrip dot org");
  mjpeg_info ("yuvafps -h for help");

  y4m_si_set_framerate( &out_streaminfo, frame_rate );                
  y4m_write_stream_header(fdOut,&out_streaminfo);
  
  if (yuv_interlacing != Y4M_UNKNOWN) 
	y4m_si_set_interlace( &in_streaminfo, yuv_interlacing);

  if( change_header_only )
    frame_rate = src_frame_rate ;
    
  /* in that function we do all the important work */
  resample( fdIn,&in_streaminfo, src_frame_rate, fdOut,&out_streaminfo, frame_rate, not_normalize );

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
