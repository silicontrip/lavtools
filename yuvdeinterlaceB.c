/*
 *  yuvdeinterlace.c
 *  deinterlace  2004 Mark Heath <mjpeg at silicontrip.org>
 *  converts interlaced source material into progressive by doubling the frame rate and 
 *  adaptively interpolating required pixels.
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
 
 gcc yuvdeinterlace.c -I/sw/include/mjpegtools -L/sw/lib -lmjpegutils -o yuvdeinterlace
 gcc yuvdeinterlace.c -I/usr/local/include/mjpegtools -L/usr/local/lib -lmjpegutils  -arch ppc -arch i386 -o yuvdeinterlace

 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// minimum number of pixels needed to detect interlace.
#define PIXELS 4


#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"

#define YUVDE_VERSION "1.6.1"




// OO style globals, with getters and setters
static int setting_mark=0;
static int setting_interpolate=0;
static int setting_fullframe=0;

void setInterpolate (int i) { setting_interpolate = i; }
int getInterpolate () { return setting_interpolate; }

void setMark(int i) { setting_mark = i; }
int getMark() { return setting_mark; }

void setFullframe(int i) { setting_fullframe = i; }
int getFullframe() { return setting_fullframe; }



static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvdeinterlace [-v] [-It|b] [-f] [-m0-4] [-h]\n"
			 "yuvdeinterlace  deinterlaces source material by\n"
			 "doubling the frame rate and interpolating the interlaced scanlines.\n"
			 "\n"
			 "\t -i Interpolator mode. 0: cubic (default). 1: linear. 2: nearest neighbour\n"
			 "\t -m Operation mode: \n"
			 "\t\t 0: deinterlace to full height double framerate (default)\n"
			 "\t\t 1: Mark interlaced pixels (useful for debugging the detection algorithm)\n"
			 "\t\t 2: deinterlace to half height double framerate. (only good for spacial filters)\n"
			 "\t\t 3: deinterlace to full height same frame rate. (Merges both fields into one)\n"
			 "\t\t 4: Interlace to full height half frame rate. (re-interlace)\n" 
			 "\t -I[t|b|p] force interlace mode\n"
			 "\t -f deinterlace entire frame (no adaptive detection)\n"
			 "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
			 "\t -h print this help\n"
			 );
}

// this is the worker detect function, 
// this is where all the tuning needs to go.
int int_detect (int x, int y,uint8_t *m[3],y4m_stream_info_t  *si) {
	
	uint8_t luma[PIXELS];
	int hp = PIXELS/2;
	int i,w,h;
	int hfd=0 ,lfd=0;
	
	int ar,ai,br,bi,cr,ci,dr,di;
	int a,b,c,d;
	
	// mjpeg_warn("int_detect");
	
	
	w = y4m_si_get_plane_width(si,0);
	h = y4m_si_get_plane_height(si,0); 
	
	// read the pixels above and below the target pixel.
	for(i=-2; i<2;i++)
		luma[i] = get_pixel(x,y+i,0,m,si)
			
	// perform a 4 point DFT
	// taking only the components we need
	
	
	//ar = luma[0] + luma[1] + luma[2] + luma[3];
	//	ai = 0;
	br = luma[0] - luma[2];
	bi = luma[3] - luma[1];
	cr = luma[0] - luma[1] + luma[2] - luma[3];
	ci = 0;
	//	dr = br;
	//	di = -bi;
	
	// ar >>= 1;
	// ai >>= 1;
	br >>= 1;
	bi >>= 1;
	cr >>= 1;
	// ci >>= 1;
	//	dr >>= 1;
	//	di >>= 1;
	
	// we are not going to perform square root
	// a = ar * ar;
	b = br * br + bi * bi;
	c = cr * cr;
	//	d = sqrt ( dr * dr + di * di);
	
	//	printf ("%d %d\n",c,b);
	
	/*
	 if (c>127) {
	 printf ("Interlace luma values: %d,%d,%d,%d  DCT coefficients %d-%d\n", luma[0],luma[1],luma[2],luma[3],c,b); 
	 }
	 */
	
	// if the power of the ac frequency is heigher than the half frequency
	
	
	// find simple alternating pattern and areas where the AC is strong.
	if (c > b && c > 12 && ( (luma[0] < luma[1] && luma[2] < luma[1] && luma[2] < luma[3] ) ||
							(luma[0] > luma[1] && luma[2] > luma[1] && luma[2] > luma[3] )) )  {
		
		//	printf ("%d %d\n",c,b);
		return 1;
	}
	
	return 0;
}

