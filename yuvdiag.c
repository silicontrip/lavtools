/*
 *  yuvdiag.c
 *  diagnostics  2008 Mark Heath <mjpeg at silicontrip.org>
 *  Technical display routines 
 *  Converts a YUV stream for technical purposes.
 *
 *  based on code:
 *  Copyright (C) 2002 Alfonso Garcia-Pati�o Barbolani <barbolani at jazzfree.com>
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
gcc -O3 yuvdiag.c -L/sw/lib -I/sw/include/mjpegtools -lmjpegutils -o yuvdiag
gcc -O3 -I/opt/local/include -I/usr/local/include/mjpegtools -L/opt/local/lib -lmjpegutils yuvdiag.c -o yuvdiag
gcc -O3 -I/opt/local/include -I/opt/local/include/freetype2 -I/usr/local/include/mjpegtools -L/opt/local/lib -lmjpegutils -lfreetype yuvdiag.c -o yuvdiag
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

#include "utilyuv.h"

#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


#define CHARWIDTH 15
#define CHARHEIGHT 22
#define LINEWIDTH 1376


#define YUVDI_VERSION "0.5"

static void print_usage() 
{
	fprintf (stderr,
		"usage: yuvdiag [-v] [-ycl] [-h]\n"
		"yuvdiag converts the yuvstream for technical viewing\n"
		"\n"
		"\t -y copy yuv channels into the luma channel\n"
		"\t -c chroma scope\n" 
		"\t -l luma scope\n" 
		"\t -t time code\n"
		"\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
		"\t -h print this help\n"
	);
}


// Would love to make this a generic vector image to overlay

void draw_luma (uint8_t *m[], y4m_stream_info_t  *sinfo) 
{

	int x,y,y1,x1,height,width,cwidth;
	int cx, cy;

	// fprintf(stderr,"draw_luma: enter\n");


	height = y4m_si_get_plane_height(sinfo,0);	
	width = y4m_si_get_plane_width(sinfo,0);	
	cwidth = y4m_si_get_plane_width(sinfo,1);	

	
	for (y1=0 ; y1 < 256; y1+=8) {
	
		// fprintf(stderr,"draw_luma: y1=%d\n",y1);

	
		y = (height -1) - y1;
	
		for (x1=0; x1 < 8; x1++) {
			
			x = x1;
			//chroma_coord(sinfo, &cx, &cy, x, y);
			
			cx = xchroma(x,sinfo);
			cy = ychroma(y,sinfo);
			
			m[0][y * width + x] = 240;
			m[1][cy * cwidth + cx] = 240;
			m[2][cy * cwidth + cx] = 128;

			x = (width - 1) - x1;
			//chroma_coord(sinfo, &cx, &cy, x, y);
			
			cx = xchroma(x,sinfo);
			cy = ychroma(y,sinfo);

			
			m[0][y * width + x] = 240;
			m[1][cy * cwidth + cx] = 240;
			m[2][cy * cwidth + cx] = 128;

		
		}
	
	}
		// fprintf(stderr,"draw_luma: exit\n");


}

void black_box(uint8_t **yuv,y4m_stream_info_t  *sinfo,int x,int y,int w,int h)
{

	int dw,dh;
	int dx,dy;
	
//fprintf (stderr,"black_box\n");
	
	dw = y4m_si_get_plane_width(sinfo,0);
	
	for (dy=y; dy<y+h;dy++) {
		for (dx=x; dx<x+w;dx++) {
		//	fprintf (stderr,"dx: %d dy: %d\n",dx,dy);
			yuv[0][dx+dy*dw] = 16;
		}
	}
}





void read_font(uint8_t **d)
{
	
	FILE *fd;
	int y,x;
	int w,h;
	char type[8],width[8],height[8],depth[8];
	
//fprintf (stderr,"read_font\n");
	
	
	*d = (uint8_t *) malloc (LINEWIDTH*CHARHEIGHT);
	
	fd = fopen ("yuvdiag_font.pbm","r");
	
	if (fd != NULL) {
		
		// fprintf(stderr,"read_font: fgets\n");
		
		fgets(type,7,fd);
		fgets(width,7,fd);
		fgets(height,7,fd);
		fgets(depth,7,fd);
		
		
		w=atoi(width);
		h=atoi(height);
		
		fprintf (stderr,"type: %s width %d height %d depth %s",type,w,h,depth);
		
		
		for (y=0;y<CHARHEIGHT;y++) {
			
			// fprintf(stderr,"y: %d\n",y);
			
			fread(*d+y*LINEWIDTH,LINEWIDTH,1,fd);
			fseek(fd,w-LINEWIDTH, SEEK_CUR);
		}
		fclose(fd);
	} else {
		mjpeg_error_exit1("Cannot open yuvdiag font (./yuvdiag_font.pbm)");
	}
	
}	

void string_tc( char *tc, int fc, y4m_stream_info_t  *sinfo ) {

	int h,m,s,f;
	y4m_ratio_t fr;
	char df = ':';
	int d,n;
	
//	fprintf (stderr,"string_tc\n");

	fr = y4m_si_get_framerate (sinfo);
	

	// TODO: need to handle NTSC drop frame
	if (fr.n % fr.d) {
		
		n = fr.n;
		
		// round up to integer frame rate
		// non drop calculation.
		fr.n += fr.d - (fr.n % fr.d);
		
		// drop calculation.
		// stick an IF around this make it command line configurable.
		fc = (fc * fr.n) / n;
		df =';';

	}
	
	// fprintf (stderr,"%d/%d int fr %d\n",fr.n,fr.d, fr.n % fr.d);

	
	h = fr.d * fc / fr.n / 3600;
	m = (fr.d * fc / fr.n / 60) % 60;
	s = (fr.d * fc / fr.n) % 60;
	f = fc % (fr.n / fr.d);
	
	
	sprintf(tc,"TCR*%02d:%02d:%02d%c%02d",h,m,s,df,f);
//	fprintf (stderr,"%d - %s\n",fc,tc);

}

void render_string_ft (uint8_t **yuv, FT_Face face, y4m_stream_info_t  *sinfo ,int x,int y,char *time) 
{
	char c,r;
    FT_UInt glyph_index,error;
	FT_Glyph  glyph;
	int dw,dx,dy,cy;
	int cpos,rpos;
	int ilace;
	
	
	dw = y4m_si_get_plane_width(sinfo,0);
	
	for (c=0;c<strlen(time);c++) {
		
		r=time[c];
		
		glyph_index = FT_Get_Char_Index( face, time[c] );
		error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
		if ( error )
			continue;  /* ignore errors */
		
		error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
		if ( error )
			continue;
		
		error = FT_Get_Glyph( face->glyph, &glyph );
		
		cpos = c * face->glyph->metrics.horiAdvance/64; 
		
		if (r=='T') {
			// font metric tweaking.
				black_box(yuv,sinfo,x,y,15 * face->glyph->metrics.horiAdvance/64,face->glyph->metrics.height/64+2);
				cy =  y + face->glyph->metrics.height/64 + 1;
		}
		//fprintf (stderr,"ft width %d\n",face->glyph->bitmap.width);
		
		if (r == '*' && y4m_si_get_interlace(sinfo)) {
			ilace = 2;
		} else {
			ilace = 1;
		}
		
		for (dy=0; dy<face->glyph->bitmap.rows;dy+=ilace) {
			for (dx=0; dx<face->glyph->bitmap.width;dx++) {
				// fprintf (stderr,"render_string dx %d dy: %d\n",dx,dy);
				yuv[0][(x+dx+cpos+face->glyph->metrics.horiBearingX/64)+(cy+dy-face->glyph->metrics.horiBearingY/64)*dw] = *(face->glyph->bitmap.buffer+dx+dy*face->glyph->bitmap.width);
			//	yuv[0][(x+dx+cpos)+(cy+dy-face->glyph->metrics.horiBearingY/64)*dw] = *(face->glyph->bitmap.buffer+dx+dy*face->glyph->bitmap.width);

			}
		}
		
		
		
	}	
	
	
}

