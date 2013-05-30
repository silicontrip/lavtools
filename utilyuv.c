#include "utilyuv.h"
#include <stdio.h>
/*
** <p>this is a utility library. It doesn't do anything itself</p>

going to start putting my most common functions here. 
 
 gcc -I/usr/local/include/mjpegtools -c utilyuv.c
 
 */

struct y4m_stream_info_cache {
	int h;
	int w;
	int cw;
	int ch;
};

struct y4m_stream_info_cache sic;

struct colour {
	uint8_t y;
	uint8_t u;
	uint8_t v;
};


// Allocate a uint8_t frame
int chromalloc(uint8_t *m[3], y4m_stream_info_t *sinfo)
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	// I'm gonna cheat and use this as an initialisation function.
	
	sic.h = y4m_si_get_plane_height(sinfo,0);
	sic.w = y4m_si_get_plane_width(sinfo,0);
	sic.ch = y4m_si_get_plane_height(sinfo,1);
	sic.cw = y4m_si_get_plane_width(sinfo,1);
	
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
void chromacpy(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo)
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	memcpy (m[0],n[0],fs);
	memcpy (m[1],n[1],cfs);
	memcpy (m[2],n[2],cfs);
	
}


//allocate an array of frames, for temporal filters
int temporalalloc (uint8_t ****yuv_data, y4m_stream_info_t *sinfo, int length)
{
	int c,d;
	
//	fprintf(stderr,"temporalalloc: in\n");
	
	*yuv_data= (uint8_t ***) malloc(sizeof (uint8_t *) * length);
	if (*yuv_data == NULL) return -1;
	for (c=0;c<length;c++) {
		(*yuv_data)[c] = (uint8_t **) malloc(sizeof (uint8_t *) * 3);
		if ((*yuv_data)[c] == NULL) {
			// how am I going to keep track of what I've allocated?
			for (d=0;d<c;d++) {
				chromafree((*yuv_data)[d]);
				free((*yuv_data)[d]);
			}
			free (*yuv_data);
			return -1;
		}
		if(chromalloc((*yuv_data)[c],sinfo)) {
			for (d=0;d<c;d++) {
				chromafree((*yuv_data)[d]);
				free((*yuv_data)[d]);
			}
			free((*yuv_data)[c]);
			free(*yuv_data);
			return -1;
		}
	}
	// fprintf(stderr,"temporalalloc: out\n");
	return 0;
}

void temporalfree(uint8_t ***yuv_data, int length) 
{
	int d;
	for (d=0;d<length;d++) {
		chromafree(yuv_data[d]);
		free(yuv_data[d]);
	}
	free (yuv_data);
}

//do I want to also shuffle the other direction?
void temporalshuffle(uint8_t ***yuv_data, int length)
{
	
	uint8_t				**temp_yuv;
	int c;
//	fprintf(stderr,"temporalshuffle: in\n");

	temp_yuv = yuv_data[0];
	for (c=0;c<length-1;c++) 		
		yuv_data[c] = yuv_data[c+1];
	yuv_data[length-1] = temp_yuv;
	
//	fprintf(stderr,"temporalshuffle: out\n");

}	

//Copy a  single field of a frame

void copyfield(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo, int which)
{
	int r = 0;
	int h,w,cw,ch;
	
	h = y4m_si_get_plane_height(sinfo,0);
	w = y4m_si_get_plane_width(sinfo,0);
	cw = y4m_si_get_plane_width(sinfo,1);
	ch = y4m_si_get_plane_height(sinfo,1);
	
	
	if (which==Y4M_ILACE_TOP_FIRST) {
		r=0;
	} else if (which==Y4M_ILACE_BOTTOM_FIRST) {
		r=1;
	} else {
		mjpeg_warn("copyfield() invalid interlace selected (%d)",which);
	}
	
	for (; r < h; r += 2)
	{
		memcpy(&m[0][r * w], &n[0][r * w], w);
		if (r<ch) {
			memcpy(&m[1][r*cw], &n[1][r*cw], cw);
			memcpy(&m[2][r*cw], &n[2][r*cw], cw);
		}
	}
}


// set a solid colour for a uint8_t frame
void chromaset(uint8_t *m[3], y4m_stream_info_t  *sinfo, int y, int u, int v )
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	memset (m[0],y,fs);
	memset (m[1],u,cfs);
	memset (m[2],v,cfs);
	
}

void chromafree(uint8_t *m[3]) 
{
	
	free(m[0]);
	free(m[1]);
	free(m[2]);
	
}

// Get a pixel, with bounds checking.
//how easy is it to make this for all planes
uint8_t get_pixel(register int x, register int y, int plane, uint8_t *m[3],y4m_stream_info_t *si)
{
	
	int w,h,off;
	uint8_t *p;
	
	/*
	h = y4m_si_get_plane_height(si,plane);
	w = y4m_si_get_plane_width(si,plane);
	*/
	
	//  the y4m_si_get_plane functions are eating large amounts of CPU on PPC machines.
	
	if ( plane == 0) {
		h = sic.h;
		w = sic.w;
	} else {
		h = sic.ch;
		w = sic.cw;
	}
	
	
	// my poor attempt to optimise for speed.
	p=m[plane]+x;
	off= y * w;
	
	if (x < 0) {x=0; p=m[plane];}
	if (x >= w) {x=w-1; p=m[plane]+x;}
	if (y < 0) {y=0; off=0;}
	if (y >= h) {y=h-1; off= y * w;}
	
	return 	*(p+off);
	
}