static void mark_deint_pixels (int x, int y,uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *si)
{
		
	set_pixel(128,x,y,0,m,si);
	set_pixel(128,xchroma(x,si),ychroma(y,si),1,m,si);
	set_pixel(192,xchroma(x,si),ychroma(y,si),2,m,si);
	
}

uint8_t nearest_interpolate (uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) { return v1; }

uint8_t linear_interpolate (uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
	
	int r = v2 - v1;

  	// Assuming halfway between the pixels.
	return (r >> 1) + v1;
	
}

// Perform a cubic interpolation

uint8_t cubic_interpolate (uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
	
	int p = (v3 - v2) - (v0 - v1);
	int q = (v0 - v1) - p;
	int r = v2 - v0;
	
	// Assuming halfway between the pixels.
	
	int tot = (p >> 3) + (q >> 2) + (r >> 1) + v1;
	
	if (tot > 255) {
		// 	fprintf (stderr,"pixel overflow %d (%d,%d,%d,%d)\n",tot,v0,v1,v2,v3);
		tot = 255;
	}
	if (tot < 0) {
		// fprintf (stderr,"pixel overflow %d (%d,%d,%d,%d)\n",tot,v0,v1,v2,v3);
		tot = 0; 
	}
	
	return tot;
	
}

// wrapper for the interpolation algorithm
uint8_t scalar_interpolate (uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
	
	switch (getInterpolate()) {
		case 0:
			return cubic_interpolate(v0,v1,v2,v3);
			break;
		case 1:
			return linear_interpolate(v0,v1,v2,v3);
			break;
		case 2:
			return nearest_interpolate(v0,v1,v2,v3);
			break;
		default:
			return cubic_interpolate(v0,v1,v2,v3);
			break;
			
	}
}

// interpolates a new pixel based on the 4 surrounding pixels from the same field
static void deint_pixels (int x, int y,uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *si)
{
	
	int w,h,cw,ch,cwr,chr,xcwr,ychr,ychrn,ychrp,ychrcw;
	int ychrn2,ychrp2;
	int tluma, tchromu,tchromv;
	int chroma_posp,chroma_posn;
	int chroma_posp2,chroma_posn2;
	
	h= y4m_si_get_plane_height(si,0);
	
	if (y<=h) { 
		
		w = y4m_si_get_plane_width(si,0);
		ch = y4m_si_get_plane_height(si,1); 
		cw = y4m_si_get_plane_width(si,1);
				
		cwr = w/cw;
		chr = h/ch;
		
		// most common values for cwr and chr are 1,2 or 4
		// optimize for these common values
		
		xcwr = xchroma(x,si);
		
		ychrcw = ychroma(y,si) * cw;
		
		ychrn = ychroma(y-1,si);
		ychrp = ychroma(y+1,si);
		ychrn2 = ychroma(y-3,si);
		ychrp2 = ychroma(y+3,si);
		
		// 4 point interpolate
		
		tluma = scalar_interpolate(get_pixel(x,y-3,0,n,si),
								   get_pixel(x,y-1,0,n,si),
								   get_pixel(x,y+1,0,n,si),
								   get_pixel(x,y+3,0,n,si));
		
		tchromu = scalar_interpolate(get_pixel(xcwr,ychrn2,1,n,si),
								   get_pixel(xcwr,ychrn,1,n,si),
								   get_pixel(xcwr,ychrp,1,n,si),
								   get_pixel(xcwr,ychrp2,1,n,si));
		
		tchromv = scalar_interpolate(get_pixel(xcwr,ychrn2,2,n,si),
									 get_pixel(xcwr,ychrn,2,n,si),
									 get_pixel(xcwr,ychrp,2,n,si),
									 get_pixel(xcwr,ychrp2,2,n,si));
		
		if (chr == 1 && cwr == 1) {
			m[0][x+ychrcw] = tluma;
		} else {
			m[0][x+y*w] = tluma;
		}
		m[1][xcwr+ychrcw] = tchromu;
		m[2][xcwr+ychrcw] = tchromv;
	//	mjpeg_warn("deint pixels done");
	}
	
}

