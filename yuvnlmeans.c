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

	int Ax,Ay,Bx,By,Sx,Sy,a,h;
	
	int iDWin;
	int iDBloc;
	float fSigma;
	float fFiltPar;
	
	// want to change these into fixed precision
	int *gwh;
	int *dsum;
	int *dweight;
	int *dwmax;
	int *gw;
	int *sumsb;
	int *weightsb;
};

static struct parameters this;

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvbilateral [-v 0..2]\n"
			 "\t -w window size (1)"
			 "\t -b search window size (10)"
			 "\t -s Sigma (1.0)"
			 "\t -f Filtering (0.55)"

			);
}

unsigned int gauss (unsigned int sigma, int x, int y) {
	return exp(-((x * x + y * y) / (2.0 * (1.0* sigma/PRECISION) * (1.0*sigma/PRECISION)))) * PRECISION;
}



static void filteruninitialize() {
	
	free(this.gwh);
	free(this.gw);
	free(this.dsum);
	free(this.dweight);
	free(this.dwmax);
}
static void filterinitialize (int Ax, int Ay, int Bx, int By, int Sx, int Sy, int a, int h, y4m_stream_info_t  *inStrInfo ) {

	
	// Window size
	// trial values, will move to Command line arguments
	this.a = a;
	this.h = h;
	
	this.Ax = Ax; 
	this.Ay = Ay;
	
	this.Sy = Sy;
	this.Sx = Sx;
	
	this.Bx = Bx;
	this.By = By;
	
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
	
	
	// gwh doesn't appear to be used anywhere.
	this.gwh = (int*)malloc(Sxdh*Sydh*sizeof(int));
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
			this.gwh[w++] = exp(-((m*m+n*n)/(2*a2))) * PRECISION;
		}
	}
	
	int framesize = y4m_si_get_plane_length(inStrInfo,0);
	
	this.dsum = (int *)malloc(framesize*sizeof(int));
	this.dweight = (int *)malloc(framesize*sizeof(int));
	this.dwmax = (int *)malloc(framesize*sizeof(int));
	
	this.gw = (int*)malloc(Sxd*Syd*sizeof(int));
	
	w = 0;
	//int m, n;
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
			
			
			this.gw[w++] = exp(-((m*m+n*n)/(2*a2))) * PRECISION;
			//fprintf (stderr,"gw: 
		}
	}
	
	if (Bx || By)
	{
		this.sumsb = (int *)malloc(Bxa*sizeof(int));
	//	if (!sumsb) env->ThrowError("TNLMeans:  malloc failure (sumsb)!");
		this.weightsb = (int *)malloc(Bxa*sizeof(int));
	//	if (!weightsb) env->ThrowError("TNLMeans:  malloc failure (weightsb)!");
	}
	
	
}

#define LUTPRECISION 1000.0
#define LUTMAX 30.0
#define LUTMAXM1 29.0
#define fTiny 0.00000001f

void  wxFillExpLut(float *lut, int size) {
	int i;
    for (i=0; i< size; i++)   lut[i]=   expf( - (float) i / LUTPRECISION);
}

float wxSLUT(float dif, float *lut) {
	
    if (dif >= (float) LUTMAXM1) return 0.0;
	
    int  x= (int) floor( (double) dif * (float) LUTPRECISION);
	
    float y1=lut[x];
    float y2=lut[x+1];
	
    return y1 + (y2-y1)*(dif*LUTPRECISION -  x);
}


int fiL2IntDist(uint8_t *u0,uint8_t *u1,int i0,int j0,int i1,int j1,int radius,int width0, int width1) {
	
    int dist=0;
	int s;
    for (s=-radius; s<= radius; s++) {
		
        int l = (j0+s)*width0 + (i0-radius);
        uint8_t *ptr0 = &u0[l];
		
        l = (j1+s)*width1 + (i1-radius);
        uint8_t *ptr1 = &u1[l];
		
		int r;
        for (r=-radius; r<=radius; r++,ptr0++,ptr1++) {
            int dif = (*ptr0 - *ptr1);
            dist += (dif*dif);
        }
		
    }
	
    return dist;
}


