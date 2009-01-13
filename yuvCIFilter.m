/*
 *  yuvCIFilter.c
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
 
gcc -I/usr/local/include/mjpegtools -L/usr/local/lib -lmjpegutils -framework QuartzCore -framework Foundation -framework AppKit yuvCIFilter.m

 */


#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "yuv4mpeg.h"
#include "mpegconsts.h"

#import <Foundation/NSAutoreleasePool.h>
#import <QuartzCore/QuartzCore.h>
#import <QuartzCore/CIFilter.h>
#import <QuartzCore/CIRAWFilter.h>
#import <QuartzCore/CIVector.h>
#import <CoreVideo/CVPixelBuffer.h>
#import <QTKit/QTKit.h>
#import <AppKit/NSBitmapImageRep.h>



#define YUV_VERSION "0.1"

NSAutoreleasePool  *pool;

static void print_usage() 
{
	fprintf (stderr,
			"usage: yuv [-v] [-f <filterName>] [-i <key=val>[,<key=val>...]] [-h]\n"
			"yuvCIFilter Allows Core Image Filters on yuv streams\n"
			"\n"
			"\t -f Name of Core Image filter. -f help for filter list\n"
			"\t -i Core Image filter settings.\n"
			"\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
			"\t -h print this help\n"
			 );
}

static void list_filters() 
{

	NSArray *flist;
	NSEnumerator * e;
	id o;

	flist = [CIFilter filterNamesInCategories:nil];
	e = [flist objectEnumerator];

	fprintf (stderr,"FILTER LIST:\n");

	while (o=[e nextObject])
		fprintf (stderr,"%s\n",[o UTF8String] );
}


// Allocate a uint8_t frame
int chromalloc(uint8_t *m[3],y4m_stream_info_t *sinfo)
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
int chromacpy(uint8_t *m[3],uint8_t *n[3],y4m_stream_info_t *sinfo)
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	memcpy (m[0],n[0],fs);
	memcpy (m[1],n[1],cfs);
	memcpy (m[2],n[2],cfs);
	
}

// set a solid colour for a uint8_t frame
set_colour(uint8_t *m[], y4m_stream_info_t  *sinfo, int y, int u, int v )
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	memset (m[0],y,fs);
	memset (m[1],u,cfs);
	memset (m[2],v,cfs);
	
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

/* looks like I need to write my own RGB - YUV converter

* ITU-R 601 for SDTV:

      Y = (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
      U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128
      V = (0.439 * R) - (0.368 * G) - (0.071 * B) + 128

   YUV to RGB Conversion:

      B = 1.164(Y - 16) + 2.018(U - 128)
      G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
      R = 1.164(Y - 16) + 1.596(V - 128)

* ITU-R 709 for HDTV:

   RGB to YUV Conversion:

      Y = (0.183 * R) + (0.614 * G) + (0.062 * B) + 16
      U = -(0.101 * R) - (0.338 * G) + (0.439 * B) + 128
      V = (0.439 * R) - (0.399 * G) - (0.040 * B) + 128

   YUV to RGB Conversion:

      B = 1.164(Y - 16) + 1.793(U - 128)
      G = 1.164(Y - 16) - 0.534(V - 128) - 0.213(U - 128)
      R = 1.164(Y - 16) + 2.115(V - 128)
*/