void set_pixel(uint8_t val,int x, int y, int plane, uint8_t *m[3],y4m_stream_info_t *si)
{

		
		int w,h;
		
		h = y4m_si_get_plane_height(si,plane);
		w = y4m_si_get_plane_width(si,plane);
		
		if (x >= 0 && x < w && y >= 0 && y < h) {
			*(m[plane]+x+y*w) = val;
		}
}
	

// Mix two colours, percent is 0-255
uint8_t mix (uint8_t c1, uint8_t c2, uint8_t per, int y) {
	return ((c1 - y) * (255 - per) + (c2 - y) * per) / 255 + y; 

}

uint8_t luma_mix (uint8_t c1, uint8_t c2, uint8_t per) {
	return mix(c1,c2,per,16); 
}

uint8_t chroma_mix (uint8_t c1, uint8_t c2, uint8_t per) {
	return mix(c1,c2,per,128); 
}




int parse_interlacing(char *str)
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

//
// Returns the greatest common divisor (GCD) of the input parameters.
//
int gcd(int a, int b)
{
	if (b == 0)
		return a;
	else
		return gcd(b, a % b);
}

int xchroma (int x, y4m_stream_info_t *si)
{
	int cwr;
	cwr = y4m_si_get_plane_width(si,0) / y4m_si_get_plane_width(si,1);
	
	if ( cwr == 1 ) {
		return  x;
	} else if ( cwr == 2) {
		return x >> 1;
	} else if (cwr == 4) {
		return x >> 2;
	}
    
    return x / cwr;
	
}

int ychroma(int y, y4m_stream_info_t *si)
{
	
	int chr,ychr;
	chr=y4m_si_get_plane_height(si,0) / y4m_si_get_plane_height(si,1);

	
	if (chr == 1) {	
		return y;
	} else if (chr == 2) {
		if (y4m_si_get_interlace(si) == Y4M_ILACE_NONE) {
			return y >> 1;
		} else {
			return ((y >> 2) << 1) + (y%2);
		}
	} else if (chr == 4) {
		// I have no idea how a /4 interlace chroma would work.
		return y >> 2;
	}
    return y / chr;

}

// I don't think this code is valid.
int timecode2framecount (y4m_stream_info_t *si, int h, int m, int s, int f, int df) 
{

	int sec;
	y4m_ratio_t fr;
	
	fr = y4m_si_get_framerate(si);
	sec = h * 3600 + m * 60 + s;
	if (fr.n % fr.d) {
		// non integer frame rate
		if (df) {
			if ( fabs(29.97 - (1.0 * fr.n / fr.d )) < 0.001){
				// TODO: this only works for 29.97
				// there is only drop code algorithm for 30000/1001
				
				int totalMinutes = 60 * h + m;
				fr.n += fr.d - (fr.n % fr.d);
				// 2 skipped frames per minute, excluding the 10 minute divisible ones.
				return sec * fr.n / fr.d - 2 * (totalMinutes - totalMinutes / 10);
			} else {
				mjpeg_warn ("Unknown drop frame frame rate: %d/%d",fr.n,fr.d);
			}
		} else {
			// non drop frame
			// round up to the next highest frame rate, time code doesn't represent real time.
			fr.n += fr.d - (fr.n % fr.d);
			return sec * fr.n / fr.d;
		}
	} else {
		//  integer frame rates
		return sec * fr.n / fr.d;
	}
	
}

void framecount2timecode(y4m_stream_info_t  *si, int *h, int *m, int *s, int *f, int fc, int *df ) 
{
	
	y4m_ratio_t fr;
	//	fprintf (stderr,"string_tc\n");
	
	fr = y4m_si_get_framerate (si);
		
	if (fr.n % fr.d) {
		
		int n = fr.n;
		
		// round up to integer frame rate
		// non drop calculation.
		fr.n += fr.d - (fr.n % fr.d);
		
	mjpeg_debug ("rounded up to : %d:%d\n",fr.n,fr.d);
		
		if (*df) {
		// drop calculation.

			if ( fabs(29.97 - (1.0 * n / fr.d )) < 0.001){
				// the correct SMPTE algorithm for 29.97
				int smpted = fc / 17982;
				int smptem = fc % 17982;
				fc +=  18*smpted + 2*((smptem - 2) / 1798);
			} else {
				// this algorithm is not the correct SMPTE algorithm. 
				// but is more generic
				fc = (fc * fr.n) / n;
			}

			
		}
	} else {
		*df = 0;
	}
	
	
	*h = fr.d * fc / fr.n / 3600;
	*m = (fr.d * fc / fr.n / 60) % 60;
	*s = (fr.d * fc / fr.n) % 60;
	*f = fc % (fr.n / fr.d);
	
}