void nlmeans_ipol(int iDWin,            // Half size of patch
                  int iDBloc,           // Half size of research window
                  float fSigma,         // Noise parameter
                  float fFiltPar,       // Filtering parameter
                  uint8_t *fpI,          // Input
                  uint8_t *fpO,          // Output
                  int iWidth,int iHeight) {
	
	
	
	
    // length of each channel
    int iwxh = iWidth * iHeight;
	
	
    //  length of comparison window
    int ihwl = (2*iDWin+1);
    int iwl = (2*iDWin+1) * (2*iDWin+1);
	
	// forcing 1 channel in this instance.
  //  int icwl = iChannels * iwl;
	int icwl = iwl;
	
	
    // filtering parameter
    float fSigma2 = fSigma * fSigma;
    float fH = fFiltPar * fSigma;
    float fH2 = fH * fH;
	
    // multiply by size of patch, since distances are not normalized
    fH2 *= (float) icwl;
	
	
	
    // tabulate exp(-x), faster than using directly function expf
    int iLutLength = (int) rintf((float) LUTMAX * (float) LUTPRECISION);
//    float *fpLut = new float[iLutLength];
	float *fpLut = (float *) malloc( sizeof(float)  * iLutLength);

    wxFillExpLut(fpLut,iLutLength);
	
	
	
	
    // auxiliary variable
    // number of denoised values per pixel
	
	// why is this a float?
    //float *fpCount = new float[iwxh];
	int *fpCount = (int *) malloc (sizeof(int) * iwxh);
	//fpClear(fpCount, 0.0f,iwxh);
	memset(fpCount,0,sizeof(int) * iwxh);
    
	
	
	
    // clear output
	//    for (int ii=0; ii < iChannels; ii++) fpClear(fpO[ii], 0.0f, iwxh);
	int *fpOut = (int *) malloc (sizeof(int) * iwxh);

	memset(fpOut,0,sizeof(int) * iwxh);

	
	
    // PROCESS STARTS
    // for each pixel (x,y)
#pragma omp parallel shared(fpI, fpO)
    {
		
		
#pragma omp for schedule(dynamic) nowait
		// auxiliary variable
		// denoised patch centered at a certain pixel
	//	float **fpODenoised = new float*[iChannels];
	//	for (int ii=0; ii < iChannels; ii++) fpODenoised[ii] = new float[iwl];
	
	// single channel	
		float *fpODenoised = (float *) malloc (sizeof(float) * iwl);
		
		int y;
        for ( y=0; y < iHeight ; y++) {
		//	fprintf(stderr,"line: %d\n",y);
			int x;
            for ( x=0 ; x < iWidth;  x++) {
				
                // reduce the size of the comparison window if we are near the boundary
                int iDWin0 = MIN(iDWin,MIN(iWidth-1-x,MIN(iHeight-1-y,MIN(x,y))));
				
				
                // research zone depending on the boundary and the size of the window
                int imin=MAX(x-iDBloc,iDWin0);
                int jmin=MAX(y-iDBloc,iDWin0);
				
                int imax=MIN(x+iDBloc,iWidth-1-iDWin0);
                int jmax=MIN(y+iDBloc,iHeight-1-iDWin0);
				
				
				
                //  clear current denoised patch
                //for (int ii=0; ii < iChannels; ii++) fpClear(fpODenoised[ii], 0.0f, iwl);
				
				
				memset(fpODenoised,0, sizeof(float) * iwl);

				
                // maximum of weights. Used for reference patch
                float fMaxWeight = 0.0f;
				
				
                // sum of weights
                float fTotalWeight = 0.0f;
				
				int j;
                for (j=jmin; j <= jmax; j++) {
					int i;
                    for (i=imin ; i <= imax; i++)
                        if (i!=x || j!=y) {
							
                            int ifDif = fiL2IntDist(fpI,fpI,x,y,i,j,iDWin0,iWidth,iWidth);
							
                            // dif^2 - 2 * fSigma^2 * N      dif is not normalized
                            float fDif = MAX(ifDif - 2 * (float) icwl *  fSigma2, 0.0f);
                            fDif = fDif / fH2;
							
                            float fWeight = wxSLUT(fDif,fpLut);
							
                            if (fWeight > fMaxWeight) fMaxWeight = fWeight;
							
                            fTotalWeight += fWeight;
							
							int is;
                            for ( is=-iDWin0; is <=iDWin0; is++) {
                                int aiindex = (iDWin+is) * ihwl + iDWin;
                                int ail = (j+is)*iWidth+i;
								
								int ir;
                                for ( ir=-iDWin0; ir <= iDWin0; ir++) {
									
                                    int iindex = aiindex + ir;
                                    int il= ail +ir;
									
                                  //  for (int ii=0; ii < iChannels; ii++)
                                        fpODenoised[iindex] += fWeight * fpI[il];
									
                                }
								
                            }
							
                        }
				}
			
                // current patch with fMaxWeight
				int is;
                for ( is=-iDWin0; is <=iDWin0; is++) {
                    int aiindex = (iDWin+is) * ihwl + iDWin;
                    int ail=(y+is)*iWidth+x;
					
					int ir;
                    for ( ir=-iDWin0; ir <= iDWin0; ir++) {
						
                        int iindex = aiindex + ir;
                        int il=ail+ir;
						
                      //  for (int ii=0; ii < iChannels; ii++)
                            fpODenoised[iindex] += fMaxWeight * fpI[il];
						
                    }
                }
				
				
				
                fTotalWeight += fMaxWeight;
				
				
				
				
				
				
				
                // normalize average value when fTotalweight is not near zero
                if (fTotalWeight > fTiny) {
					
					
					int is;
                    for ( is=-iDWin0; is <=iDWin0; is++) {
                        int aiindex = (iDWin+is) * ihwl + iDWin;
                        int ail=(y+is)*iWidth+x;
						int ir;
                        for ( ir=-iDWin0; ir <= iDWin0; ir++) {
                            int iindex = aiindex + ir;
                            int il=ail+ ir;
							
                            fpCount[il]++;
							
                            //for (int ii=0; ii < iChannels; ii++) {
                                fpOut[il] += fpODenoised[iindex] / fTotalWeight;
								
                            }
							
                        }
                    }
					
					
                }
				
				
				
				
				
				
            }
			
			
			
         //   for (int ii=0; ii < iChannels; ii++) delete[] fpODenoised[ii];
          //  delete[] fpODenoised;
		free(fpODenoised);
			
        }
		
		
		
		
    

int ii;
    for ( ii=0; ii < iwxh; ii++)
        if (fpCount[ii]>0) {
				fpO[ii] = fpOut[ii] / fpCount[ii];
        } else {
				fpO[ii] = fpI[ii];
        }
	
	
	
	
    // delete memory
    free(fpLut);
    free(fpCount);
free(fpOut);
	
	
}

