/*
 *  generic.c
 *    Mark Heath <mjpeg0 at silicontrip.org>
 *  http://silicontrip.net/~mark/lavtools/
 *
 
** <h3>YUV Subtitles</h3>
** <p> a subtitle rendering utility for yuv streams</p>
** <p> Reads an ascii subtitle file and renders a TTF in the image</p>
** <p> The subtitle file is in the format </p>
** <pre>
**10,100,Leah and the wookie must never again leave this city.
**100,170,That was never a condition of our arrangment.
**</pre>
**<h4>Arguments</h4>
**<pre>
**-f <fontfile> path to TTF file.
**-s <size> size to render the subtitle
**-y <y pos> vertical position on screen
**-c <colour> colour in the format y,u,v
**-u <filename> subtitle file filename
**</pre>
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


#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "utilyuv.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H



struct subtitle {
	
	//in frame counts
	int on;
	int off;
//	char *text;
	unsigned char text[256]; // for testing
};

struct subhead {
	int entries;
	struct subtitle *subs;
};


#define VERSION "0.1"

static void print_usage() 
{
	fprintf (stderr,
			 "usage: yuvsubtitle\n"
			 "\t-f <fontfile> path to TTF file.\n"
			 "\t-s <size> size to render the subtitle\n"
			 "\t-y <y pos> vertical position on screen\n"
			 "\t-c <colour> colour in the format y,u,v\n"
			 "\t-u <filename> subtitle file filename\n"
			);
}

// cheap and nasty UTF-8 to UTF-16 decoder
unsigned int decode_char ( int *p, unsigned char * text)
{
	unsigned int sp;
	
	char special[3];
	sp = text[*p];
//	 mjpeg_debug ("encoded character %d",sp);
	
	// try for a uri style special characters

	if (sp == '%') {
		strncpy(special,&text[*p+1],2);
		special[2] = '\0';
		*p+=2;
		sscanf(special,"%x",&sp);
	} else if ((sp & 224) == 192) {
		if ((text[*p+1] & 192) == 128) {
		//	mjpeg_debug("decode UTF dual character");

			sp = ((sp & 31) << 6) + (text[*p+1] & 63);
			*p++;
		}
	} else if ((sp & 240) == 224) {
		
		if ((text[*p+1] & 192) == 128 && (text[*p+2] & 192) == 128) {
//			mjpeg_debug("decode UTF triple character");
			sp = ((sp & 15) << 12) + ((text[*p+1] & 63) << 6) + (text[*p+2] & 63);
			*p+=2;
		}
	} else if ((sp & 248) == 240) {
		
		if ((text[*p+1] & 192) == 128 && (text[*p+2] & 192) == 128 && (text[*p+3] & 192) == 128) {
//			mjpeg_debug("decode UTF quad character");

			sp = ((sp & 7) << 18) + ((text[*p+1] & 63) << 12)  + ((text[*p+2] & 63) << 6) + (text[*p+3] & 63);
			*p+=3;

		}
	}
	
	//mjpeg_debug ("decoded character %x",sp);

	return sp;
	
}

void ftdims (int *px, int *py, int *lines, FT_Face face, unsigned char * text) {

	
	FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
	FT_UInt       glyph_index;
	int           pen_x, pen_y,n,max,topmax=0,rowmax=0;
	char special[5];
	unsigned int sp;
	uint8_t x,y,z,w;
	
	
	pen_x = 0; max = 0; pen_y = 0;
	for ( n = 0; n < strlen(text); n++ ) {
		
		sp = decode_char(&n,text);
		
		if (sp == 10) {
			if (pen_x>max) max=pen_x;
			pen_x = 0; //pen_y += rowmax + topmax;
			(*lines)++;
		} else if (sp >=32) {
		
			FT_Load_Char( face, sp, FT_LOAD_RENDER );
		
			/* ignore errors */
			pen_x += slot->advance.x;
			pen_y += slot->advance.y; /* not useful for now */
			
			if ( slot->bitmap_top > topmax) topmax=slot->bitmap_top;
			if ( slot->bitmap.rows-slot->bitmap_top > rowmax) rowmax = slot->bitmap.rows-slot->bitmap_top;
			
		}
	}
	if (max > pen_x) pen_x = max;
	*px = pen_x;
	*py = rowmax + topmax;
}