// Merges 4 interlace pixels into 1
static void merge_pixels (int x, int y,uint8_t *m[3],uint8_t *n[3],frame_dimensions *fd)
{
	
	int w,h,cw,ch,cwr,chr,xcwr,ychr,ychrcw;
	int ychr1,ychr2,ychr3,ychr4;
	int tluma, tchromu,tchromv;
	int chroma_pos1,chroma_pos2, chroma_pos3,chroma_pos4;
	int y1,y2,y3,y4;
	
	h = fd->plane_height_luma; 
	w = fd->plane_width_luma;
	ch = fd->plane_height_chroma; 
	cw = fd->plane_width_chroma;
	
	// even y
	y1 = y - 1;
	y2 = y ;
	y3 = y + 1;
	y4 = y + 2;
	
	// 	fprintf (stderr,"pix: %d %d %d %d\n",y1,y2,y3,y4);
	
	// most common values for cwr and chr are 1,2 or 4
	
	cwr = fd->chroma_width_ratio;
	chr = fd->chroma_height_ratio;
	
	// optimize for these common values
	
	xcwr = xchroma(x,si);
	ychrcw  = ychroma(y,si) * cw;
	ychr1 = ychroma(y1,si);
	ychr2 = ychroma(y2,si);
	ychr3 = ychroma(y3,si);
	ychr4 = ychroma(y4,si);
	
	/*
	chroma_pos1 = xcwr + ychr1 * cw;
	chroma_pos2 = xcwr + ychr2 * cw;
	chroma_pos3 = xcwr + ychr3 * cw;
	chroma_pos4 = xcwr + ychr4 * cw;
	*/
	// 4 point interpolate
	
	
	tluma = scalar_interpolate(get_pixel(x,y1,0,n,si),
							   get_pixel(x,y2,0,n,si),
							   get_pixel(x,y3,0,n,si),
							   get_pixel(x,y4,0,n,si));
	tchromu = scalar_interpolate(get_pixel(xcwr,ychr1,1,n,si),
								 get_pixel(xcwr,ychr2,1,n,si),
								 get_pixel(xcwr,ychr3,1,n,si),
								 get_pixel(xcwr,ychr4,1,n,si));
	tchromv = scalar_interpolate(get_pixel(xcwr,ychr1,2,n,si),
								 get_pixel(xcwr,ychr2,2,n,si),
								 get_pixel(xcwr,ychr3,2,n,si),
								 get_pixel(xcwr,ychr4,2,n,si));
	
	if (chr == 1 && cwr == 1) {
		m[0][x+ychrcw] = tluma;
	} else {
		m[0][x+y*w] = tluma;
	}
	m[1][xcwr+ychrcw] = tchromu;
	m[2][xcwr+ychrcw] = tchromv;
	
}

// copies an interlace frame to two half height fields.
static void copy_fields (uint8_t *l[3], uint8_t *m[3], uint8_t *n[3], y4m_stream_info_t *si ) {
	
	copyfield(l,n,si,Y4M_ILACE_BOTTOM_FIRST);
	copyfield(m,n,si,Y4M_ILACE_TOP_FIRST);

}


