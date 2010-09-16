#ifndef _UTILYUV_H_
#define _UTILYUV_H_

#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include <stdint.h>
#include <string.h>



// allocates the plane buffers based on the stream info
int chromalloc(uint8_t *m[3], y4m_stream_info_t *sinfo);

// copies plane buffers from n to m (m=n)
void chromacpy(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo);
void copyfield(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo, int which);


// set a solid colour for a uint8_t frame
void chromaset(uint8_t *m[3], y4m_stream_info_t  *sinfo, int y, int u, int v );
uint8_t get_pixel(int x, int y, int plane, uint8_t *m[3],y4m_stream_info_t *si);

void chromafree(uint8_t *m[3]);


// returns the opposite field ordering
int invert_order(int f);
int parse_interlacing(char *str);
int gcd(int a, int b);

// convert luma co-ordinates into chroma co-ordinates
int xchroma (int x, y4m_stream_info_t *si);
int ychroma (int y, y4m_stream_info_t *si);


#endif