void draw_bitmap (FT_Bitmap*  bitmap, int x, int y, uint8_t *m[3], y4m_stream_info_t *si,  uint8_t yc, uint8_t uc, uint8_t vc )
{
	
	FT_Int  i, j, p, q;
	uint8_t bri, piy,piu,piv;
	FT_Int  x_max;
	FT_Int  y_max;

	int width,height,cx,cy;

	width = y4m_si_get_plane_width(si,0);
	height = y4m_si_get_plane_height(si,0);
	
	x_max = x + bitmap->width;
	y_max = y + bitmap->rows;

	
	for ( i = x, p = 0; i < x_max; i++, p++ )
	{
		for ( j = y, q = 0; j < y_max; j++, q++ )
		{
			if ( i >= width || j >= height )
				continue;
			
			// configurable colour
			bri  = bitmap->buffer[q * bitmap->width + p];
			
			piy = luma_mix(get_pixel (i,j,0,m,si),yc,bri);
			
			cx = xchroma(i,si);
			cy = ychroma(j,si);
			
			piu = chroma_mix(get_pixel (cx,cy,1,m,si),uc,bri);
			piv = chroma_mix(get_pixel (cx,cy,2,m,si),vc,bri);
			
			set_pixel(piy,i,j,0,m,si);
			set_pixel(piu,cx,cy,1,m,si);
			set_pixel(piv,cx,cy,2,m,si);
			
			
		}
	}		
}				  


static void filterframe (uint8_t *m[3], y4m_stream_info_t *si, FT_Face face, unsigned char * text,
						 int pen_y, int yc, int uc, int vc, int yadvance )
{
	
	FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
	FT_UInt       glyph_index;
	int           pen_x,  n,x,y;	
	int error;
	int twidth,theight,lines=0;
	uint8_t bri;
	uint8_t piy,piu,piv;
	int cx,cy;
	int width;
	char special[5];
	int sp;
	
	mjpeg_debug ("text: %s\n",text);

	
	// configurable start location.
	width = y4m_si_get_plane_width(si,0);

	ftdims(&twidth, &theight, &lines, face,text);
	twidth = twidth >> 6;
	
	pen_x =  width / 2 - twidth / 2;
	if (pen_x < 0) {
		mjpeg_warn("text larger than screen width");
		pen_x = 0;
	}
	
	mjpeg_debug ("dims: w: %d h: %d l: %d",twidth,theight,lines);
	
	pen_y -= theight * lines;
	
	
	for ( n = 0; n < strlen(text); n++ )
	{
		/* load glyph image into the slot (erase previous one) */
		
		sp = decode_char(&n,text);		
		
		if (sp == 10) {
			
			//TODO: get correct vertical spacing.
		//	FT_Load_Char( face, sp, FT_LOAD_RENDER );

			pen_x =  width / 2 - twidth / 2;
			pen_y += theight+4; // test value since most of my subs tests are 24

		} else {
			
			FT_Load_Char( face, sp, FT_LOAD_RENDER );
			
			for (sp=0;sp<3;sp++)
			for (cx=-1; cx < 2; cx ++) 
				for (cy =-1; cy<2; cy++) {
					
					draw_bitmap( &slot->bitmap,
								pen_x + slot->bitmap_left + cx + sp,
								pen_y - slot->bitmap_top + cy + sp, 
								m,si,16,128,128 );
					
				}
			/*
			 draw_bitmap( &slot->bitmap,
			 pen_x + slot->bitmap_left - 1,
			 pen_y - slot->bitmap_top - 1, 
			 m,si,16,128,128 );
			 
			 draw_bitmap( &slot->bitmap,
			 pen_x + slot->bitmap_left + 1,
			 pen_y - slot->bitmap_top - 1, 
			 m,si,16,128,128 );
			 
			 draw_bitmap( &slot->bitmap,
			 pen_x + slot->bitmap_left - 1,
			 pen_y - slot->bitmap_top + 1, 
			 m,si,16,128,128 );
			 */
			
			
			
			
			draw_bitmap( &slot->bitmap,
						pen_x + slot->bitmap_left,
						pen_y - slot->bitmap_top,
						m,si,yc,uc,vc );
			
			/* increment pen position */
			pen_x += slot->advance.x >> 6;
			pen_y += slot->advance.y >> 6; /* not useful for now */
		}
		/* now, draw to our target surface */
		
	}
	
}