static void deint_frame (uint8_t *l[3], uint8_t *m[3], uint8_t *n[3], frame_dimensions *fd ) {
	
	// detect and interpolate
	// this detection algorithm takes PIXELS vertical pixels and looks for the classic interlace pattern
	// if interlace is detected the *missing* pixels are interpolated.
	
	int x,y,hp;
	int w,h,cw,ch;
	int mark = 0;
	int full = 0;
	uint8_t luma[PIXELS];
	
//	mjpeg_warn("deint_frame");
	
	
	h = fd->plane_height_luma; 
	w = fd->plane_width_luma;
	
	chromacpy(m,n,fd);
	chromacpy(l,n,fd);
	
	mark = getMark();
	full = getFullframe();
	
	for (x=0; x<w; x++) {
		for (y=0; y<h; y+=2) {
			// is interpolation required
			// there may be a more efficient way to de-interlace a full frame
			if (full != 0 || int_detect(x,y,n,fd) || int_detect(x,y+1,n,fd)) {
				
				switch (mark) {
					case 1:
						mark_deint_pixels(x,y,l,n,fd);
						mark_deint_pixels(x,y+1,l,n,fd);
						break;
					case 3:
						merge_pixels(x,y,l,n,fd);
						merge_pixels(x,y+1,l,n,fd);
						break;
					default:
						deint_pixels(x,y,l,n,fd);
						deint_pixels(x,y+2,l,n,fd);
						deint_pixels(x,y+1,m,n,fd);
						deint_pixels(x,y+3,m,n,fd);
						break;
				}
			}
		}
	}
}

// it appears that the y4m_si_get functions are chewing up lots'o'cpu 

void set_dimensions ( struct frame_dimensions *fd, y4m_stream_info_t  *si) 
{
	
	fd->plane_height_luma = y4m_si_get_plane_height(si,0) ; 
	fd->plane_width_luma = y4m_si_get_plane_width(si,0);
	fd->plane_height_chroma = y4m_si_get_plane_height(si,1) ; 
	fd->plane_width_chroma = y4m_si_get_plane_width(si,1);
	fd->plane_length_luma = y4m_si_get_plane_length(si,0);
	fd->plane_length_chroma = y4m_si_get_plane_length(si,1);
	
	fd->chroma_width_ratio = fd->plane_width_luma / fd->plane_width_chroma;
	fd->chroma_height_ratio = fd->plane_height_luma / fd->plane_height_chroma;
	
}

static void deinterlace(int fdIn,
						y4m_stream_info_t  *inStrInfo,
						int fdOut,
						y4m_stream_info_t  *outStrInfo
						)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	uint8_t            *yuv_tdata[3] ;
	uint8_t            *yuv_bdata[3] ;
	int                read_error_code ;
	int                write_error_code ;
	int interlaced = Y4M_UNKNOWN;            //=Y4M_ILACE_NONE for not-interlaced scaling, =Y4M_ILACE_TOP_FIRST or Y4M_ILACE_BOTTOM_FIRST for interlaced scaling
	
	struct frame_dimensions fdim;
	
	
	
	set_dimensions (&fdim, outStrInfo);
	
	// Allocate memory for the YUV channels
	interlaced = y4m_si_get_interlace(inStrInfo);
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_tdata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_bdata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// de interlace frame.
		if (read_error_code == Y4M_OK) {
			
			// de-interlace one field to odata and the other in place
			
			// different behaviour depending on operating mode
			if (getMark() != 2 )
				deint_frame(yuv_bdata, yuv_tdata, yuv_data, &fdim);
			else 
				copy_fields(yuv_bdata, yuv_tdata, yuv_data, &fdim);
			
			//mjpeg_warn("write");
			
			
			if (getMark() == 3 || getMark() == 1) {
				write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_bdata );
			} else if (interlaced == Y4M_ILACE_TOP_FIRST) {
				write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_tdata );
				write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_bdata );
			} else {
				write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_bdata );
				write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_tdata );
			}
			
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	}
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );
	free( yuv_tdata[0] );
	free( yuv_tdata[1] );
	free( yuv_tdata[2] );
	free( yuv_bdata[0] );
	free( yuv_bdata[1] );
	free( yuv_bdata[2] );
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	if( write_error_code != Y4M_OK )
		mjpeg_error_exit1 ("Error writing output stream!");
	
}

// Re-interlace a 50p stream into a 25i stream.