void render_string (uint8_t **yuv, uint8_t *fd,y4m_stream_info_t  *sinfo ,int x,int y,char *time) 
{

	int dw,dx,dy;
	char c,r;
	int cpos,rpos;

	dw = y4m_si_get_plane_width(sinfo,0);

//fprintf (stderr,"render_string\n");


//	fprintf (stderr,"render_string strlen: %d\n",strlen(time));
	
	for (c=0;c<strlen(time);c++) {

		r=time[c];

//fprintf (stderr,"render_string char: %c\n",r);

		cpos = c * CHARWIDTH; rpos = (r-32) * CHARWIDTH;
		for (dy=0; dy<CHARHEIGHT;dy++) {
			for (dx=0; dx<CHARWIDTH;dx++) {
				//	fprintf (stderr,"render_string dx %d dy: %d\n",dx,dy);
				yuv[0][(x+dx+cpos)+(y+dy)*dw] = fd[dx+rpos+dy*LINEWIDTH];
				
			}
		}
	}

}

static void timecode(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, char *fontname )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*font_data;
	int                read_error_code ;
	int                write_error_code = Y4M_OK;
	int frameCounter = 0;
	char time[32];
	
	int w,h,error;

	FT_Library  library;
	FT_Face     face;
	
//	fprintf (stderr,"timecode\n");

	
	error = FT_Init_FreeType( &library );
	if ( error )
	{
		mjpeg_error_exit1 ("Couldn't initialise the FT library!");
	}

	error = FT_New_Face( library, fontname, 0, &face );
	if ( error == FT_Err_Unknown_File_Format )
	{
		mjpeg_error_exit1 ("Unknown Font type!");
	}
	else if ( error )
	{
		mjpeg_error_exit1 ("Error reading font file!");
	}
	
	// error = FT_Set_Char_Size( face, 0, 8*64, 300, 300 );
	
	error = FT_Set_Pixel_Sizes( face, 32, 28 );   
	
