#include "utilyuv.h"

/* going to start putting my most common functions here. 
 
 gcc -I/usr/local/include/mjpegtools -c utilyuv.c
 
 */

struct y4m_stream_info_cache {
	int h;
	int w;
	int cw;
	int ch;
};

 struct y4m_stream_info_cache sic;

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
		mjpeg_warn("invalid interlace selected");
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
	
	//  the y4m_si_get_plane functions are eating large amounts of CPU.
	
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
	int cwr,xcwr;
	cwr = y4m_si_get_plane_width(si,0) / y4m_si_get_plane_width(si,1);
	
	if ( cwr == 1 ) {
		xcwr = x;
	} else if ( cwr == 2) {
		xcwr = x >> 1;
	} else if (cwr == 4) {
		xcwr = x >> 2;
	} else {
		xcwr = x / cwr;
	}
	
	return xcwr;
	
}

int ychroma(int y, y4m_stream_info_t *si)
{
	
	int chr,ychr;
	chr=y4m_si_get_plane_height(si,0) / y4m_si_get_plane_height(si,1);

	
	if (chr == 1) {	
		ychr = y;
	} else if (chr == 2) {
		if (y4m_si_get_interlace(si) == Y4M_ILACE_NONE) {
			ychr = y >> 1;
		} else {
			ychr = ((y >> 2) << 1) + (y%2);
		}
	} else if (chr == 4) {
		// I have no idea how a /4 interlace chroma would work.
		ychr = y >> 2;
	} else {	
		ychr = y / chr;		
	}
	
	return ychr;
}