static void filterframeNL (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si) {

	int b;
	for (b=0; b < 3; b++)
	{
	
		 uint8_t *srcp = n[b];
		 uint8_t *dstp = m[b];

		
		const int height = y4m_si_get_plane_height(si,b);
		const int width = y4m_si_get_plane_width(si,b);
		const int heightm1 = height -1;
		const int widthm1 = width -1;

		int x1,y1,x2,y2,u,v;
		
		for (y1=0;y1<height;y1++) {

			fprintf (stderr,"LINE %d\n",y1);
			
			for (x1=0;x1<width;x1++) {
				//fprintf (stderr,"COL %d\n",x1);

				memset(this.dsum,0,height * width * sizeof(int));

				int srcoff = y1*width + x1;
				int total = 0;
				
				for(y2=0;y2<height;y2++) {
				
					for(x2=0;x2<width;x2++) {
					
						int dstoff = y2*width + x2;

						
						int yT = -MIN(MIN(this.Ay,y1),y2);
						int yB = MIN(MIN(this.Ay,heightm1-y1),heightm1-y2);
						int xL = -MIN(MIN(this.Ax,x1),x2);
						int xR = MIN(MIN(this.Ax,widthm1-x1),widthm1-x2);
						
						int count = 0;
						int diff = 0;
						
						int winoff = yT * width;
						
						for (v=yT;v<yB;v++)
						{
							
							for (u=xL;u<xR;u++) 
							{
							//	fprintf (stderr,"srcoff %d dstoff %d winoff %d u %d\n",srcoff,dstoff,winoff,u);
								diff += abs(srcp[srcoff + winoff + u] - srcp[dstoff + winoff + u]);
								count ++;
							}
							
							winoff += width;
				
						}
						
					// fprintf(stderr,"diff: %d count %d\n",diff,count);
						if (count == 0) {
							this.dsum[dstoff] = 0;
						} else {
							this.dsum[dstoff] = 255 - (diff / count);
							total += this.dsum[dstoff];
						}
					}
					
				}
				
				
				dstp[srcoff] = 0;
				int winoff = 0;
				int pixel =0 ;
				for(y2=0;y2<height;y2++) {
					
					for(x2=0;x2<width;x2++) {
						pixel += srcp[ winoff + x2 ] * this.dsum[winoff + x2];
					}
											 winoff += width;
				}
				//	fprintf (stderr,"pixel: %d Total: %d\n",pixel,total);

				if (total == 0) {
					dstp[srcoff] = 0;
				} else {
					dstp[srcoff] = pixel / total;
				}
			}
		}
		
	}
}