static void depro(  int fdIn 
				  , y4m_stream_info_t  *inStrInfo
				  , int fdOut
				  , y4m_stream_info_t  *outStrInfo
				  , int proc
				  )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	uint8_t            *yuv_o1data[3] ;
	uint8_t            *yuv_o2data[3] ;
	int                frame_data_size ;
	int                read_error_code ;
	int                write_error_code ;
	int interlaced = -1;            //=Y4M_ILACE_NONE for not-interlaced scaling, =Y4M_ILACE_TOP_FIRST or Y4M_ILACE_BOTTOM_FIRST for interlaced scaling
	int w,h,y;
	
	// Allocate memory for the YUV channels
	interlaced = y4m_si_get_interlace(outStrInfo);
	h = y4m_si_get_height(inStrInfo) ; w = y4m_si_get_width(inStrInfo);
	frame_data_size = h * w;
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	h = y4m_si_get_height(outStrInfo) ; w = y4m_si_get_width(outStrInfo);
	frame_data_size = h * w;
	if (chromalloc(yuv_o1data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_o2data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	
	if (interlaced == Y4M_ILACE_BOTTOM_FIRST ) {
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o2data );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o1data );
	} else if (interlaced == Y4M_ILACE_TOP_FIRST) {
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o1data );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o2data );
	} else {
		mjpeg_warn ("Cannot determine interlace order (assuming top first)\n");
		/* assume top first */
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o1data );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o2data );
	}
	
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		if (read_error_code == Y4M_OK) {
			
			//  interlace frame.
			
			for (y=0; y<h/2; y++) 
			{
				
				/* copy the luminance scan line from the odd frame */
				memcpy(&yuv_data[0][y*2*w],&yuv_o1data[0][y*2*w],w);
				
				/* copy the alternate luminance scan line from the even frame */
				memcpy(&yuv_data[0][(y*2+1)*w],&yuv_o2data[0][(y*2+1)*w],w);
				
				
				/* average the chroma data */
				if (y % 2) {
					// odd bottom field
					//	memcpy (&yuv_data[1][y*w/2],&yuv_o2data[1][y*w/2],w/2);
					//	memcpy (&yuv_data[2][y*w/2],&yuv_o2data[2][y*w/2],w/2);
					int x;
					for (x=0; x<w/2; x++)
					{
						if (proc) {
							yuv_data[1][y*w/2+x] = (yuv_o2data[1][y*w/2+x] + yuv_o1data[1][y*w/2+x]) / 2;
							yuv_data[2][y*w/2+x] = (yuv_o2data[2][y*w/2+x] + yuv_o1data[2][y*w/2+x]) / 2;
						} else {
							yuv_data[1][y*w/2+x] = (yuv_o2data[1][y*w/2+x] + yuv_o2data[1][(y-1)*w/2+x]) /2;
							yuv_data[2][y*w/2+x] = (yuv_o2data[2][y*w/2+x] + yuv_o2data[2][(y-1)*w/2+x]) /2;
						}
					}
					
				} else {
					// even top field
					//memcpy (&yuv_data[1][y*w/2],&yuv_o1data[1][y*w/2],w/2);
					//memcpy (&yuv_data[2][y*w/2],&yuv_o1data[2][y*w/2],w/2);
					int x;
					for (x=0; x<w/2; x++)
					{
						if (proc) {
							yuv_data[1][y*w/2+x] = (yuv_o2data[1][y*w/2+x] + yuv_o1data[1][y*w/2+x]) / 2;
							yuv_data[2][y*w/2+x] = (yuv_o2data[2][y*w/2+x] + yuv_o1data[2][y*w/2+x]) / 2;
						} else {
							yuv_data[1][y*w/2+x] = (yuv_o1data[1][y*w/2+x] + yuv_o1data[1][(y+1)*w/2+x]) /2;
							yuv_data[2][y*w/2+x] = (yuv_o1data[2][y*w/2+x] + yuv_o1data[2][(y+1)*w/2+x]) /2;
						}
					}
				}
			}
			
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data );
			
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		if (interlaced == Y4M_ILACE_BOTTOM_FIRST ) {
			read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o2data );
			read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o1data );
		} else {
			read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o1data );
			read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_o2data );
		}
	}
	
	
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );
	free( yuv_o1data[0] );
	free( yuv_o1data[1] );
	free( yuv_o1data[2] );
	free( yuv_o2data[0] );
	free( yuv_o2data[1] );
	free( yuv_o2data[2] );
	
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
	
	int verbose = 4 ; //LOG_ERROR
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	y4m_ratio_t frame_rate;
	int interlaced,pro_chroma=0,yuv_interlacing= Y4M_UNKNOWN;
	int height;
	int fullframe=0;
	int c ;
	const static char *legal_flags = "I:v:i:pchfm:";
	
	setFullframe(0);
	setMark(0);
	setInterpolate(0);
	
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
				case 'f':
				setFullframe(1);
				break;
				case 'I':
				switch (optarg[0]) {
					case 't':  yuv_interlacing = Y4M_ILACE_TOP_FIRST;  break;
					case 'b':  yuv_interlacing = Y4M_ILACE_BOTTOM_FIRST;  break;
					default:
						mjpeg_error("Unknown value for interlace: '%c'", optarg[0]);
						return -1;
						break;
				}
				break;
				case 'i':
				setInterpolate(atoi(optarg));
				break;
				case 'm':
				setMark(atoi(optarg));
				break;
				case 'c':
				pro_chroma = 1;
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
	
	// Check input parameters
	height = y4m_si_get_height (&in_streaminfo);
	frame_rate = y4m_si_get_framerate( &in_streaminfo );

	if (yuv_interlacing==Y4M_UNKNOWN) {
		interlaced = y4m_si_get_interlace (&in_streaminfo);
	} else {
		interlaced = yuv_interlacing;
	}
	
	if ((height%2) && (getMark() != 4))
		mjpeg_error_exit1("material is not even frame height");
	
	if ((interlaced == Y4M_ILACE_NONE) && (getMark() != 4)) {
		mjpeg_error_exit1("source material not interlaced");
	}


	// set up values for the output stream
	switch (getMark()) {
		case 4:  // progressive to interlace
			frame_rate.d *= 2;
			if (interlaced != Y4M_ILACE_NONE)
				mjpeg_warn("source material is interlaced");
			if (yuv_interlacing != Y4M_ILACE_NONE) {
				mjpeg_warn("No interlace mode selected");
				yuv_interlacing = Y4M_ILACE_BOTTOM_FIRST;
			}
			interlaced = yuv_interlacing;
			break;
		case 2: // half height 
			height = height >> 1;
		case 0: // double frame rate
			frame_rate.n *= 2 ;
		case 3: // same height, same frame rate (merge)
			if (yuv_interlacing != Y4M_UNKNOWN)
				y4m_si_set_interlace(&in_streaminfo, yuv_interlacing);
			
			interlaced = Y4M_ILACE_NONE;
			break;
	}
	

	
	// Prepare output stream	
	
	y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
	
	
	// Information output
	mjpeg_info ("yuvdeinterlace (version " YUVDE_VERSION ") is a general deinterlace/interlace utility for yuv streams");
	mjpeg_info ("(C) 2005 Mark Heath <http://silicontrip.net/lavtools/>");
	mjpeg_info ("yuvdeinterlace -h for help");
	
	y4m_si_set_framerate( &out_streaminfo, frame_rate );                
	y4m_si_set_height (&out_streaminfo, height);
	y4m_si_set_interlace(&out_streaminfo, interlaced);
	y4m_write_stream_header(fdOut,&out_streaminfo);
    
	/* in that function we do all the important work */
	if (getMark() != 4) 
		deinterlace( fdIn, &in_streaminfo, fdOut, &out_streaminfo);
	
	/* this has been replaced by yuvafps */
	if (getMark() == 4) 
		depro( fdIn, &in_streaminfo,  fdOut,&out_streaminfo,pro_chroma);
	
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
