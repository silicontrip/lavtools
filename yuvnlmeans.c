/*
 *  yuvbilateral.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
 *
 *  an NL-means spacial filter
 *
 *  NL-means filter based on code from:
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
#include <float.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "utilyuv.h"

#define VERSION "0.1"

#define PRECISION 256


#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define ABS(a) ((a) > 0 ? (a) : (-(a)))

#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)


struct parameters {

	unsigned int sigmaD;
	unsigned int sigmaR;
	
	unsigned int kernelRadius;
	
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
			 "usage: yuvbilateral -r sigmaR -d sigmaD [-v 0..2]\n"
			 "\t -r sigmaR set the similarity distance"
			 "\t -r sigmaD set the temporal/spacial distance"

			);
}

unsigned int gauss (unsigned int sigma, int x, int y) {
	return exp(-((x * x + y * y) / (2.0 * (1.0* sigma/PRECISION) * (1.0*sigma/PRECISION)))) * PRECISION;
}

unsigned int similarity(int p, int s) {
	// this equals: Math.exp(-(( Math.abs(p-s)) /  2 * this.sigmaR * this.sigmaR));
	// but is precomputed to improve performance
	if (this.direction == 0) 
		return this.gaussSimilarity[abs(p-s)];
	
	return this.gaussSimilarity[255-abs(p-s)];

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
	if (totalWeight > 0 )
	o[j * w + i] = (sum / totalWeight);
	else
	o[j * w + i] = intensityCenter;
}


static void filterframe (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si, double *dssum, double *dsweight, double *dswmax, int Ax, int Ay, int Sx, int Sy, int h, double *gw)
{
	
	int b;
	
	int Sxd = Sx*2+1;
	int Syd = Sy*2+1;
	
	double 	h2in = -1.0/(h*h);
	double hin = -1.0/h;
	
	
	//fprintf (stderr,"trace: filterframe in\n");
	
	for (b=0; b<3; ++b)
	{
		
		const unsigned char *srcp = n[b];
		const unsigned char *pfp = n[b];
		unsigned char *dstp = m[b];
		const int height = y4m_si_get_plane_height(si,b);
		const int heightm1 = height-1;
		const int width = y4m_si_get_plane_width(si,b);
		const int widthm1 = width-1;
		const int pitch = width ;

		memset(dssum,0,height * width * sizeof(double));
		memset(dsweight,0,height * width * sizeof(double));
		memset(dswmax,0,height * width * sizeof(double));

		int y;
	//	fprintf (stderr,"trace: filterframe y\n");

		for (y=0; y < height; ++y) {
			
			const int stopy = MIN(y+Ay,heightm1);
			const int doffy = y*width;
			int x;
			
		//	fprintf (stderr,"trace: filterframe b=%d y=%d\n",b,y);

			for (x=0; x < width; ++x) {
				
				const int startxt = MAX(x-Ax,0);
				const int stopx = MIN(x+Ax,widthm1);
				const int doff = doffy+x;
				
			//	fprintf (stderr,"trace: filterframe doff=%d\n",doff);

				
				double *dsum = &dssum[doff];
				double *dweight = &dsweight[doff];
				double *dwmax = &dswmax[doff];
				
				int u;
// fprintf (stderr,"trace: filterframe u in\n");

				for (u=y; u<=stopy; ++u)
				{
					const int startx = u == y ? x+1 : startxt;
					const int yT = -MIN(MIN(Sy,u),y);
					const int yB = MIN(MIN(Sy,heightm1-u),heightm1-y);
					const unsigned char *s1_saved = pfp + (u+yT)*pitch;
					const unsigned char *s2_saved = pfp + (y+yT)*pitch + x;
					const double *gw_saved = gw+(yT+Sy)*Sxd+Sx;
					const int pfpl = u*pitch;
					const int coffy = u*width;
					
					int v;
				//	fprintf (stderr,"trace: filterframe v in\n");

					for ( v=startx; v<=stopx; ++v)
					{
						const int coff = coffy+v;
						double *csum = &dssum[coff];
						double *cweight = &dsweight[coff];
						double *cwmax = &dswmax[coff];
						const int xL = -MIN(MIN(Sx,v),x);
						const int xR = MIN(MIN(Sx,widthm1-v),widthm1-x);
						const unsigned char *s1 = s1_saved + v;
						const unsigned char *s2 = s2_saved;
						const double *gwT = gw_saved;
						double diff = 0.0, gweights = 0.0;
						
						int j;
				//		fprintf (stderr,"trace: filterframe j in\n");

						for (j=yT; j<=yB; ++j)
						{
							
							int k;
						//	fprintf (stderr,"trace: filterframe k in\n");

							for ( k=xL; k<=xR; ++k)
							{
								diff += ABS(s1[k]-s2[k])*gwT[k];
								gweights += gwT[k];
							}
						//	fprintf (stderr,"trace: filterframe k out\n");

							s1 += pitch;
							s2 += pitch;
							gwT += Sxd;
						}
						const double weight = exp((diff/gweights)*hin);
						*cweight += weight;
						*dweight += weight;
						*csum += srcp[x]*weight;
						*dsum += pfp[pfpl+v]*weight;
						if (weight > *cwmax) *cwmax = weight;
						if (weight > *dwmax) *dwmax = weight;
					}
				}
					const double wmax = *dwmax <= DBL_EPSILON ? 1.0 : *dwmax;
					*dsum += srcp[x]*wmax;
					*dweight += wmax;
					dstp[x] = MAX(MIN((int)(((*dsum)/(*dweight))+0.5),255),0);
					
				}

				dstp += pitch;
				srcp += pitch;
				
			
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
	
	
	int Ax,Ay,Bx,By,Sx,Sy,a,h;
	
	// Window size
	// trial values, will move to Command line arguments
	a = 2;
	h = 1;
	
	Ax = 4; 
	Ay = 4;
	
	Sy = 4;
	Sx = 4;
	
	Bx = 4;
	By = 4;
	
	int Sxd = Sx*2+1;
	int Syd = Sy*2+1;
	
	int Sxa = Sxd*Syd;
	int Bxd = Bx*2+1;
	int Byd = By*2+1;
	int Bxa = Bxd*Byd;
	int Axd = Ax*2+1;
	int Ayd = Ay*2+1;
	int Axa = Axd*Ayd;
	// Azdm1 = Az*2;
	int a2 = a*a;
	
	
	const int Sxh = (Sx+1)>>1;
	const int Syh = (Sy+1)>>1;
	const int Sxdh = Sxh*2+1;
	const int Sydh = Syh*2+1;
	const int Bxh = (Bx+1)>>1;
	const int Byh = (By+1)>>1;
	
	
	
	double *gwh = (double*)malloc(Sxdh*Sydh*sizeof(double));
	int w = 0, m, n;
	int j;
	for ( j=-Syh; j<=Syh; ++j)
	{
		if (j < 0) m = MIN(j+Byh,0);
		else m = MAX(j-Byh,0);
		int k;
		for ( k=-Sxh; k<=Sxh; ++k)
		{
			if (k < 0) n = MIN(k+Bxh,0);
			else n = MAX(k-Bxh,0);
			gwh[w++] = exp(-((m*m+n*n)/(2*a2)));
		}
	}
	
	int framesize = y4m_si_get_plane_length(inStrInfo,0);
	
	double *dsum = (double *)malloc(framesize*sizeof(double));
	double *dweight = (double *)malloc(framesize*sizeof(double));
	double *dwmax = (double *)malloc(framesize*sizeof(double));
	
	double *gw = (double*)malloc(Sxd*Syd*sizeof(double));
	
//	int w = 0, m, n;
	// int j;
	for ( j=-Sy; j<=Sy; ++j)
	{
		if (j < 0) m = MIN(j+By,0);
		else m = MAX(j-By,0);
		int k;
		for ( k=-Sx; k<=Sx; ++k)
		{
			if (k < 0) n = MIN(k+Bx,0);
			else n = MAX(k-Bx,0);
			gw[w++] = exp(-((m*m+n*n)/(2*a2)));
		}
	}
	
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			// static void filterframe (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si, double *dsum, double *dweight, double *dwmax, int Ax, int Ay, int Sx, int Sy, double *gw)

			
			filterframe(yuv_odata,yuv_data,inStrInfo,dsum,dweight,dwmax,Ax,Ay,Sx,Sy,h,gw);
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
	const static char *legal_flags = "v:hr:d:i";
	
	float sigma;
	
	this.sigmaR = 0;
	this.sigmaD = 0;
	this.direction = 0;
	
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
			case 'i':
				this.direction = 1;
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
	mjpeg_info ("yuvbilateral (version " VERSION ") is a spatial bilateral filter for yuv streams");
	mjpeg_info ("(C) 2010 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info ("yuvbilateral -h for help");
	
    
	/* in that function we do all the important work */
//	filterinitialize ();
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
