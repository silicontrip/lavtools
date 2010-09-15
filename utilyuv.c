#include "utilyuv.h"

/* going to start putting my most common functions here. 
 
 gcc -I/usr/local/include/mjpegtools -c utilyuv.c
 
 */

// Allocate a uint8_t frame
int chromalloc(uint8_t *m[3], y4m_stream_info_t *sinfo)
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

