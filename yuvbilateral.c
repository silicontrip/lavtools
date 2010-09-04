/*
 *  yuvbilateral.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
 *
 * a spacial bilateral filter
 *
 *  Bilateral filter  based on code from:
 *  http://user.cs.tu-berlin.de/~eitz/bilateral_filtering/
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
#include <math.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "utilyuv.h"

#define VERSION "0.1"

#define PRECISION 256

struct parameters {

	unsigned int sigmaD;
	unsigned int sigmaR;
	
	unsigned int kernelRadius;
	
	unsigned int *kernelD;
	unsigned int *kernelR;
	
	unsigned int *gaussSimilarity;
	
	unsigned int twoSigmaRSquared;
	
	
};

static struct parameters this;

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvbilateral -r sigmaR -d sigmaD [-v 0..2]\n"
			 "\t -r sigmaR set the "
			 "\t -r sigmaD set the "

			);
}

unsigned int gauss (unsigned int sigma, int x, int y) {
	return exp(-((x * x + y * y) / (2.0 * (1.0* sigma/PRECISION) * (1.0*sigma/PRECISION)))) * PRECISION;
}

unsigned int similarity(int p, int s) {
	// this equals: Math.exp(-(( Math.abs(p-s)) /  2 * this.sigmaR * this.sigmaR));
	// but is precomputed to improve performance
	return this.gaussSimilarity[abs(p-s)];
}

static void filterinitialize () {

	int kernelSize, center;
	int x,y,i;
	
	this.kernelRadius = this.sigmaD>this.sigmaR?this.sigmaD * 2:this.sigmaR * 2;
	this.kernelRadius = this.kernelRadius / PRECISION;
	
	this.twoSigmaRSquared = (2 * (1.0 *this.sigmaR/PRECISION)  * (1.0 *this.sigmaR/PRECISION)) * PRECISION;
	
	kernelSize = this.kernelRadius * 2 + 1;
	center = (kernelSize - 1) / 2;
	
	
	this.kernelD = (unsigned int*) malloc( sizeof (unsigned int) * kernelSize * kernelSize);
	
	if (this.kernelD == NULL ){
		mjpeg_error_exit1("Cannot allocate memory for filter kernel");
	}
		
	
	for ( x = -center; x < -center + kernelSize; x++) {
		for ( y = -center; y < -center + kernelSize; y++) {
			this.kernelD[x + center + (y + center) * kernelSize] = gauss(this.sigmaD, x, y);
			//fprintf(stderr,"x: %d y: %d = %d\n",x,y,this.kernelD[x + center + (y + center) * kernelSize]);
		}
	}
	
	this.gaussSimilarity = (unsigned int*) malloc(sizeof (unsigned int) * 256);
	if (this.gaussSimilarity == NULL ){
		free(this.kernelD);
		mjpeg_error_exit1("Cannot allocate memory for gaussian curve");
	}
	
	// precomute all possible similarity values for
	// performance reasons
	for ( i = 0; i < 256; i++) {
		this.gaussSimilarity[i] = exp(-((i) / (1.0 * this.twoSigmaRSquared/PRECISION))) * PRECISION;
	}
	
	
}

static void filterpixel(uint8_t *o, uint8_t *p, int i, int j, int w, int h) {

	unsigned int sum =0;
	unsigned int totalWeight = 0;
	int weight;
	int m,n;
	
	uint8_t intensityCenter = p[j * w + i];
	int mMax = i + this.kernelRadius;
	int nMax = j + this.kernelRadius;
	
	for ( m = i-this.kernelRadius; m < mMax; m++) {
		for ( n = j-this.kernelRadius; n < nMax; n++) {
			
			if (m>=0 && n>=0 && m < w && n < h) {
				int intensityKernelPos = p[m + n * w];
				
				weight = this.kernelD[(i-m + this.kernelRadius) + (j-n + this.kernelRadius) * (this.kernelRadius*2)] * similarity(intensityKernelPos,intensityCenter);
				totalWeight += weight;
				sum += (weight * intensityKernelPos);
			}
		}
	}
	
	o[j * w + i] = (sum / totalWeight);
	//o[j * w + i] = intensityCenter;
}


static void filterframe (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si)
{
	
	int x,y;
	int height,width,height2,width2;
	
	height=y4m_si_get_plane_height(si,0);
	width=y4m_si_get_plane_height(si,0);

	// I'll assume that the chroma subsampling is the same for both u and v channels
	height2=y4m_si_get_plane_height(si,1);
	width2=y4m_si_get_plane_width(si,1);

	
	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			
			filterpixel(m[0],n[0],x,y,width,height);
			
			if (x<width2 && y<height2) {
				filterpixel(m[1],n[1],x,y,width2,height2);
				filterpixel(m[2],n[2],x,y,width2,height2);
			}
			 
		}
	}
	
}

static void filter(int fdIn, int fdOut, y4m_stream_info_t  *inStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,inStrInfo)){
		chromafree(yuv_data);
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	}
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			filterframe(yuv_odata,yuv_data,inStrInfo);
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	}
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	chromafree( yuv_data );
	chromafree( yuv_odata);
	
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
	const static char *legal_flags = "v:hr:d:";
	
	float sigma;
	
	this.sigmaR = 0;
	this.sigmaD = 0;
	
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
			case 'r':
				sigma = atof(optarg);
				this.sigmaR = sigma * PRECISION;
				break;
			case 'd':
				sigma = atof(optarg);
				this.sigmaD = sigma * PRECISION;
				break;
				
		}
	}
	
	if (this.sigmaR == 0 || this.sigmaD == 0) {
		print_usage();
		mjpeg_error_exit1("Sigma D and R must be set");
		
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
	mjpeg_info ("yuvbilateral (version " VERSION ") is a spatial bilateral filter for yuv streams");
	mjpeg_info ("(C) 2010 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info ("yuvbilateral -h for help");
	
    
	/* in that function we do all the important work */
	filterinitialize ();
	y4m_write_stream_header(fdOut,&in_streaminfo);
	filter(fdIn,fdOut, &in_streaminfo);
	y4m_fini_stream_info (&in_streaminfo);
	//filteruninitialize();
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
