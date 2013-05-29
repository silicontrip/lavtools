/*
  *  yuvhsync.c
** <p>attempts to align horizontal sync drift.
** work in progress </p>
  *
  *  modified from yuvadjust.c by
  *  Copyright (C) 2010 Mark Heath
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
#include "utilyuv.h"

static void print_usage() 
{
  fprintf (stderr,
           "usage: yuvhsync -m <max shift> -s <search length>\n"
           "\n"
           "-v\tVerbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
           "-m\tMaximum number of pixels to shift\n"
           "-s\tCompare this number of pixels when determining shift\n"
           "-n\tDo not shift video\n"
		);
}

void shift_video (int s, int line, uint8_t *yuv_data[3],y4m_stream_info_t *sinfo)
{
	
	int x,w,cw,cs;
	int vss;
    int linew,linecw;
	
//	mjpeg_debug("shift_video %d %d",s,line);
	

		w = y4m_si_get_plane_width(sinfo,0);
		cw = y4m_si_get_plane_width(sinfo,1);
		
		vss = y4m_si_get_plane_height(sinfo,0) / y4m_si_get_plane_height(sinfo,1);
		
		cs = s * cw / w;
	
    linew = line * w;
    linecw = line * cw;
	// could memcpy() do this?
		if (s>0) {
			for (x=w; x>=s; x--)
				*(yuv_data[0]+x+(linew))= *(yuv_data[0]+(x-s)+(linew));
		
		// one day I should start catering for more or less than 3 planes.
			if (line % vss) {
				for (x=cw; x>=cs; x--) {
					*(yuv_data[1]+x+linecw)= *(yuv_data[1]+(x-cs)+linecw);
					*(yuv_data[2]+x+linecw)= *(yuv_data[2]+(x-cs)+linecw);
				}
			}
		}
		if (s<0) {
			for (x=0; x<=w+s; x++)
				*(yuv_data[0]+x+(linew))= *(yuv_data[0]+(x-s)+(linew));
			
			
			// one day I should start catering for more or less than 3 planes.
			if (line % vss) {
				for (x=0; x<=cw; x++) {
					*(yuv_data[1]+x+(linecw))= *(yuv_data[1]+(x-cs)+(linecw));
					*(yuv_data[2]+x+(linecw))= *(yuv_data[2]+(x-cs)+(linecw));
				}
			}
			
		}
//	mjpeg_debug("exit shift_video %d %d",s,line);

}

int search_video (int m, int s, int line, uint8_t *yuv_data[3],y4m_stream_info_t *sinfo) 
{

	int w,x1,diff;
	int max,c=0,linew;
	
	w = y4m_si_get_plane_width(sinfo,0);
	
    linew= w * line;

    // I'm not quite sure what I'm attempting to do here.
    // I think I'm trying to find the black frame.
	for (x1=0;x1<m;x1++) 
	{
    }
	//	fprintf(stderr," %d",diff);
	}
//	fprintf (stderr,"\n");
	return c;
}

// this method isn't too effective
int search_video_1 (int m, int s, int line, uint8_t *yuv_data[3],y4m_stream_info_t *sinfo) 
{

	int w,h;
	int x1,x2;
	int min,shift,tot;
	int linew, line1w;

    int ilace = y4m_si_get_interlace(sinfo);
	
	w = y4m_si_get_plane_width(sinfo,0);
	h = y4m_si_get_plane_height(sinfo,0);

	linew = line * w;
    
    if (ilace == Y4M_ILACE_NONE)
        line1w = (line+1) * w ;
    else
        line1w = (line+2) * w;

    line1w = (line+2) * w;

    
	mjpeg_debug("search_video %d",line);

    // 2 or 1 dependent on interlace or not.
	if (line+2 > h) {
		mjpeg_warn("line > height");
		return 0;
	}
    
	shift = 0;
	for (x1=-m;x1<m;x1++) 
	{
		tot = 0;
		for(x2=0; x2<s;x2++) 
		{
			// don't know if I should apply a standard addition to pixels outside the box.
			if (x1+x2 >=0 && x1+x2 < w) 
				tot += abs ( *(yuv_data[0]+x1+x2+linew) - *(yuv_data[0]+x2+line1w));
			else
				tot += 128;
		}
	
		// ok it wasn't max afterall, it was min.
		if (x1==0) min = tot;
		if (tot < min) { min = tot; shift = x1;}
	}
	
	mjpeg_debug("exit search_video %d",line);

	return shift;
}

static void process(  int fdIn , y4m_stream_info_t  *inStrInfo,
	int fdOut, y4m_stream_info_t  *outStrInfo,
	int max,int search, int noshift)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3],*yuv_odata[3];	
	// int result[720]; // will change to malloc based on max shift
	int *lineresult;
	int                y_frame_data_size, uv_frame_data_size ;
	int                read_error_code  = Y4M_OK;
	int                write_error_code = Y4M_OK ;
	int                src_frame_counter ;
	int x,y,w,h,cw,ch;

	h = y4m_si_get_plane_height(inStrInfo,0);
	ch = y4m_si_get_plane_height(inStrInfo,1);

    lineresult = (int *) malloc(sizeof(int) * h);
    
	chromalloc(yuv_data,inStrInfo);
	
// initialise and read the first number of frames
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		for (y=0; y<h-1; y++) 
			lineresult[y] = search_video_1(max,search,y,yuv_data,inStrInfo);
			
        
        if (noshift) {
			/* graphing this would be nice */
			printf ("%d",y);
			for (x=0; x < h; x++)
				printf (", %d",lineresult[x]);
			printf("\n");
		
        } else {
            
            int shifter = 0;
            for (y=0; y<h-1; y++) {
                shifter += lineresult[y];
                shift_video(shifter,y,yuv_data,inStrInfo);
                
            }
            write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data );
        }
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );
		++src_frame_counter ;
	}

  // Clean-up regardless an error happened or not

    y4m_fini_frame_info( &in_frame );

    free (lineresult);
    chromafree(yuv_data);
	
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
	const static char *legal_flags = "v:m:s:n";
	int max_shift = 0, search = 0;
    int noshift=0;
	int c;

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
    case 'n':
            noshift=1;
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

  /* in that function we do all the important work */
    if (!noshift)
        y4m_write_stream_header(fdOut,&out_streaminfo);

	process( fdIn,&in_streaminfo,fdOut,&out_streaminfo,max_shift,search,noshift);

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
