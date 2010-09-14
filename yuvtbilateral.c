/*
 *  generic.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
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

#define PRECISION 1024

struct parameters {
	
	unsigned int sigmaD;
	unsigned int sigmaR;
	
	unsigned int kernelRadius;
	unsigned int kernelSize;
	
	
	unsigned int *kernelD;
	unsigned int *kernelR;
	
	unsigned int *gaussSimilarity;
	
	unsigned int twoSigmaRSquared;
	
	int direction;
	
};

static struct parameters this;


static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuv\n"
			 );
}


unsigned int gauss (unsigned int sigma, int x, int y) {
	
	// fprintf (stderr,"%f , %d %d\n", (1.0* sigma/PRECISION) ,x,y);
	
	//float sigmaf =  (1.0* sigma/PRECISION)
	
	return (exp(-((1.0 * x * x) / (2.0 * (1.0* sigma/PRECISION) * (1.0*sigma/PRECISION))))) * PRECISION;
}

unsigned int similarity(int p, int s) {
	// this equals: Math.exp(-(( Math.abs(p-s)) /  2 * this.sigmaR * this.sigmaR));
	// but is precomputed to improve performance
		return this.gaussSimilarity[abs(p-s)];
		
}

static void filterinitialize () {
	
	// int center;
	int x,y,i;
	
	this.kernelRadius = this.sigmaD>this.sigmaR?this.sigmaD * 2:this.sigmaR * 2;
	this.kernelRadius = this.kernelRadius / PRECISION;
	
	this.twoSigmaRSquared = (2 * (1.0 *this.sigmaR/PRECISION)  * (1.0 *this.sigmaR/PRECISION)) * PRECISION;
	
	this.kernelSize = this.kernelRadius * 2 + 1;
	// center = (this.kernelSize - 1) / 2;
	
	
	this.kernelD = (unsigned int*) malloc( sizeof (unsigned int) * this.kernelSize );
	
	if (this.kernelD == NULL ){
		mjpeg_error_exit1("Cannot allocate memory for filter kernel");
	}
	
fprintf(stderr,"size %d, radius %d \n",this.kernelSize,this.kernelRadius);

	// x = -center;
	//fprintf (stderr,"x %d < %d\n",x,this.kernelSize - center);

	
	for ( x=0; x < this.kernelSize ; x++) {
	//	fprintf (stderr,"x %d\n",x);
		this.kernelD[x] = gauss(this.sigmaD, x-this.kernelRadius, 0);
	fprintf(stderr,"x: %d  = %d\n",x,this.kernelD[x]);
	}
	
	// fprintf (stderr,"gauss Similarity malloc\n",i);

	this.gaussSimilarity = (unsigned int*) malloc(sizeof (unsigned int) * 256);
	if (this.gaussSimilarity == NULL ){
		free(this.kernelD);
		mjpeg_error_exit1("Cannot allocate memory for gaussian curve");
	}
	
	// precomute all possible similarity values for
	// performance reasons
	for ( i = 0; i < 256; i++) {
		//fprintf (stderr,"i %d\n",i);
		this.gaussSimilarity[i] = exp(-((i) / (1.0 * this.twoSigmaRSquared/PRECISION))) * PRECISION;
	}
	
	
}



static void filterpixel(uint8_t **o, uint8_t ***p,int chan, int i, int j, int w, int h) {
	
	unsigned int sum =0;
	unsigned int totalWeight = 0;
	int weight;
	int z;
	
	uint8_t intensityCenter = p[this.kernelRadius][chan][j * w + i];
	
	for ( z = 0; z < this.kernelSize; z++) {
		
		int intensityKernelPos = p[z][chan][i + j * w];
		
		//fprintf (stderr,"kernel D %d. kp %d cen %d\n",this.kernelD[z],intensityKernelPos,intensityCenter);
		
		weight = this.kernelD[z] * similarity(intensityKernelPos,intensityCenter);
		totalWeight += weight;
		sum += (weight * intensityKernelPos);
		
	}
//	fprintf(stderr,"total %d\n",totalWeight);
	if (totalWeight > 0 )
		o[chan][i+j*w] = sum / totalWeight;
	else
		o[chan][i+j*w] = intensityCenter;
	
}

static void filterframe (uint8_t *m[3], uint8_t ***n, y4m_stream_info_t *si)
{
	
	int x,y;
	int height,width,height2,width2;
	
	height=y4m_si_get_plane_height(si,0);
	width=y4m_si_get_plane_width(si,0);
	
	// I'll assume that the chroma subsampling is the same for both u and v channels
	height2=y4m_si_get_plane_height(si,1);
	width2=y4m_si_get_plane_width(si,1);
	
	
	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
					
			filterpixel(m,n,0,x,y,width,height);
			
			if (x<width2 && y<height2) {
				filterpixel(m,n,1,x,y,width2,height2);
				filterpixel(m,n,2,x,y,width2,height2);
			}
			
		}
	}
	
}


static void filter(  int fdIn ,int fdOut , y4m_stream_info_t  *inStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            ***yuv_data;
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int c,d;
	
	// Allocate memory for the YUV channels
	
	
	
	if (chromalloc(yuv_odata,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	// sanity check!!!
	yuv_data= (uint8_t ***) malloc(sizeof (uint8_t *) * this.kernelSize);
	if (yuv_data == NULL) {
		chromafree(yuv_odata);
		mjpeg_error_exit1("cannot allocate memory for frame buffer");
	}
	for (c=0;c<this.kernelSize;c++) {
		yuv_data[c] = (uint8_t **) malloc(sizeof (uint8_t *) * 3);
		if (yuv_data[c] == NULL) {
			// how am I going to keep track of what I've allocated?
			mjpeg_error_exit1("cannot allocate memory for frame buffer");
		}
		if(chromalloc(yuv_data[c],inStrInfo)) {
			mjpeg_error_exit1("cannot allocate memory for frame buffer");
		}
	}
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[this.kernelSize-1] );
	
	//init loop mode
	for (c=this.kernelSize-1;c>0;c--) {
		chromacpy(yuv_data[c-1],yuv_data[c],inStrInfo);
	}
	
	for(d=1;d<this.kernelRadius;d++) {
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[this.kernelSize-1] );
		// shuffle
		// should use pointers...
		for (c=0;c<this.kernelSize-1;c++) {
			chromacpy(yuv_data[c],yuv_data[c+1],inStrInfo);
		}
		
	}
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			filterframe(yuv_odata,yuv_data,inStrInfo);
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data[this.kernelSize-1] );
		// shuffle
		// should use pointers...
		for (c=0;c<this.kernelSize-1;c++) {
			chromacpy(yuv_data[c],yuv_data[c+1],inStrInfo);
		}
		
	}
	// finalise loop		
	for(c=0;c<this.kernelRadius;c++) {
		if (read_error_code == Y4M_OK) {
			filterframe(yuv_odata,yuv_data,inStrInfo);
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_odata );
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		
		// shuffle
		// should use pointers...
		for (c=0;c<this.kernelSize-1;c++) {
			chromacpy(yuv_data[c],yuv_data[c+1],inStrInfo);
		}
		
	}
	
	
	// Clean-up regardless an error happened or not
	
	
	y4m_fini_frame_info( &in_frame );
	chromafree(yuv_odata);
	
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
	float sigma;
	int c ;
	const static char *legal_flags = "?hv:r:d:";
	
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
	mjpeg_info ("yuvtbilateral (version " VERSION ") is a general deinterlace/interlace utility for yuv streams");
	mjpeg_info ("(C) 2010 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info (" -h for help");
	
	
	/* in that function we do all the important work */
	filterinitialize ();
	y4m_write_stream_header(fdOut,&in_streaminfo);

	filter(fdIn, fdOut, &in_streaminfo);
	
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