int argbYUV(uint8_t **m, NSBitmapImageRep *bit, y4m_stream_info_t *si) 
{

	uint8_t yc,uc,vc;
	uint16_t ut,vt;
	unsigned char r,g,b;
	uint8_t *n[3];
	unsigned char *d;
	
	int bpp;
	int x,y,fs;
	int height, width;

	height = y4m_si_get_height(si);
	width = y4m_si_get_width(si);

	d=[bit bitmapData];
	bpp=[bit bytesPerRow];


// Allocate a yuv444 buffer

	fs = y4m_si_get_plane_length(si,0);
	
	n[0] = (uint8_t *)malloc(fs);
	n[1] = (uint8_t *)malloc(fs);
	n[2] = (uint8_t *)malloc(fs);
	
	if( !n[0] || !n[1] || !n[2]) {
		return -1;
	}

// convert to YUV444


	for (y=0; y< height; y++) {
		for(x=0; x<width; x++) {
	
			r = d[y * bpp + (x<<2) + 1];
			g = d[y * bpp + (x<<2) + 2];
			b = d[y * bpp + (x<<2) + 3];
			
			// if (ITU601) {
			// optimize this, do want
			// would integer calculations work?
				yc =((16843 * r) + (33030 * g) + (6423 * b)>>16) + 16;
				uc =(-(9699 * r) - (19071 * g) + (28770 * b)>>16) + 128;
				vc =((28770 * r) - (24117 * g) - (4653 * b)>>16) + 128;
/*
				yc =  (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
				uc = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;
				vc =  (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
*/

			n[0][y * width + x] = yc;
			n[1][y * width + x] = uc;
			n[2][y * width + x] = vc;
		}
	}

// copy luma 

		memcpy (m[0],n[0],fs);

// do nasty subsampling
// forced to 420 subsampling

	for (y=0; y<(height-2); y+=2) {
		for (x=0; x<(width-2); x+=2) {
		
//		fprintf(stderr,"argbYUV %d %d\n",x,y);

		// average 4 chroma pixels to 1
		// sum
			ut = n[1][y * width + x] + n[1][(y+1) * width + x] + n[1][y * width + x+1] + n[1][(y+1) * width + x+1] ;
			vt = n[2][y * width + x] + n[2][(y+1) * width + x] + n[2][y * width + x+1] + n[2][(y+1) * width + x+1] ;
			
			// divide by 4 and put back into buffer
			m[1][(y>>1) * (width>>1) + (x>>1)] =  ut >> 2;
			m[2][(y>>1) * (width>>1) + (x>>1)] =  vt >> 2;
		}
	}
	
	free (n[0]);
	free (n[1]);
	free (n[2]);
	
	return 0;
}


void yuvCVcopy(CVPixelBufferRef cf,  uint8_t ** m, y4m_stream_info_t *si) 
{
	
	size_t y,x,w,h,b;
	uint8_t cy0,cy1,cu,cv;
	OSType pixelFormat;
	uint8_t *buffer;
	
//	NSLog(@"yuvCVcopy");
	// I assume that plane 0 is larger or equal to the other planes
	
	pixelFormat=CVPixelBufferGetPixelFormatType(cf);
	switch (pixelFormat) {
			
		case 'y420':
			for(y=0; y< CVPixelBufferGetHeightOfPlane(cf,0); y++ ) {
				
				memcpy(CVPixelBufferGetBaseAddressOfPlane(cf,0) + y * CVPixelBufferGetBytesPerRowOfPlane(cf,0), 
					   m[0] + y * CVPixelBufferGetWidthOfPlane(cf,0),
					   CVPixelBufferGetWidthOfPlane(cf,0));
				
				// I assume that the U and V planes are sub sampled at the same rate
				if (y < CVPixelBufferGetHeightOfPlane(cf,1)) {
					memcpy(CVPixelBufferGetBaseAddressOfPlane(cf,1) + y * CVPixelBufferGetBytesPerRowOfPlane(cf,1),
						   m[1] + y * CVPixelBufferGetWidthOfPlane(cf,1),
						   CVPixelBufferGetWidthOfPlane(cf,1));
					memcpy(CVPixelBufferGetBaseAddressOfPlane(cf,2) + y * CVPixelBufferGetBytesPerRowOfPlane(cf,2),
						   m[2] + y * CVPixelBufferGetWidthOfPlane(cf,2),
						   CVPixelBufferGetWidthOfPlane(cf,2));
				}
				
			}
			break;
			case 'yuvs':
			
			h = CVPixelBufferGetHeight(cf);
			w = CVPixelBufferGetWidth(cf);
			b = CVPixelBufferGetBytesPerRow(cf);
			buffer = (uint8_t *) CVPixelBufferGetBaseAddress(cf);
		//	int sz = CVPixelBufferGetDataSize(cf);
			
			// NSLog(@"yuvCVcopy: BytesPerRow: %d %dx%d size: %d",b,w,h,sz);
			
			//optimise this, do want.
			for(y=0; y< h; y++ ) {
				for(x=0; x< (w>>1); x++ ) {
					
					// planar vs packed, it's such a religious argument.
					cy0 = m[0][(x<<1) + y * w];
					cy1 = m[0][(x<<1) + 1 + y * w];
					
					//  assuming 420 sub sampling
					if (y4m_si_get_interlace(si) == Y4M_ILACE_NONE) {
						// Progressive, 1,1,2,2,3,3,4,4
						cu = m[1][x + (y>>1) * (w>>1)];					
						cv = m[2][x + (y>>1) * (w>>1)];
					} else {
						// Interlaced, 1,2,1,2,3,4,3,4
						// 0,1,2,3,4,5,6,7  y
						// 0,0,0,0,1,1,1,1  >>2
						// 0,0,0,0,2,2,2,2  >>2 <<1
						// 0,1,0,1,2,3,2,3  >>2 <<1 + %2
						
						// 0,0,1,1,2,2,3,3 >>1
						// 0,1,1,2,2,3,3,4 >>1 + %2
						
						// I think this interlacing algorithm is correct.
						cu = m[1][x + (((y>>2)<<1) + (y % 2)) * (w>>1)];					
						cv = m[2][x + (((y>>2)<<1) + (y % 2)) * (w>>1)];
					}
					
					// lets see if this is correct
			//		NSLog(@"yuvCVcopy: %d %dx%d",b,y,x);
					buffer[y * b + (x << 2)] = cy0;
					buffer[y * b + (x << 2)  + 1] = cu;
					buffer[y * b + (x << 2) + 2] = cy1;
					buffer[y * b + (x << 2) + 3] = cv;
				}
			}
			break;
			default:
			// I'm guessing this bit of code is not Endian independent??
				NSLog(@"No code to handle Pixelbuffer type: %c%c%c%c",pixelFormat>>24,pixelFormat>>16,pixelFormat>>8,pixelFormat);
			break;
	} 	
}


