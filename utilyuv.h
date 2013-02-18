#ifndef _UTILYUV_H_
#define _UTILYUV_H_

#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


// allocates the plane buffers based on the stream info
int chromalloc(uint8_t *m[3], y4m_stream_info_t *sinfo);
void chromafree(uint8_t *m[3]);


// functions for temporal based filters
int temporalalloc (uint8_t ****yuv_data, y4m_stream_info_t *sinfo, int length);
void temporalfree(uint8_t ***yuv_data, int length);
void temporalshuffle(uint8_t ***yuv_data, int length);


// copies plane buffers from n to m (m=n)
void chromacpy(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo);
void copyfield(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo, int which);


// set a solid colour for a uint8_t frame
void chromaset(uint8_t *m[3], y4m_stream_info_t  *sinfo, int y, int u, int v );

// pixel functions
uint8_t get_pixel(int x, int y, int plane, uint8_t *m[3],y4m_stream_info_t *si);
void set_pixel(uint8_t val,int x, int y, int plane, uint8_t *m[3],y4m_stream_info_t *si);

// colour mixing functions
uint8_t mix (uint8_t c1, uint8_t c2, uint8_t per, int y);
uint8_t luma_mix (uint8_t c1, uint8_t c2, uint8_t per);
uint8_t chroma_mix (uint8_t c1, uint8_t c2, uint8_t per);


// returns the opposite field ordering
int invert_order(int f);
int parse_interlacing(char *str);
int gcd(int a, int b);

// convert luma co-ordinates into chroma co-ordinates
int xchroma (int x, y4m_stream_info_t *si);
int ychroma (int y, y4m_stream_info_t *si);

// time code functions
int timecode2framecount (y4m_stream_info_t *si, int h, int m, int s, int f, int df) ;
void framecount2timecode(y4m_stream_info_t  *si, int *h, int *m, int *s, int *f,  int fc, int *df );


#endif