//	read_font(&font_data);
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	w = y4m_si_get_plane_width(inStrInfo,0);
	h = y4m_si_get_plane_height(inStrInfo,0);


	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	

	w = (w / 2) - (15 * CHARWIDTH / 2);
	h = h - 24-CHARHEIGHT;
	
	//	fprintf (stderr,"box pos: %d %d\n",w,h);

	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
		
			// convert counter into TC string
			string_tc(time,frameCounter,inStrInfo);

			// draw black box
			
			// black_box(yuv_data,inStrInfo,w,h,15 * CHARWIDTH,CHARHEIGHT);
			
			// render string
		
			render_string_ft (yuv_data,face,inStrInfo,w,h,time);
		
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_data );
			frameCounter++;
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
	
	free (time);
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");

}

static void channel(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,height,width;
	int oheight,owidth;
	int y;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	cheight = y4m_si_get_plane_height(inStrInfo,1);
	cwidth = y4m_si_get_plane_width(inStrInfo,1);

	width = y4m_si_get_plane_width(inStrInfo,0);
	height = y4m_si_get_plane_height(inStrInfo,0);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
				chromaset (yuv_odata,outStrInfo,16,128,128);
			
			// this makes assumptions that we are in 420
			
			for (y=0; y< cheight; y++) {
				// luma copy
				memcpy(yuv_odata[0] + y * owidth,  yuv_data[0] + y * width, width);
				memcpy(yuv_odata[0] + (y + cheight) * owidth,  yuv_data[0] + (y + cheight) * width, width);

				
				memcpy(yuv_odata[0] + y * owidth + width, yuv_data[1] + y * cwidth, cwidth);
				memcpy(yuv_odata[0] + (y + cheight) * owidth + width, yuv_data[2] + y * cwidth, cwidth);
				
			}
			
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

static void chroma_scope(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,ocheight,ocwidth;
	int oheight,owidth;
	int y,x;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	cheight = y4m_si_get_plane_height(inStrInfo,1);
	cwidth = y4m_si_get_plane_width(inStrInfo,1);

	ocwidth = y4m_si_get_plane_width(outStrInfo,1);
	ocheight = y4m_si_get_plane_height(outStrInfo,1);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			chromaset (yuv_odata,outStrInfo,16,128,128);
			
			for (x=0; x< cwidth; x++) 
				for (y=0; y< cheight; y++) {
					// fprintf (stderr,"U: %d V: %d\n",yuv_data[1][y*cwidth+x],yuv_data[2][y*cwidth+x]);
					/* if ( y< ocheight && x < ocwidth) {
						yuv_odata[1][y*ocwidth+x] = x*2;
						yuv_odata[2][y*ocwidth+x] = y*2;
					}
					*/
					if (yuv_odata[0][yuv_data[1][y*cwidth+x] + yuv_data[2][y*cwidth+x] * owidth] < 255) 
					yuv_odata[0][yuv_data[1][y*cwidth+x] + yuv_data[2][y*cwidth+x] * owidth] ++;
				}
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

static void luma_scope(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,height,width;
	int oheight,owidth;
	int y,x;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	cheight = y4m_si_get_plane_height(inStrInfo,1);
	cwidth = y4m_si_get_plane_width(inStrInfo,1);

	width = y4m_si_get_plane_width(inStrInfo,0);
	height = y4m_si_get_plane_height(inStrInfo,0);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			chromaset (yuv_odata,outStrInfo,16,128,128);
			chromacpy (yuv_odata,yuv_data,inStrInfo);
			
			for (x=0; x< width; x++) 
				for (y=0; y< height; y++) {
					// fprintf (stderr,"U: %d V: %d\n",yuv_data[1][y*cwidth+x],yuv_data[2][y*cwidth+x]);
					if ( yuv_odata[0][((oheight-1) -  yuv_data[0][y*width+x] ) * owidth + x] < 255 )
						yuv_odata[0][((oheight-1) -  yuv_data[0][y*width+x] ) * owidth + x] +=2;
				}
				draw_luma(yuv_odata,outStrInfo);
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

void acc_hist(  int fdIn  , y4m_stream_info_t  *inStrInfo, int fdOut, y4m_stream_info_t  *outStrInfo )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3];
	uint8_t				*yuv_odata[3];
	int                read_error_code ;
	int                write_error_code ;
	int cheight,cwidth,height,width;
	int oheight,owidth;
	int y,x;
	int hist[256],max,choice;

	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,outStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	/* Initialize counters */
	
	width = y4m_si_get_plane_width(inStrInfo,0);
	height = y4m_si_get_plane_height(inStrInfo,0);

	owidth = y4m_si_get_plane_width(outStrInfo,0);
	oheight = y4m_si_get_plane_height(outStrInfo,0);

	write_error_code = Y4M_OK ;
	
	for (x=0; x<256; x++) 
		hist[x] = 0;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			chromaset (yuv_odata,outStrInfo,16,128,128);
			// chromacpy (yuv_odata,yuv_data,inStrInfo);
			
			for (x=0; x< width; x++) 
				for (y=0; y< height; y++) {
					// fprintf (stderr,"U: %d V: %d\n",yuv_data[1][y*cwidth+x],yuv_data[2][y*cwidth+x]);
					hist[yuv_data[0][y*width+x]] ++;
				}
			max = hist[0]; choice = 0;
			for (x=0; x< owidth; x++) {
				if (hist[x]>max) {
					max = hist[x];
					choice = x;
				}
			}
		//	mjpeg_debug("max: %d choice %d",max,choice);
			for (x=0; x<owidth; x++) {
				for (y=0; y < (255.0 * hist[x]) / max; y++)
					yuv_odata[0][(oheight-y) * owidth + x] = 224;
			}
			write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_odata );

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
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}