static void filterframeB  (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si) {

	int b;
	
	int 	Byd = this.By*2+1;
	int 	Bxd = this.Bx*2+1;
	int 	Bxa = Bxd*Byd;
	int		Sxd = this.Sx*2+1;
	double hin = -1.0/this.h;
	
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
		int *sumsb_saved = this.sumsb+this.Bx;
		int *weightsb_saved = this.weightsb+this.Bx;
		int y;
		for (y=this.By; y<height+this.By; y+=Byd)
		{
			const int starty = MAX(y-this.Ay,this.By);
			const int stopy = MIN(y+this.Ay,heightm1-MIN(this.By,heightm1-y));
			const int yTr = MIN(Byd,height-y+this.By);
			
			int x;
			for ( x=this.Bx; x<width+this.Bx; x+=Bxd)
			{
				memset(this.sumsb,0,Bxa*sizeof(int));
				memset(this.weightsb,0,Bxa*sizeof(int));
				int wmax = 0;
				const int startx = MAX(x-this.Ax,this.Bx);
				const int stopx = MIN(x+this.Ax,widthm1-MIN(this.Bx,widthm1-x));
				const int xTr = MIN(Bxd,width-x+this.Bx);
				int u;
				for ( u=starty; u<=stopy; ++u)
				{
					const int yT = -MIN(MIN(this.Sy,u),y);
					const int yB = MIN(MIN(this.Sy,heightm1-u),heightm1-y);
			//	fprintf (stderr,"yb: %d sy: %d hm1: %d u: %d y: %d\n",yB,this.Sy,heightm1,u,y);

					const int yBb = MIN(MIN(this.By,heightm1-u),heightm1-y);
					const unsigned char *s1_saved = pfp + (u+yT)*pitch;
					const unsigned char *s2_saved = pfp + (y+yT)*pitch + x;
					const unsigned char *sbp_saved = pfp + (u-this.By)*pitch;
					const int *gw_saved = this.gw+(yT+this.Sy)*Sxd+this.Sx;
					int v;
					for ( v=startx; v<=stopx; ++v)
					{
						if (u == y && v == x) continue;
						const int xL = -MIN(MIN(this.Sx,v),x);
						const int xR = MIN(MIN(this.Sx,widthm1-v),widthm1-x);
						const unsigned char *s1 = s1_saved + v;
						const unsigned char *s2 = s2_saved;
						const int *gwT = gw_saved;
						int diff = 0, gweights = 0;
						int j;
					//	 fprintf (stderr,"yt: %d yb: %d\n",yT,yB);

						for ( j=yT; j<=yB; j++)
						{
							int k;
					//		fprintf (stderr,"xl: %d xr: %d\n",xL,xR);
							for ( k=xL; k<=xR; ++k)
							{
								diff += ABS(s1[k]-s2[k])*gwT[k];
								gweights += gwT[k];
							}
							s1 += pitch;
							s2 += pitch;
							gwT += Sxd;
						}
						
					//	fprintf (stderr,"diff: %d gweights: %d\n",diff,gweights);
						
						const int weight = exp((diff/gweights)*hin) * PRECISION;
						const int xRb = MIN(MIN(this.Bx,widthm1-v),widthm1-x);
						const unsigned char *sbp = sbp_saved + v;
						int *sumsbT = sumsb_saved;
						int *weightsbT = weightsb_saved;
						// int j;
						for ( j=-this.By; j<=yBb; ++j)
						{
							int k;
							for ( k=-this.Bx; k<=xRb; ++k)
							{
								sumsbT[k] += sbp[k]*weight;
								weightsbT[k] += weight;
							}
							sumsbT += Bxd;
							weightsbT += Bxd;
							sbp += pitch;
						}
						if (weight > wmax) wmax = weight;
					}
				}
				const unsigned char *srcpT = srcp+x-this.Bx;
				unsigned char *dstpT = dstp+x-this.Bx;
				int *sumsbTr = this.sumsb;
				int *weightsbTr = this.weightsb;
				if (wmax <= 1) wmax = PRECISION;
				int j;
				for ( j=0; j<yTr; ++j)
				{
					int k;
					for ( k=0; k<xTr; ++k)
					{
						sumsbTr[k] += srcpT[k]*wmax;
						weightsbTr[k] += wmax;
						dstpT[k] = MAX(MIN((int)((sumsbTr[k]/weightsbTr[k])+0.5),255),0);
					}
					srcpT += pitch;
					dstpT += pitch;
					sumsbTr += Bxd;
					weightsbTr += Bxd;
				}
			}
			dstp += pitch*Byd;
			srcp += pitch*Byd;
		}
	}
}