static void filter(  int fdIn, int fdOut,
					y4m_stream_info_t  *soutfo,
					y4m_stream_info_t  *sinfo,
					CIFilter* effectFilter)
{
	y4m_frame_info_t   in_frame ;
	uint8_t            *yuv_data[3], *yuv_odata[3] ;
	unsigned char *data;
	int                read_error_code ;
	int                write_error_code ;
	int b,bufsize;
	CVPixelBufferRef        currentFrame;
	CVReturn				cvError;
	
	CGRect      imageRect;
	CIImage     *inputImage,  *outputImage;
	
	
	// Allocate memory for the YUV channels
	
	if (chromalloc(yuv_data,sinfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");
	
	if (chromalloc(yuv_odata,sinfo))		
		mjpeg_error_exit1 ("Could'nt allocate memory for the YUV4MPEG data!");

	
	// Create the Core Video Buffer
	cvError = CVPixelBufferCreate(NULL,
								  y4m_si_get_width(sinfo),
								  y4m_si_get_height(sinfo),
								  kYUVSPixelFormat,
								  NULL,
								  &currentFrame);
	
	if (cvError != kCVReturnSuccess) 
		mjpeg_error_exit1 ("Could'nt create CoreVideo buffer!");
	
	CVPixelBufferLockBaseAddress(currentFrame,0);
	
	
	
	/* Initialize counters */
	
	write_error_code = Y4M_OK ;
	
	y4m_init_frame_info( &in_frame );
	read_error_code = y4m_read_frame(fdIn, sinfo,&in_frame,yuv_data );
	
	NSBitmapImageRep* bir = [NSBitmapImageRep alloc];
	inputImage = [CIImage emptyImage];
	
	while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
		
	//	fprintf (stderr,"filter: loop\n");
		
		NSAutoreleasePool *loopPool = [[NSAutoreleasePool alloc] init];
		
		// do work
		if (read_error_code == Y4M_OK) {
			
			// convert yuv_data to Core Video data
			
			yuvCVcopy(currentFrame,yuv_data,sinfo);


			[inputImage initWithCVImageBuffer:currentFrame];
			[effectFilter setValue:inputImage forKey:@"inputImage"];
			
			// Convert Core Image to core video.
			
			/* convert core image to nsbitmaprep */
			if (effectFilter == nil) {
				fprintf (stderr,"filter: effectFilter is nil\n");
			}
			outputImage = [effectFilter valueForKey:@"outputImage"];
			
			imageRect = [outputImage extent];
			if (imageRect.size.width > y4m_si_get_width(sinfo) &&
				imageRect.size.height > y4m_si_get_height(sinfo)) {
				CIFilter *crop = [CIFilter filterWithName:@"CICrop"];
				[crop setValue:[CIVector vectorWithX:0
				   Y:0
				   Z:y4m_si_get_width(sinfo)
				   W:y4m_si_get_height(sinfo)]
				forKey:@"inputRectangle"];
				[crop setValue:outputImage forKey:@"inputImage"];
				outputImage = [crop valueForKey:@"outputImage"];

			}

		//	fprintf (stderr,"filter extent: %f %f \n", imageRect.size.width, imageRect.size.height);
			
			[bir initWithCIImage:outputImage];
			
		//	fprintf (stderr,"filter: data=\n");

			
			//data = [bir bitmapData];
						
			// Convert Core Video to yuv_data 
		
			argbYUV(yuv_data,bir,sinfo);
			
			write_error_code = y4m_write_frame(fdOut, sinfo, &in_frame, yuv_data );		
		}
		
		y4m_fini_frame_info( &in_frame );
		y4m_init_frame_info( &in_frame );
		read_error_code = y4m_read_frame(fdIn, sinfo,&in_frame,yuv_data );
		
		[loopPool release];

		
	}
	// Clean-up regardless an error happened or not
	y4m_fini_frame_info( &in_frame );
	
	CVPixelBufferUnlockBaseAddress(currentFrame,0);
	CVPixelBufferRelease(currentFrame);
	
	free( yuv_data[0] );
	free( yuv_data[1] );
	free( yuv_data[2] );
	
	if( read_error_code != Y4M_ERR_EOF )
		mjpeg_error_exit1 ("Error reading from input stream!");
	
}

// parse filter key vaue pairs

void parse_filter(char *filter) {


}

// *************************************************************************************
// MAIN
// *************************************************************************************
int main (int argc, char *argv[])
{
	
	int verbose = 4; // LOG_ERROR 
	int fdIn = 0 ;
	int fdOut = 1 ;
	y4m_stream_info_t in_streaminfo, out_streaminfo ;
	int c ;
	int yuv_interlacing;
	const static char *legal_flags = "vhf:i:";
	CIFilter* effectFilter;
	char *effectString;
	NSString* optString;
	
	effectString = NULL;
	
	pool = [[NSAutoreleasePool alloc] init];
	
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
				// convert Cstring into NSString
				
				optString = [NSString stringWithUTF8String:optarg];
				effectFilter = [[CIFilter filterWithName:optString] retain];
				[effectFilter setDefaults];
				
				if (effectFilter == nil) {
					list_filters();
					mjpeg_error_exit1 ("Unknown Effect Filter %s",optarg);
				}
				break;
			case 'i':
				// make a copy of the input string
				// for later parsing, as the i option may be entered before the f option
				effectString = (char *)malloc(strlen(optarg)+1);
				strncpy(effectString, optarg,strlen(optarg));
				
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
		}
	}
	
	// mjpeg tools global initialisations
	mjpeg_default_handler_verbosity (verbose);
	
	// Initialize input streams
	y4m_init_stream_info (&in_streaminfo);
	y4m_init_stream_info (&out_streaminfo);

	
	// parse the effectString 
	
	if (effectString) {
	
		// parse
		NSString *ap;
		NSArray *arguments;
		NSEnumerator *e;
		id o;

		ap = [NSString stringWithUTF8String:effectString];

		arguments = [ap componentsSeparatedByString:@","];

		e = [arguments objectEnumerator];
	
		while (o = [e nextObject]) {
	
			NSArray *keyval = [o componentsSeparatedByString:@"="];
			mjpeg_info("arg: %s=%s", [[keyval objectAtIndex:0] UTF8String], [[keyval objectAtIndex:1] UTF8String]);
		
			[effectFilter setValue:[keyval objectAtIndex:1] forKey:[keyval objectAtIndex:0]];

	
		}
				
	}