#define MODE_CHANNEL 0
#define MODE_CHROMA 1
#define MODE_LUMA 2
#define MODE_HIST 3
#define MODE_TIMEC 4

// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR ;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	int width, mode, c;
	const static char *legal_flags = "tvyiclhf:";
	
	char *fontname=NULL;
	
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
				case 'i':
					mode = MODE_HIST;
					break;
				case 'y':
					mode = MODE_CHANNEL;
					break;
				case 'c':
					mode = MODE_CHROMA;
					break;
				case 'l':
					mode = MODE_LUMA;
					break;
				case 't':
					mode = MODE_TIMEC;
					break;
			case 'f':
				fontname = (char *) malloc (strlen(optarg));
				strcpy(fontname , optarg);
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
	mjpeg_info ("yuvdiag (version " YUVDI_VERSION ") is a video diagnostic for yuv streams");
	mjpeg_info ("(C) 2008 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info ("yuvdiag -h for help");
	
	y4m_init_stream_info (&out_streaminfo);
	y4m_copy_stream_info(&out_streaminfo, &in_streaminfo);

	
	/* in that function we do all the important work */
	if (mode == MODE_CHANNEL) {
		y4m_si_set_width (&out_streaminfo, y4m_si_get_plane_width(&in_streaminfo,1) + y4m_si_get_plane_width(&in_streaminfo,0));
		
		y4m_write_stream_header(fdOut,&out_streaminfo);
		channel(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
		
	}
	if (mode == MODE_CHROMA) {
		y4m_si_set_width (&out_streaminfo,256);
		y4m_si_set_height (&out_streaminfo,256);

		y4m_write_stream_header(fdOut,&out_streaminfo);
		chroma_scope(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
	}
	
	if (mode == MODE_LUMA) {
		y4m_si_set_height (&out_streaminfo,256 + y4m_si_get_plane_height(&in_streaminfo,0));

		y4m_write_stream_header(fdOut,&out_streaminfo);
		luma_scope(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
	}

	if (mode == MODE_HIST) {
		y4m_si_set_width (&out_streaminfo,256);
		y4m_si_set_height (&out_streaminfo,256);

		y4m_write_stream_header(fdOut,&out_streaminfo);
		acc_hist(fdIn, &in_streaminfo, fdOut, &out_streaminfo);
	}
	if (mode == MODE_TIMEC) {
		
		if (fontname==NULL) 
			mjpeg_error_exit1("no font specified");
		
		y4m_write_stream_header(fdOut,&in_streaminfo);
		timecode(fdIn, &in_streaminfo, fdOut,fontname);
	}

	
	y4m_fini_stream_info (&in_streaminfo);
	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