static void filterframe (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si) {	
	int b;
	
	int Sxd = this.Sx*2+1;
	int Syd = this.Sy*2+1;
	
	double 	h2in = -1.0/(this.h*this.h);
	double hin = -1.0/this.h;
	
	
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

		memset(this.dsum,0,height * width * sizeof(int));
		memset(this.dweight,0,height * width * sizeof(int));
		memset(this.dwmax,0,height * width * sizeof(int));

		int y;
	//	fprintf (stderr,"trace: filterframe y\n");

		for (y=0; y < height; ++y) {
			
			// window edge limiter
			const int stopy = MIN(y+this.Ay,heightm1);
			// vertical offset pointer
			const int doffy = y*width;
			int x;
			
		//	fprintf (stderr,"trace: filterframe b=%d y=%d\n",b,y);

			for (x=0; x < width; ++x) {
				
				// window edge limiter
				const int startxt = MAX(x-this.Ax,0);
				const int stopx = MIN(x+this.Ax,widthm1);
				// horizontal and vertical offset pointer
				const int doff = doffy+x;
				
			//	fprintf (stderr,"trace: filterframe doff=%d\n",doff);

				
				int *dsum = &this.dsum[doff];
				int *dweight = &this.dweight[doff];
				int *dwmax = &this.dwmax[doff];
				
				int u;
// fprintf (stderr,"trace: filterframe u in\n");

				for (u=y; u<=stopy; ++u)
				{
					const int startx = u == y ? x+1 : startxt;
					const int yT = -MIN(MIN(this.Sy,u),y);
					const int yB = MIN(MIN(this.Sy,heightm1-u),heightm1-y);
					const unsigned char *s1_saved = pfp + (u+yT)*pitch;
					const unsigned char *s2_saved = pfp + (y+yT)*pitch + x;
					const int *gw_saved = this.gw+(yT+this.Sy)*Sxd+this.Sx;
					const int pfpl = u*pitch;
					const int coffy = u*width;
					
					int v;
				//	fprintf (stderr,"trace: filterframe v in\n");

					for ( v=startx; v<=stopx; ++v)
					{
						const int coff = coffy+v;
						int *csum = &this.dsum[coff];
						int *cweight = &this.dweight[coff];
						int *cwmax = &this.dwmax[coff];
						const int xL = -MIN(MIN(this.Sx,v),x);
						const int xR = MIN(MIN(this.Sx,widthm1-v),widthm1-x);
						const unsigned char *s1 = s1_saved + v;
						const unsigned char *s2 = s2_saved;
						const int *gwT = gw_saved;
						int diff = 0, gweights = 0;
						
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
						int weight;
						if (diff != 0) {
							//fprintf(stderr,"exp of  %f\n",exp(diff/gweights)*hin);
							
							// what is this doing in the middle of processing
							weight = exp((diff/gweights)*hin) * PRECISION;
						} else {
							weight = PRECISION;
						}
						*cweight += weight;
						*dweight += weight;
						*csum += srcp[x]*weight;
						*dsum += pfp[pfpl+v]*weight;
						if (weight > *cwmax) *cwmax = weight;
						if (weight > *dwmax) *dwmax = weight;
					}
				}
					const int wmax = *dwmax <= 1 ? PRECISION : *dwmax;
					*dsum += srcp[x]*wmax;
					*dweight += wmax;
					dstp[x] = MAX(MIN((int)(((*dsum + PRECISION >> 1)/(*dweight))),255),0);
					
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
	
	
	
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			// static void filterframe (uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si, double *dsum, double *dweight, double *dwmax, int Ax, int Ay, int Sx, int Sy, double *gw)

			int ii;
			for (ii=0; ii< 3; ii++) 
				nlmeans_ipol(this.iDWin,this.iDBloc,this.fSigma,this.fFiltPar,yuv_data[ii],yuv_odata[ii],y4m_si_get_plane_width(inStrInfo,ii),y4m_si_get_plane_height(inStrInfo,ii));
			/*
			if (this.Bx || this.By) {
				filterframeB(yuv_odata,yuv_data,inStrInfo);
			} else {
				filterframeNL(yuv_odata,yuv_data,inStrInfo);
			}
			 */
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
	const static char *legal_flags = "v:hw:b:s:f:";
	
	/*
	static struct option long_options[] =
	{
		/// These options set a flag. 
		{"verbose", no_argument,       &verbose, 2},
		{"quiet",   no_argument,       &verbose, 0},
		// These options don't set a flag.
		// We distinguish them by their indices. 
		{"Ax",     required_argument,       0, 'a'},
		{"Ay",  required_argument,       0, 'b'},
		{"Bx",  required_argument, 0, 'd'},
		{"By",  required_argument, 0, 'c'},
		{"Sx",    required_argument, 0, 'i'},
		{"Sy",    required_argument, 0, 'j'},
		{"a",    required_argument, 0, 'k'},
		{"h",    required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};
	*/
	
	int win,bloc; 
	float sigma, filtpar;
	
	win=1;
	bloc=10;
	sigma=1.0;
	filtpar=0.55;
	
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
			case 'w':
				win = atoi(optarg);
				break;
			case 'b':
				bloc = atoi(optarg);
				break;
				
			case 's':
				sigma = atof(optarg);
				break;
			case 'f':
				filtpar = atof(optarg);
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
	mjpeg_info ("yuvnlmeans (version " VERSION ") is a spatial bilateral filter for yuv streams");
	mjpeg_info ("(C) 2010 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info ("yuvbilateral -h for help");
	
    
	/* in that function we do all the important work */
	//filterinitialize (Ax,Ay,Bx,By,Sx,Sy,a,h,&in_streaminfo);
	this.iDWin = win;
	this.iDBloc = bloc;
	this.fSigma = sigma;
	this.fFiltPar = filtpar;
	
	y4m_write_stream_header(fdOut,&in_streaminfo);
	filter(fdIn,fdOut, &in_streaminfo);
	y4m_fini_stream_info (&in_streaminfo);
	filteruninitialize();
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