// print settings
	mjpeg_info ("FILTER: %s\n",[[effectFilter description] UTF8String]);
/*
	NSDictionary *ikd = [effectFilter dictionaryWithValuesForKeys:[effectFilter inputKeys]];
	NSEnumerator * e = [ikd keyEnumerator];
	id o;
	while (o = [e nextObject]) {
		fprintf(stderr,"%s=%s\n",[o UTF8String],[[[ikd objectForKey:o] description] UTF8String]);	
	}
*/
	
	// ***************************************************************
	// Get video stream informations (size, framerate, interlacing, aspect ratio).
	// The streaminfo structure is filled in
	// ***************************************************************
	// INPUT comes from stdin, we check for a correct file header
	if (y4m_read_stream_header (fdIn, &in_streaminfo) != Y4M_OK)
		mjpeg_error_exit1 ("Could'nt read YUV4MPEG header!");
	
	y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
	
	// Information output
	mjpeg_info ("yuvCIFilter (version " YUV_VERSION ") Allows Core Image filters on yuv streams");
	mjpeg_info ("(C) 2008 Mark Heath <mjpeg0 at silicontrip.org>");
	mjpeg_info ("yuvCIFilter -h for help");
	
	y4m_write_stream_header(fdOut,&out_streaminfo);
	
	/* in that function we do all the important work */
	filter(fdIn, fdOut, &in_streaminfo, &out_streaminfo, effectFilter);
	
	y4m_fini_stream_info (&in_streaminfo);
	y4m_fini_stream_info (&out_streaminfo);

	[pool release];

	return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