unsigned char * get_sub (struct subhead s, int fc) {

	int n;
	// mjpeg_info("entries: %d ", s.entries);
	for (n=0; n < s.entries; n++){
	
		// mjpeg_info ("checking %d - %d",s.subs[n].on,  s.subs[n].off);
		
		if (s.subs[n].on <= fc && s.subs[n].off >= fc) 
			return s.subs[n].text;
		
	}
	return '\0';
	
}


static void filter(  int fdIn, int fdOut , y4m_stream_info_t  *inStrInfo, FT_Face     face, struct subhead subs, 
				   int pen_y, int yc, int uc, int vc, int yadvance )
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3] ;
	int                read_error_code ;
	int                write_error_code ;
	int framecounter=0;
	unsigned char *text;
	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,inStrInfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
		
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
	framecounter++;
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
		// do work

	//	mjpeg_info("frame: %d",framecounter);
		if (read_error_code == Y4M_OK) {
			
			text=get_sub(subs,framecounter);
			if (text != '\0') { 
				filterframe(yuv_data,inStrInfo,face,text,pen_y,yc,uc,vc,yadvance);
			}
			write_error_code = y4m_write_frame( fdOut, inStrInfo, &in_frame, yuv_data );
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
		framecounter++;

	}
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

/*
void read_subs_t(struct subhead *s) 
{

	s->entries=5 ;
	
	s->subs =  malloc(sizeof(struct subtitle) * s->entries);
	
	s->subs[0].on = 10;
	s->subs[0].off = 100;
	strcpy(s->subs[0].text,"Leah and the Wookie must never again leave this city.");

	s->subs[1].on = 100;
	s->subs[1].off = 170;
	strcpy(s->subs[1].text,"That was never a condition of our arrangment.");

	s->subs[2].on = 170;
	s->subs[2].off = 220;
	strcpy(s->subs[2].text,"Nor was giving Han to this bounty hunter.");
	
	s->subs[3].on = 230;
	s->subs[3].off = 350;
	strcpy(s->subs[3].text,"I have altered the Deal. Pray I don't alter it any further.");
	
	s->subs[4].on = 368;
	s->subs[4].off = 434; 
	strcpy(s->subs[4].text,"This deal is getting worse all the time.");

}
*/

void edlcount (FILE *file, int *maxline, int *lines)
{
	
	int c;
	int max=0;
	int count=0;
	
	*maxline=0;
	*lines=0;
	
	
	flockfile(file); // for optimising the single character reads
	while (!feof(file)){
		c = getc_unlocked(file);
		count++;
		if (c==10) {
			(*lines)++;
			if (count > *maxline) {
				*maxline = count;
				count=0;
			}
		}
	}
	funlockfile(file);
	rewind(file);
	
}


int read_subs (struct subhead *s, char *filename) {

	FILE *fn;
	char *line;
	int maxline,lines,count=0,c;
	char *p;

	fn = fopen(filename,"r");
	if (fn == NULL) {
		mjpeg_error_exit1("Cannot open subtitle file");
	}
	
	edlcount(fn,&maxline,&lines);

	s->entries=lines ;
	
	s->subs =  malloc(sizeof(struct subtitle) * s->entries);
	
	line = (char *)malloc(maxline);
	if (line == NULL) {
		mjpeg_error("Error allocating line memory");
		fclose (fn);
		return -1;
	}
	
	while (fgets(line,maxline,fn) != NULL) {
	
		sscanf(line,"%d,%d",&s->subs[count].on,&s->subs[count].off);
		for (c=0; c < strlen(line); c++) {
			if (line[c] == ',') {
				p=&line[c+1];
			}
		}
			strcpy(s->subs[count].text, p);
		for (c=0; c < strlen(s->subs[count].text); c++) {
			
		//	mjpeg_debug("copying character: %d",s->subs[count].text[c]);
			
			if (s->subs[count].text[c] < 32) {
				s->subs[count].text[c]=0;		
			}
		}
		
		count++;
	}
	
	free (line);
	
}


// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 1; 
	int top_field =0, bottom_field = 0,double_height=1;
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	y4m_ratio_t frame_rate;
	int interlaced,ilace=0,pro_chroma=0,yuv_interlacing= Y4M_UNKNOWN;
	int height=16;
	int c, pen_y;
	const static char *legal_flags = "hv:f:s:y:c:u:";
	FT_Library  library;
	FT_Face     face;
	struct subhead subs;
	char * subname = NULL;
	int yc,uc,vc;
	
	if (FT_Init_FreeType(&library)) 
		mjpeg_error_exit1("Cannot initialise the freetype library");
	
	// defaults
	pen_y = -1;

	// default text colour
	yc = 235;
	uc = 128;
	vc = 128;
	
	
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'v':
				verbose = atoi (optarg);
				if (verbose < 0 || verbose > 2)
					mjpeg_error_exit1 ("Verbose level must be [0..2]");
				break;
			case 'f':
				c = FT_New_Face( library,
								optarg,
								0,
								&face );
				if ( c == FT_Err_Unknown_File_Format )
				{
					mjpeg_error_exit1("Do not recognise the font file");
				}
				else if ( c )
				{
					mjpeg_error_exit1("Error reading the font file");
				}
				break;
			case 's':
				height = atoi(optarg);
				break;
			case 'y':
				pen_y = atoi(optarg);
				break;
			case 'c':
				sscanf (optarg,"%d,%d,%d",&yc,&uc,&vc);
				break;
			case 'u':
				subname = malloc(strlen(optarg));
				strcpy(subname,optarg);
				break;
			case 'h':
			case '?':
				print_usage (argv);
				return 0 ;
				break;
		}
	}
	
	FT_Set_Pixel_Sizes( face,		/* handle to face object */
						0,			/* pixel_width           */
						height );   /* pixel_height          */
	
	
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
	mjpeg_info ("yuvsubtitle (version " VERSION ") is a subtitle rendering utility for yuv streams");
	mjpeg_info ("(C)  Mark Heath <mjpeg0 at silicontrip.org>");
	// mjpeg_info ("yuvcropdetect -h for help");
	
	if (pen_y==-1) {
		pen_y = y4m_si_get_plane_height(&in_streaminfo,0) - 48;
	}
	
    
	y4m_write_stream_header(fdOut,&in_streaminfo);
	
	if (subname != NULL) {
	// read the subtitle file
		read_subs(&subs,subname);
		filter(fdIn, fdOut, &in_streaminfo,face,subs,pen_y,yc,uc,vc,14 * height / 10);
		// mjpeg_debug ("free subname");
		free (subname);
		// mjpeg_debug ("free subs");

		free (subs.subs);
	} else {
		mjpeg_error_exit1("No subtitle filename specified");
	}
	/* in that function we do all the important work */
	y4m_fini_stream_info (&in_streaminfo);
	
	mjpeg_debug ("done face");

	FT_Done_Face    ( face );
	mjpeg_debug ("done freetype");

	FT_Done_FreeType( library );

	
	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
