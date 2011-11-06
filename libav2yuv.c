/*
// libav2yuv
// adapted from
// avcodec_sample.0.4.9.cpp

// A small sample program that shows how to use libavformat and libavcodec to
// read video from a file.
//
// This version is for the 0.4.9-pre1 release of ffmpeg. This release adds the
// av_read_frame() API call, which simplifies the reading of video frames 
// considerably. 
//
// gcc -O3 -I/usr/local/include -I/usr/local/include/mjpegtools -lavcodec -lavformat -lavutil -lmjpegutils libav2yuv.c -o libav2yuv
// quadrant gcc -O3 -I/sw/include -I/sw/include/mjpegtools -L/sw/lib -lavcodec -lavformat -lavutil -lmjpegutils libav2yuv.c -o libav2yuv 
// gcc -O3 -I/opt/local/include -I/usr/local/include/mjpegtools -L/opt/local/lib -lavcodec -lavformat -lavutil -lmjpegutils libav2yuv.c -o libav2yuv
//
// valgrind build

gcc -g -I/opt/local/include -I/usr/local/include/mjpegtools libav2yuv.c -c
gcc -g -I/opt/local/include -I/usr/local/include/mjpegtools -L/opt/local/lib -lavcodec -lavformat -lavutil -lmjpegutils libav2yuv.o -o libav2yuv
dsymutil libav2yuv

// I really should put history here
// 5th Jul 2010 - Force framerate command line argument not parsed correctly
// 5th Apr 2009 - First regression test passed.  EDL version.
// 18th Mar 2009 - Audio range fixed, sample accurate.
// 17th Mar 2009 - Multifile version.
// 4th Feb 2009 - Range version. Audio range not working
// 2nd Feb 2009 - Audio writing version.
// 7th July 2008 - Added Force Format option 
// 4th July 2008 - Added Aspect Ratio Constants
// 3rd July 2008 - Will choose the first stream found if no stream is specified  
// 24th Feb 2008 - Found an unexpected behaviour where frames were being dropped. libav said that no frame was decoded. Have output the previous frame in this instance.
//
*/

/* Possible inclusion for EDL
 Comments
 
 Comments can appear at the beginning of the EDL file (header) or between the edit lines in the EDL. The first block of comments in the file is defined to be the header comments and they are associated with the EDL as a whole. Subsequent comments in the EDL file are associated with the first edit line that appears after them.
 Edit Entries
 
 <filename|tag>  <EditMode>  <TransitionType>[num]  [duration]  [srcIn]  [srcOut]  [recIn]  [recOut]
 <filename|tag>: Filename or tag value. Filename can be for an MPEG file, Image file, or Image file template. Image file templates use the same pattern matching as for command line glob, and can be used to specify images to encode into MPEG. i.e. /usr/data/images/image*.jpg
 <EditMode>: 'V' | 'A' | 'VA' | 'B' | 'v' | 'a' | 'va' | 'b' which equals Video, Audio, Video_Audio edits (note B or b can be used in place of VA or va).
 <TransitonType>: 'C' | 'D' | 'E' | 'FI' | 'FO' | 'W' | 'c' | 'd' | 'e' | 'fi' | 'fo' | 'w'. which equals Cut, Dissolve, Effect, FadeIn, FadeOut, Wipe.
 [num]: if TransitionType = Wipe, then a wipe number must be given. At the moment only wipe 'W0' and 'W1' are supported.
 [duration]: if the TransitionType is not equal to Cut, then an effect duration must be given. Duration is in frames.
 [srcIn]: Src in. If no srcIn is given, then it defaults to the first frame of the video or the first frame in the image pattern. If srcIn isn't specified, then srcOut, recIn, recOut can't be specified.
 [srcOut]: Src out. If no srcOut is given, then it defaults to the last frame of the video - or last image in the image pattern. if srcOut isn't given, then recIn and recOut can't be specified.
 [recIn]: Rec in. If no recIn is given, then it is calculated based on its position in the EDL and the length of its input.
 
 [recOut]: Rec out. If no recOut is given, then it is calculated based on its position in the EDL and the length of its input. first frame of the video.
 For srcIn, srcOut, recIn, recOut, the values can be specified as either timecode, frame number, seconds, or mps seconds. i.e. 
 [tcode | fnum | sec | mps], where:
 tcode : SMPTE timecode in hh:mm:ss:ff
 fnum : frame number (the first decodable frame in the video is taken to be frame 0).
 sec : seconds with 's' suffix (e.g. 5.2s)
 mps : seconds with 'mps' suffix (e.g. 5.2mps). This corresponds to the 'seconds' value displayed by Windows MediaPlayer.
 */


#include <yuv4mpeg.h>
#include <mpegconsts.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <regex.h>

#define BYTES_PER_SAMPLE 4

#define PAL "PAL"
#define PAL_WIDE "PAL_WIDE"
#define NTSC "NTSC"
#define NTSC_WIDE "NTSC_WIDE"

/*
struct commandlineopts {
	&yuv_interlacing,&yuv_frame_rate,&yuv_aspect, &yuv_ss_mode,&fdOut,
	&audioWrite,&search_codec_type,&convert,&stream,avif,&rangeString,&subRange
};
*/
 
struct edlentry {
	char *filename;
	char audio;
	char video;
	char *in;
	char *out;
};

// ^([^ /]+) ([AVBavb]|VA|va) (C) ([0-9]*:?[0-9]*:?[0-9]*[;:]?[0-9]+) ([0-9]*:?[0-9]*:?[0-9]*[;:]?[0-9]+) 

// going to use the regex library

// 00:00:00;00

// I'm not sure if this code is smaller than my non regex code
// but lessons learnt here will be useful for the EDL parser

int64_t parseTimecodeRE (char *tc, int frn, int frd) {
	
	//	char *pattern = "^([0-9][0-9]*)(:)([0-9][0-9]*)(:)([0-9][0-9]*)([:;])([0-9][0-9]*)$";
	//	char *pattern = "^([0-9]*)(:?)([0-9]*)(:?)([0-9]*)([:;]?)([0-9]+)$";
	char *pattern = "^([0-9]*)(^|:)([0-9]*)(^|:)([0-9]*)(^|[:;])([0-9]+).*$";
	//	char *pattern = "^\([0-9]*\)\(:?\)\([0-9]*\)\(:?\)\([0-9]*\)\([:;]?\)\([0-9]+\)$";
	//	char *pattern = "^([0-9]+)(:)([0-9]+)(:)([0-9]+)([:;])([0-9]+)$";
	
	regex_t tc_reg;
	int h=0,m=0,s=0,f=0,le,off,fn;
	size_t num=8;
	regmatch_t codes[8];
	int nummatch;
	float fps,frameNumber;
	int64_t fn64;
	
	//	fprintf (stderr, "REGCOMP %s\n",pattern);
	
	if (regcomp(&tc_reg, pattern, REG_EXTENDED) != 0) {
		fprintf (stderr, "REGEX compile failed\n"); // since I know that the REGEX is correct, what else would cause this.
		return -1;
	}
	
	// fprintf (stderr, "REGEXEC %s\n",tc);
	
	nummatch = regexec(&tc_reg, tc, num, codes, 0 );
	if ( nummatch != 0) {
		fprintf (stderr, "parser: error REGEX match failed\n");
		return -1;
	}
	/*
	 for (f=0; f<num; f++)  {
	 le =codes[f].rm_eo-codes[f].rm_so;
	 off = codes[f].rm_so;
	 fprintf (stderr,"%d: from %lld to %lld (%.*s)\n",f,codes[f].rm_so,codes[f].rm_eo,le,tc+off);
	 }
	 */
	
	if ( 1.0 * frn / frd == 30000.0 / 1001.0) {
		
		// or is this a : ?
		if (tc[codes[6].rm_so] == ':') {
			fprintf (stderr,"parser: NTSC Drop Code\n");
			frn = 30;
			frd = 1;
		}
	}
	fps = 1.0 * frn / frd;		
	
	
	// split the string by converting the : into nulls
	
	for (f=2; f < 7; f+=2) 
		if (codes[f].rm_eo != 0) {
			tc[codes[f].rm_so] = '\0';
		}
	
	// convert into integers
	if (codes[7].rm_eo != 0) 
		f = atoi(tc+codes[7].rm_so);
	
	if (codes[5].rm_eo != 0) 
		s = atoi(tc+codes[5].rm_so);
	
	if (codes[3].rm_eo != 0) 
		m = atoi(tc+codes[3].rm_so);
	
	if (codes[1].rm_eo != 0) 
		h = atoi(tc+codes[1].rm_so);
	
	//	fprintf (stderr," %d - %d - %d - %d \n",h,m,s,f);
	
	regfree( &tc_reg );
	
	if ((h>0 && m>59) ||  (m>0 && s>59) || (s>0 && f >= fps))  {
		fprintf (stderr,"parser error: timecode digit too large\n");
		return -1;
	}
	
	frameNumber =   1.0 * ( h * 3600 + m * 60 + s ) * fps + f;
	fn = (frameNumber);
	
	fn64 = fn;
	
	//	fprintf (stderr,"parser: framenumber %d == %lld\n",fn,fn64);
	
	return fn64;
	
}

int splitTimecode(char **s, char **e, char *rs) {
	
	
	int dashcount = 0;
	int dashplace = 0;
	int i;
	
	for (i=0; i<strlen(rs); i++) {
		if (rs[i] == '-') {
			dashcount ++;
			dashplace = i;
		}
	}
	
	if (dashcount == 1) {
		*e = rs + dashplace + 1;
		*s = rs;
		rs[dashplace] = '\0';
		return 0;
	}		
	return -1;
}

int parseEDLline (char *line, char **fn, char *audio, char *video, char **in, char **out)
{
	
	regex_t tc_reg;
	size_t num=10;
	regmatch_t codes[10];
	int rc,f;
	int le,off;
	char *va;
	
	// char *pattern = "^([^ ]+)( +)([AVBavb]|VA|va)( +)(C)( +)([0-9]*:?[0-9]*:?[0-9]*[;:]?[0-9]+)( +)([0-9]*:?[0-9]*:?[0-9]*[;:]?[0-9]+)$";
	char *pattern = "^([^ ]+)( +)([AVBavb]|VA|va)( +)(C)( +)([0-9]*:?[0-9]*:?[0-9]*[;:]?[0-9]+)( +)([0-9]*:?[0-9]*:?[0-9]*[;:]?[0-9]+).*$";
	
	if (regcomp(&tc_reg, pattern, REG_EXTENDED) != 0) {
		fprintf (stderr, "REGEX compile failed\n");
		return -1;
	}
	
	*audio = 0;
	*video = 0;
	*in = 0;
	*out = 0;
	*fn=0;
	
	//	fprintf (stderr, "REGEXEC %s\n",tc);
	
	rc = regexec(&tc_reg, line, num, codes, 0 );
	if ( rc != 0) {
		fprintf (stderr, "parser: EDL error REGEX match failed\n");
		return -1;
	}
	
	/*
	 for (f=0; f<num; f++)  {
	 le =codes[f].rm_eo-codes[f].rm_so;
	 off = codes[f].rm_so;
	 fprintf (stderr,"%d: from %lld to %lld (%.*s)\n",f,codes[f].rm_so,codes[f].rm_eo,le,line+off);
	 }
	 */
	
	for (f=2; f <= 8; f+=2) 
		if (codes[f].rm_eo != 0) {
			line[codes[f].rm_so] = '\0';
		}
	
	*fn = line+codes[1].rm_so;
	*in = line+codes[7].rm_so;
	*out = line + codes[9].rm_so;
	
	va = line+codes[3].rm_so;
	
	//	fprintf (stderr,"EDITMODE: %s\n",va);
	
	if (!strcmp(va,"VA") || !strcmp(va,"va") || va[0]=='B' || va[0]=='b') {
		*audio = 1;
		*video = 1;
	} else if (va[0]=='V' || va[0]=='v') {
		*video = 1;
	} else if (va[0]=='A' || va[0]=='a') {
		*audio = 1;
	}
	
	return 0;
	
}

int edlcount (FILE *file, int *maxline, int *lines)
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

int parseEDL (char *file, struct edlentry **list)
{
	
	FILE *fh;
	char *line;
	int maxline,lines,count=0;
	char *fn,*in,*out;
	char ema,emv;
	struct edlentry *inner;
	
	fh = fopen(file,"r");
	if (fh == NULL) {
		mjpeg_error ("Opening EDL file");
		return -1;
	}
	
	//	count active lines
	//	count active characters
	
	edlcount(fh,&maxline,&lines);
	
	// should sanity check maxline and lines
	
	// malloc edlentry array 
	//	malloc read buffer
	
	line = (char *)malloc(maxline);
	if (line == NULL) {
		mjpeg_error("Error allocating line memory");
		fclose (fh);
		return -1;
	}

	*list = (struct edlentry *)malloc(lines*sizeof(struct edlentry));
	inner = *list;
	if (line == NULL) {
		mjpeg_error("Error allocating edl memory\n");
		free(line);
		fclose (fh);
		return -1;
	}
	
	while (fgets(line,maxline,fh) != NULL) {
		
		//		parse line
		
		if (parseEDLline (line, &fn, &ema, &emv,&in,&out) == -1) {
			mjpeg_error("Error in EDL file line: %d: %s\n",count+1,line);
		} else {
			//	fprintf (stderr,"Parsing line: %d\n",count);
			//	malloc filename
			mjpeg_debug("malloc filename.");

			inner[count].filename = (char *)malloc(strlen(fn)+1);
			if (inner[count].filename == NULL) {
				mjpeg_error("Error allocating edl filename memory");
				free(line);
				free(list);
				fclose(fh);
				return -1;
			}
			
			//	check file readable.
			
			// cannot parse timecode at this point as we have no knowledge of the frame rate.
			//	parse timecode;
			//	check in < out

			inner[count].in = (char *)malloc(strlen(in)+1);
			if (inner[count].in == NULL) {
				mjpeg_debug("Error allocating edl timecode memory");
				free(line);
				// grr memory leak
				// free(list.filename);
				free(list);
				fclose(fh);
				return -1;
			}

			inner[count].out = (char *)malloc(strlen(out)+1);
			if (inner[count].out == NULL) {
				mjpeg_error("Error allocating edl filename memory.");
				free(line);
				//			free(list.filename);
				//			free(list.in);
				free(list);
				fclose(fh);
				return -1;
			}
			// copy values to struct.
			
			strcpy(inner[count].filename,fn);
			strcpy(inner[count].in,in);
			strcpy(inner[count].out,out);
			
			//	fprintf (stderr,"a: %d v: %d\n",ema,emv);
			
			inner[count].audio = ema;
			inner[count].video = emv;
			
			count++;
			// fprintf (stderr,"End of loop\n");
		}
	}
	return count;
}


void avchromacpy (uint8_t *dst[3], AVFrame *src, y4m_stream_info_t *sinfo)
{
	
	int y,h,w;
	int cw,ch;
	
	w = y4m_si_get_plane_width(sinfo,0);
	h = y4m_si_get_plane_height(sinfo,0);
	cw = y4m_si_get_plane_width(sinfo,1);
	ch = y4m_si_get_plane_height(sinfo,1);
	
	//mjpeg_debug ("copy %d bytes to: %x from: %x",w,dst[0]+y*w,(src->data[0])+y*src->linesize[0]);
	
	
	for (y=0; y<h; y++) {
		//		mjpeg_debug ("copy %d bytes to: %x from: %x",w,dst[0]+y*w,(src->data[0])+y*src->linesize[0]);
		
		memcpy(dst[0]+y*w,(src->data[0])+y*src->linesize[0],w);
		if (y<ch) {
#ifdef DEBUG
			mjpeg_debug("copy %d bytes to: %x from: %x",cw,dst[1]+y*cw,(src->data[1])+y*src->linesize[1]);
#endif
			memcpy(dst[1]+y*cw,(src->data[1])+y*src->linesize[1],cw);
			memcpy(dst[2]+y*cw,(src->data[2])+y*src->linesize[2],cw);
		}
	}
	
}

/*
void chromalloc(uint8_t *m[3],y4m_stream_info_t *sinfo)
{	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	
	m[0] = (uint8_t *)malloc( fs );
	m[1] = (uint8_t *)malloc( cfs);
	m[2] = (uint8_t *)malloc( cfs);
	
	mjpeg_debug("alloc yuv_data: %x,%x,%x",m[0],m[1],m[2]);

	
}
*/
static void print_usage() 
{
	fprintf (stderr,
			 "usage: libav2yuv [-s<stream>] [-Ip|b|t] [-F<rate>] [-A<aspect>] [-S<chroma>] [-o<outputfile] <filename>\n"
			 "converts any media file recognised by libav to yuv4mpeg stream\n"
			 "\n"
			 "\t -w Write a PCM file not a video file\n"
			 "\t -I<pbt> Force interlace mode overides parameters read from media file\n"
			 "\t -F<n:d> Force framerate\n"
			 "\t -f <fmt> Force format type (if incorrectly detected)\n"
			 "\t -A(<n:d>|PAL|PAL_WIDE|NTSC|NTSC_WIDE) Force aspect ratio\n"
			 "\t -S<chroma> Force chroma subsampling mode\n"
			 "\t   if the mode in the stream is unsupported will upsample to YUV444\n"
			 "\t -c Force conversion to chroma mode (requires a chroma mode)\n"
			 "\t -s select stream other than stream 0\n"
			 "\t -o<outputfile> write to file rather than stdout\n"
			 "\t -r [[[HH:]MM:]SS:]FF-[[[HH:]MM:]SS:]FF playout only these frames\n"
			 "\t -E enable y4m extensions (may be required if source file is not a common format)\n"
			 "\t -h print this help\n"
			 );
}

int parseCommandline (int argc, char *argv[], 
					  int *yi,
					  y4m_ratio_t *yfr,
					  y4m_ratio_t *ya,
					  int *ysm,
					  int *fdOut,
					  int *aw,
					  int *sct,
					  int *con,
					  int *str,
					  AVInputFormat *av,
					  char **rs,
					  int *sr)
{
	
	int i;
	const static char *legal_flags = "EwchI:F:A:S:o:s:f:r:e:v:";
	
	*aw=0;
	*sct=AVMEDIA_TYPE_VIDEO;
	*yi = Y4M_UNKNOWN;
	*ysm = Y4M_UNKNOWN;
	*fdOut = 1;
	*con = 0;
	*str = 0;
	*sr = 0;
	av = NULL;
	
	
	// Parse commandline arguments
	while ((i = getopt (argc, argv, legal_flags)) != -1) {
		switch (i) {
			case 'E':
				y4m_accept_extensions(1); // manual override 
				break;
			case 'I':
				switch (optarg[0]) {
					case 'P':
					case 'p':  *yi = Y4M_ILACE_NONE;  break;
					case 'T':
					case 't':  *yi = Y4M_ILACE_TOP_FIRST;  break;
					case 'B':
					case 'b':  *yi = Y4M_ILACE_BOTTOM_FIRST;  break;
					default:
						mjpeg_error("Unknown value for interlace: '%c'", optarg[0]);
						return -1;
						break;
				}
				break;
			case 'F':
				if( Y4M_OK != y4m_parse_ratio(yfr, optarg) ) {
					mjpeg_error_exit1 ("Syntax for frame rate should be Numerator:Denominator");
					return -1;
				}
                break;
			case 'A':
				if( Y4M_OK != y4m_parse_ratio(ya, optarg) ) {
					if (!strcmp(optarg,PAL)) {
						y4m_parse_ratio(ya, "128:117");
					} else if (!strcmp(optarg,PAL_WIDE)) {
						y4m_parse_ratio(ya, "512:351");
					} else if (!strcmp(optarg,NTSC)) {
						y4m_parse_ratio(ya, "4320:4739");
					} else if (!strcmp(optarg,NTSC_WIDE)) {
						y4m_parse_ratio(ya, "5760:4739");
					} else {
						mjpeg_error("Syntax for aspect ratio should be Numerator:Denominator");
						mjpeg_error("Or "PAL", "PAL_WIDE", "NTSC" and "NTSC_WIDE".");
						return -1;
					}
				}
				break;
			case 'S':
				*ysm = y4m_chroma_parse_keyword(optarg);
				if (*ysm == Y4M_UNKNOWN) {
					mjpeg_error("Unknown subsampling mode option:  %s", optarg);
					mjpeg_error("Try: 420mpeg2 420jpeg 444 422 411 mono");
					return -1;
				}
				break;
			case 'o':
				*fdOut = open (optarg,O_CREAT|O_WRONLY,0644);
				if (*fdOut == -1) {
					mjpeg_error_exit1 ("Cannot open file for writing");
				}
				break;	
			case 'w':
				*aw=1;
				*sct=AVMEDIA_TYPE_AUDIO;
				break;
			case 'c':
				*con = 1;
				break;
			case 's':
				*str = atoi(optarg);
				break;
			case 'f':
				av = av_find_input_format	(optarg);
				break;
			case 'r':
				*rs = (char *) malloc (strlen(optarg)+1);
				strcpy(*rs,optarg);
				// would like to split into 2 parts to bring inline with EDL version
				*sr=1;
				break;
			case 'v':
				mjpeg_default_handler_verbosity (atoi (optarg));
				break;
				
			case 'e':
				
			case 'h':
			case '?':
				return -1 ;
				break;
		}
	}
	return 0;
	
}

int open_av_file (AVFormatContext **pfc, char *fn, AVInputFormat *avif, int st, int sct,AVCodecContext **pcc, AVCodec **pCodec)
{
	
	int i,avStream=-1;
	
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	
	// Open video file
	
#if LIBAVFORMAT_VERSION_MAJOR  < 53
	if(av_open_input_file(pfc, fn, avif, 0, NULL)!=0)
#else
	if(avformat_open_input(pfc, fn, avif, NULL)!=0)
#endif
		return -1; // Couldn't open file
	
	pFormatCtx = *pfc;
	
	//	fprintf (stderr,"av_find_stream_info\n");
	
	// Retrieve stream information
#if LIBAVFORMAT_VERSION_MAJOR  < 53
	if(av_find_stream_info(pFormatCtx)<0)
#else
	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
#endif
		return -1; // Couldn't find stream information
	
	//	fprintf (stderr,"dump_format\n");
	
	// Dump information about file onto standard error
#if LIBAVFORMAT_VERSION_MAJOR  < 53
	dump_format(pFormatCtx, 0, fn, 0);
#else
	av_dump_format(pFormatCtx, 0, fn, 0);
#endif
	// Find the first video stream
	// not necessarily a video stream but this is legacy code
	for(i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==sct)
		{
			// DEBUG: print out codec
			//			fprintf (stderr,"Video Codec ID: %d (%s)\n",pFormatCtx->streams[i]->codec->codec_id ,pFormatCtx->streams[i]->codec->codec_name);
			if (avStream == -1 && st == 0) {
				// May still be overridden by the -s option
				avStream=i;
			}
			if (st == i) {
				avStream=i;
				break;
			}
		}
	if(avStream==-1) {
		mjpeg_error("open_av_file: could not find an AV stream");
		return -1; // Didn't find a video stream
	}
	
	// Get a pointer to the codec context for the video stream
	*pcc=pFormatCtx->streams[avStream]->codec;
	
	pCodecCtx = *pcc;
	
	// Find the decoder for the video stream
	*pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(*pCodec==NULL) {
		mjpeg_error("open_av_file: could not find codec");
		return -1; // Codec not found
	}
	
	// Open codec
#if LIBAVCODEC_VERSION_MAJOR < 53 
	if(avcodec_open(pCodecCtx, *pCodec)<0) {
#else
	if(avcodec_open2(pCodecCtx, *pCodec,NULL)<0) {
#endif
		mjpeg_error("open_av_file: could not open codec");
		return -1; // Could not open codec
	}
	
	return avStream;
}

int init_video(y4m_ratio_t *yuv_frame_rate, int stream, AVFormatContext *pFormatCtx, y4m_ratio_t *yuv_aspect, 
			   int *convert, int *yuv_ss_mode, int *convert_mode, y4m_stream_info_t *si, AVFrame **pFrame) 
{
	// All video related decoding
	
	AVCodecContext *pCodecCtx=pFormatCtx->streams[stream]->codec;
	
	// Read framerate, aspect ratio and chroma subsampling from Codec
	if (yuv_frame_rate->d == 0) {
		yuv_frame_rate->n = pFormatCtx->streams[stream]->r_frame_rate.num;
		yuv_frame_rate->d = pFormatCtx->streams[stream]->r_frame_rate.den;
	}
	if (yuv_aspect->d == 0) {
		yuv_aspect->n = pCodecCtx-> sample_aspect_ratio.num;
		yuv_aspect->d = pCodecCtx-> sample_aspect_ratio.den;
	}
	
	// 0:0 is an invalid aspect ratio default to 1:1
	if (yuv_aspect->d == 0 || yuv_aspect->n == 0 ) {
		yuv_aspect->n=1;
		yuv_aspect->d=1;
	}
	
	if (*convert) {
		if (*yuv_ss_mode == Y4M_UNKNOWN) {
			mjpeg_warn("init_video: Convert to Unknown Chroma Subsampling mode\n");
			print_usage();
			return -1;	
		} else {
			switch (*yuv_ss_mode) {
				case Y4M_CHROMA_420MPEG2: *convert_mode = PIX_FMT_YUV420P; break;
				case Y4M_CHROMA_422: *convert_mode = PIX_FMT_YUV422P; break;
				case Y4M_CHROMA_444: *convert_mode = PIX_FMT_YUV444P; break;
				case Y4M_CHROMA_411: *convert_mode = PIX_FMT_YUV411P; break;
				case Y4M_CHROMA_420JPEG: *convert_mode = PIX_FMT_YUVJ420P; break;
				case Y4M_CHROMA_MONO: *convert_mode = PIX_FMT_GRAY8; break;
				default:
					mjpeg_error("Cannot convert to this chroma mode");
					return -1;
					break;
			}
			mjpeg_warn("Enabling non standard YUV4MPEG stream");
			y4m_accept_extensions(1);
			
		}
	} else if (*yuv_ss_mode == Y4M_UNKNOWN) {
		switch (pCodecCtx->pix_fmt) {
			case PIX_FMT_YUV420P: *yuv_ss_mode=Y4M_CHROMA_420MPEG2; break;
			case PIX_FMT_YUV422P: *yuv_ss_mode=Y4M_CHROMA_422; break;
			case PIX_FMT_YUV444P: *yuv_ss_mode=Y4M_CHROMA_444; break;
			case PIX_FMT_YUV411P: *yuv_ss_mode=Y4M_CHROMA_411; break;
			case PIX_FMT_YUVJ420P: *yuv_ss_mode=Y4M_CHROMA_420JPEG; break;
			default:
				*yuv_ss_mode=Y4M_CHROMA_444; 
				*convert_mode = PIX_FMT_YUV444P;
				// is there a warning function
				mjpeg_warn("Unsupported Chroma mode (%d). Upsampling to YUV444",pCodecCtx->pix_fmt);
				// enable advanced yuv stream
				mjpeg_info("Enabling non standard YUV4MPEG stream");
				y4m_accept_extensions(1);
				*convert = 1;
				break;
		}
	}
	
	// I will make the assumption that a previous allocation has the same size frame.
	// It's a limitation of the output stream, so putting this limit in the allocation
	// shouldn't matter
	if (*pFrame == NULL) {	
		// Allocate video frame
		*pFrame=avcodec_alloc_frame();
		mjpeg_debug("avmalloc pFrame: %x",*pFrame);
	}
	// Output YUV format details
	// is there some mjpeg_info functions?
	mjpeg_info ("YUV Aspect Ratio: %d:%d",yuv_aspect->n,yuv_aspect->d);
	mjpeg_info ("YUV frame rate: %d:%d",yuv_frame_rate->n,yuv_frame_rate->d);
	mjpeg_info ("YUV Chroma Subsampling: %d",*yuv_ss_mode);
		
	// Set the YUV stream details
	// Interlace is handled when the first frame is read.
	y4m_si_set_sampleaspect(si, *yuv_aspect);
	y4m_si_set_framerate(si, *yuv_frame_rate);
	y4m_si_set_chroma(si, *yuv_ss_mode);
	
	return 0;
}	

// I give up, I cannot work out why av_write_frame is not working
int manual_write_yuv (uint8_t *m[3], y4m_stream_info_t *sinfo) {
	
	int fs,cfs;
	int r=0;
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
	
	printf("FRAME\n");
	
	write(1,m[0],fs);
	write(1,m[1],cfs);
	write(1,m[2],cfs);
	
	return 0;
	
}

int process_video (AVCodecContext  *pCodecCtx, AVFrame *pFrame, AVFrame **pFrame444, AVPacket *packet, uint8_t	**buffer,
				   int *header_written, int *yuv_interlacing, int convert, int convert_mode, y4m_stream_info_t *streaminfo,
				   uint8_t  *yuv_data[3], int fdOut, y4m_frame_info_t *frameinfo, int write, struct SwsContext *img_convert_ctx)
{
	
	int frameFinished,numBytes;
	int write_error_code;
	int yuv_width[3];
	
	//	mjpeg_debug ("decode video");
	
#ifdef HAVE_AVCODEC_DECODE_VIDEO2
	avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, packet);
#else
	avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet->data, packet->size);
#endif
	//
	// Did we get a video frame?
	// frameFinished does not mean decoder finished, means that the packet can be freed.
	
#ifdef DEBUGPROCESSVIDEO
	mjpeg_debug("frameFinished: %d",frameFinished);
#endif
	
	// Save the frame to disk
	
	// As we don't know interlacing until the first frame
	// we wait until the first frame is read before setting the interlace flag
	// and outputting the YUV header
	// It also appears that some codecs don't set width or height until the first frame either
	
	if (!*header_written) {
		if (frameFinished) {
			if (*yuv_interlacing == Y4M_UNKNOWN) {
				if (pFrame->interlaced_frame) {
					if (pFrame->top_field_first) {
						*yuv_interlacing = Y4M_ILACE_TOP_FIRST;
					} else {
						*yuv_interlacing = Y4M_ILACE_BOTTOM_FIRST;
					}
				} else {
					*yuv_interlacing = Y4M_ILACE_NONE;
				}
			}
			
			if (img_convert_ctx) {
				// initialise conversion to different chroma subsampling
			
				if (!*pFrame444) {
					*pFrame444=avcodec_alloc_frame();
					mjpeg_debug("avmalloc pFrame444: %x",*pFrame444);
				}
				numBytes=avpicture_get_size(convert_mode, pCodecCtx->width, pCodecCtx->height);
				

				if (*buffer == NULL) {
					*buffer=(uint8_t *)malloc(numBytes);
					mjpeg_debug("malloc buffer %x",*buffer);
				}
				avpicture_fill((AVPicture *)*pFrame444, *buffer, convert_mode, pCodecCtx->width, pCodecCtx->height);				
			}
			
			y4m_si_set_interlace(streaminfo, *yuv_interlacing);
			y4m_si_set_width(streaminfo, pCodecCtx->width);
			y4m_si_set_height(streaminfo, pCodecCtx->height);
			
#ifdef DEBUGPROCESSVIDEO
			fprintf (stderr,"yuv_data: %x pFrame: %x\nchromalloc\n",yuv_data,pFrame);
#endif					
			
			chromalloc(yuv_data,streaminfo);
			
#ifdef DEBUGPROCESSVIDEO
			fprintf (stderr,"yuv_data: %x pFrame: %x\n",yuv_data,pFrame);
#endif					
			
			mjpeg_info ("YUV interlace: %d",*yuv_interlacing);
			mjpeg_info ("YUV Output Resolution: %dx%d",pCodecCtx->width, pCodecCtx->height);
			
			// Need to work out why this isn't being set earlier
			//	y4m_accept_extensions(1);
			if ((write_error_code = y4m_write_stream_header(fdOut, streaminfo)) != Y4M_OK)
			{
				// should this be fatal?
				// Yes as it will result in a broken yuv4mpeg stream, and may cause downstream filters to crash.
				mjpeg_error_exit1("Write header failed: %s", y4m_strerr(write_error_code));
			} 
			*header_written = 1;
		}
	}
	
	if (*header_written) {
		// if (frameFinished) { 
		// appears that if it's not decoded there isn't anything in the ffmpeg buffer
		// this can cause seg faults.
		
		if (img_convert_ctx) {
			
			//	mjpeg_debug("sws_scale(%x,%x,%lu,%d,%x,%lu).",img_convert_ctx, (*pFrame444)->data, (*pFrame444)->linesize,pCodecCtx->height,pFrame->data, pFrame->linesize);
			//sws_scale(img_convert_ctx, (*pFrame444)->data, (*pFrame444)->linesize, 0, pCodecCtx->height, pFrame->data, pFrame->linesize);
			sws_scale(img_convert_ctx,  pFrame->data, pFrame->linesize, 0, pCodecCtx->height,(*pFrame444)->data, (*pFrame444)->linesize);
			
			//	mjpeg_debug("after sws_scale(). %d \n",convert_mode);
			
			avchromacpy(yuv_data,*pFrame444,streaminfo);
			
			//	manual_write_yuv(yuv_data,streaminfo);
			
			/*
			 yuv_width[0] = y4m_si_get_plane_width(streaminfo,0);
			 yuv_width[1] = y4m_si_get_plane_width(streaminfo,1);
			 yuv_width[2] = y4m_si_get_plane_width(streaminfo,2);
			 
			 sws_scale(img_convert_ctx, yuv_data, yuv_width, 0, pCodecCtx->height, pFrame->data, pFrame->linesize);
			 */
			
		} else {
#ifdef DEBUGPROCESSVIDEO
			fprintf (stderr,"yuv_data: %x pFrame: %x\n",yuv_data,pFrame);
#endif					
			avchromacpy(yuv_data,pFrame,streaminfo);
		}
	} /* frame finished */
	
	if (write && *header_written) {
		
		//		mjpeg_debug("writing yuv data (%d, %x, %x, %x, %d,%d)",fdOut, streaminfo, frameinfo, yuv_data,
		//				y4m_si_get_plane_length(streaminfo,0),y4m_si_get_plane_length(streaminfo,1));
		//		mjpeg_debug("yuvdata: %x %x %x",yuv_data[0],yuv_data[1],yuv_data[2]);
		//		mjpeg_debug("frameinfo : %d %d %d", y4m_fi_get_presentation(frameinfo), y4m_fi_get_temporal(frameinfo), y4m_fi_get_spatial(frameinfo));
		
		// y4m_accept_extensions(1);
		
		mjpeg_debug ("Stream info length: %d chroma: %d",y4m_si_get_framelength(streaminfo),y4m_si_get_chroma(streaminfo));
		
		
		if ((write_error_code = y4m_write_frame(fdOut, streaminfo, frameinfo, yuv_data) != Y4M_OK)) 
			mjpeg_error("Write frame failed: %s", y4m_strerr(write_error_code));
		
	}
	
	if (frameFinished) {
		// I'm getting reports of double frees, not sure if I should free the packet here. 
		// Or am I leaking memory?
		// OK I'm leaking memory. So wtf do I do
		
		/*
		 mjpeg_warn("freeing packet: %x",packet);
		 */
#ifdef HAVE_AV_FREE_PACKET
		//mjpeg_warn ("using av_free_packet");
		av_free_packet(packet);
#else
		//mjpeg_warn ("using av_freep");
		av_freep(packet);
#endif
		 
	}
	else 
		mjpeg_warn ("FRAME NOT FINISHED");
	
	
}	

int main(int argc, char *argv[])
{
    AVFormatContext *pFormatCtx;
	AVInputFormat *avif = NULL;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame = NULL; 
    AVFrame         *pFrame444 = NULL; 
    AVPacket        packet;
    int             numBytes,numSamples;
	int audioWrite = 0,search_codec_type=AVMEDIA_TYPE_VIDEO;
    uint8_t         *buffer = NULL;
	int16_t		*aBuffer = NULL;
	char *tc_in = NULL,*tc_out=NULL;
	
	int i,fdOut = 1 ;
	int yuv_interlacing = Y4M_UNKNOWN;
	int yuv_ss_mode = Y4M_UNKNOWN;
	y4m_ratio_t yuv_frame_rate;
	y4m_ratio_t yuv_aspect;
	// need something for chroma subsampling type.
	;
	int header_written = 0;
	int convert = 0;
	int stream = 0,subRange=0;
	int finishedit = 0;
	enum PixelFormat convert_mode;
	int64_t sampleCounter=0,frameCounter=0,startFrame=0,endFrame=1<<30;
	int samplesFrame;
	int newFile =1;
	char *rangeString = NULL;
	char *openfile;
	int edlfiles,edlcounter;
	struct edlentry *edllist = NULL;
	int y,skip=0;
	int                frame_data_size ;
	uint8_t            *yuv_data[3] ;      
	struct SwsContext *img_convert_ctx =NULL;
	
	y4m_stream_info_t streaminfo;
	y4m_frame_info_t frameinfo;
	
	y4m_init_stream_info(&streaminfo);
	y4m_init_frame_info(&frameinfo);
	
	yuv_frame_rate.d = 0;
	yuv_aspect.d = 0;
	
    // Register all formats and codecs
    av_register_all();
	mjpeg_default_handler_verbosity (0);
	
	
	// Parse commandline arguments
	if (parseCommandline(argc,argv,&yuv_interlacing,&yuv_frame_rate,&yuv_aspect, &yuv_ss_mode,&fdOut,
						 &audioWrite,&search_codec_type,&convert,&stream,avif,&rangeString,&subRange) == -1) {
		print_usage();
		exit (-1);
	}
	
	optind--;
	argc -= optind;
	argv += optind;
	
	if (argc == 1) {
		print_usage (argv);
		return 0 ;
	}
	
	if (rangeString) 
		if (splitTimecode(&tc_in,&tc_out,rangeString)==-1) {
			fprintf (stderr,"Timecode range, incorrect format. Should be:\n\t[[[hh:]mm:]ss:]ff-[[[hh:]mm:]ss:]ff\n\t[[[hh:]mm:]ss;]ff-[[[hh:]mm:]ss;]ff for NTSC drop code\nmm and ss may be 60 or greater if they are the leading digit.\nff maybe FPS or greater if leading digit\n");
			exit -1;
		}
	
	for (;(argc--)>1;argv++) {
		
		openfile = argv[1];
		
		// check if filename is EDL
		
		// fprintf (stderr,"file ext: %s\n",openfile+strlen(openfile)-3);
		
		edlfiles = 1; skip = 0;
		// Should I make EDL file a special case using a command line option.
		// Or do the following.  look for edl at the end.
		// Actually I should also check for string length.
		if (!strcmp(openfile+strlen(openfile)-4,".edl"))
		{
			//	fprintf (stderr,"parsing edl file\n");
			edlfiles = parseEDL(openfile,&edllist);
			//	fprintf (stderr,"EDL struct location: %x containing %d entries\n",edllist,edlfiles);
		}
		// set number of files for loop (1 otherwise)
		// end if
		
		// for loop number of files (1 if not EDL)
		for (edlcounter=0;edlcounter<edlfiles;edlcounter++) {
			
			// if EDL
			if (edllist) {
				// should allow for multiple edits from the one file.
				
				mjpeg_info("running EDL entry: %d %s",edlcounter,edllist[edlcounter].filename);
				fprintf (stderr,"in: %s out: %s",edllist[edlcounter].in, edllist[edlcounter].out);
				
				//fprintf (stderr,"in: %s out: %s audio: %d video: %d\n",edllist[edlcounter].in, edllist[edlcounter].out,edllist[edlcounter].audio, edllist[edlcounter].video);
				
				
				// set editmode (search_codec_type)
				// set in and out points
				tc_in = edllist[edlcounter].in;
				tc_out = edllist[edlcounter].out;
				
				skip = 0;
				if (audioWrite && edllist[edlcounter].audio) {
					search_codec_type = AVMEDIA_TYPE_AUDIO;
				} else if (!audioWrite && edllist[edlcounter].video) {
					search_codec_type = AVMEDIA_TYPE_VIDEO;
				} else {
					// skip if write mode (audio or video) != edit mode
					skip = 1;
				}
				
				if (!skip) {
					newFile =1;
					if (finishedit) {	// this means that we've exited early from an edit
						finishedit=0;
						// check that the file names are the same
						if (!strcmp(openfile,edllist[edlcounter].filename)) {
							
							// check that the start of the new edit is later than the current frame/sample counter
							// this means that we've exited early from an edit
							startFrame = parseTimecodeRE(tc_in,yuv_frame_rate.n,yuv_frame_rate.d);
							endFrame = parseTimecodeRE(tc_out,yuv_frame_rate.n,yuv_frame_rate.d);
							if (startFrame == -1 || endFrame == -1) {
								mjpeg_error_exit1("Timecode range, incorrect format. Should be:\n\t[[[[hh:]mm:]ss:]ff]-[[[[hh:]mm:]ss:]ff]\n\t[[[[hh:]mm:]ss;]ff]-[[[[hh:]mm:]ss;]ff] for NTSC drop code\nmm and ss may be 60 or greater if they are the leading digit.\nff maybe FPS or greater if leading digit\n");
							}
							
							if (startFrame > endFrame) {
								mjpeg_error_exit1("Timecode range, incorrect format. Should be:\n\t[[[[hh:]mm:]ss:]ff]-[[[[hh:]mm:]ss:]ff]\n\t[[[[hh:]mm:]ss;]ff]-[[[[hh:]mm:]ss;]ff] for NTSC drop code\nmm and ss may be 60 or greater if they are the leading digit.\nff maybe FPS or greater if leading digit\n");
							}
							
							
							if (audioWrite) {
								if (sampleCounter < startFrame * samplesFrame)  {
									newFile = 0;
								}
							} else {
								if (frameCounter < startFrame) {
									newFile=0;
								}
							}
						}
					}
				}
				
				
				openfile = edllist[edlcounter].filename;
				
			}
			if (!skip) {
				if (newFile) {
					stream = open_av_file(&pFormatCtx, openfile, avif, stream, search_codec_type, &pCodecCtx, &pCodec);
					if (stream == -1) {
						mjpeg_error("Error with video file: %s",openfile);
					}
					
					// get the frame rate of the first video stream, if cutting audio.
					//		if (audioWrite && rangeString) {
					if (audioWrite && tc_in) {
						if (yuv_frame_rate.d == 0) {
							for(i=0; i<pFormatCtx->nb_streams; i++) {
								if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
								{
									yuv_frame_rate.n = pFormatCtx->streams[i]->r_frame_rate.num;
									yuv_frame_rate.d = pFormatCtx->streams[i]->r_frame_rate.den;
								}
							}
						}
					}
					if (audioWrite==0) {
						if (init_video( &yuv_frame_rate, stream, pFormatCtx, &yuv_aspect, &convert, &yuv_ss_mode, &convert_mode, &streaminfo, &pFrame) == -1) {
							mjpeg_error_exit1("Error initialising video file: %s",openfile);
							exit (-1);
						}
						if (convert) {
							img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
															 pCodecCtx->width, pCodecCtx->height, convert_mode, SWS_BICUBIC, NULL, NULL, NULL); 
						}
					} else {
						numBytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
						if (tc_in) {
							// does this need more precision?
							samplesFrame  = pCodecCtx->sample_rate * yuv_frame_rate.d / yuv_frame_rate.n ;
						}
						if (aBuffer == NULL) {

							aBuffer = (int16_t *) malloc (numBytes);
							mjpeg_debug("malloc aBuffer: %x",aBuffer);

							// allocate for audio
						}
					}
					//	frameCounter++;
					
					// convert cut range into frame numbers.
					// now do I remember how NTSC drop frame works?
					if (tc_in) {
						startFrame = -1; endFrame = -1;
						startFrame = parseTimecodeRE(tc_in,yuv_frame_rate.n,yuv_frame_rate.d);
						endFrame = parseTimecodeRE(tc_out,yuv_frame_rate.n,yuv_frame_rate.d);
						if (startFrame == -1 || endFrame == -1) {
							mjpeg_error_exit1("Timecode range, incorrect format. Should be:\n\t[[[[hh:]mm:]ss:]ff]-[[[[hh:]mm:]ss:]ff]\n\t[[[[hh:]mm:]ss;]ff]-[[[[hh:]mm:]ss;]ff] for NTSC drop code\nmm and ss may be 60 or greater if they are the leading digit.\nff maybe FPS or greater if leading digit\n");
						}
						
						if (startFrame > endFrame) {
							mjpeg_error_exit1("Timecode range, incorrect format. Should be:\n\t[[[[hh:]mm:]ss:]ff]-[[[[hh:]mm:]ss:]ff]\n\t[[[[hh:]mm:]ss;]ff]-[[[[hh:]mm:]ss;]ff] for NTSC drop code\nmm and ss may be 60 or greater if they are the leading digit.\nff maybe FPS or greater if leading digit\n");
						}
						frameCounter = 0; sampleCounter = 0;
					}
				}

#ifdef DEBUG
				if (audioWrite!=0) {
					mjpeg_debug ("sample counter: %lld - %lld  (%lld - %lld) spf %d",startFrame,endFrame,startFrame * samplesFrame,endFrame*samplesFrame,samplesFrame);
				}
#endif	
				
				//	fprintf (stderr,"loop until nothing left (%x:%x)\n",pFormatCtx,&packet);
				// Loop until nothing read
				while(av_read_frame(pFormatCtx, &packet)>=0 && !finishedit)
				{
					
					// fprintf (stderr,"inside loop until nothing left\n");
					
					
					// Is this a packet from the desired stream?
					if(packet.stream_index==stream)
					{
						// Decode video frame
						if (audioWrite==0) {
							
							// fprintf (stderr,"frame counter: %lld  (%lld - %lld)\n",frameCounter,startFrame,endFrame);
							if (frameCounter >= startFrame && frameCounter<= endFrame) {
								
								process_video (pCodecCtx, pFrame, &pFrame444, &packet, &buffer,
											   &header_written, &yuv_interlacing, convert, convert_mode, &streaminfo,
											   yuv_data, fdOut, &frameinfo,1,img_convert_ctx);
								
							} else
							/*
							 {
							 
							 process_video (pCodecCtx, pFrame, &pFrame444, &packet, &buffer,
							 &header_written, &yuv_interlacing, convert, convert_mode, &streaminfo,
							 yuv_data, fdOut, &frameinfo,0);
							 }
							 */
								
								if (frameCounter >= (startFrame-25) && frameCounter< startFrame) {
									
									// need to decode about 1 second before the start but not write until the correct frame.								
									// decode without writing
									process_video (pCodecCtx, pFrame, &pFrame444, &packet, &buffer,
												   &header_written, &yuv_interlacing, convert, convert_mode, &streaminfo,
												   yuv_data, fdOut, &frameinfo,0,img_convert_ctx);
									
								} else if (frameCounter<25) {
									process_video (pCodecCtx, pFrame, &pFrame444, &packet, &buffer,
												   &header_written, &yuv_interlacing, convert, convert_mode, &streaminfo,
												   yuv_data, fdOut, &frameinfo,0,img_convert_ctx);
								}
							
							if (frameCounter > endFrame) {
								finishedit = 1;
								// for some reason when we finish, we skip a frame which is causing syncing problems.
								// so count it here.
								// I would like to determine the cause, but this is the work around.
								frameCounter++;
								
							}
							
							if (header_written) {
								frameCounter++;
							} 
							
						} else {
							// decode Audio
#ifdef HAVE_AVCODEC_DECODE_AUDIO3
							avcodec_decode_audio3(pCodecCtx, aBuffer, &numBytes, &packet);
#else
							avcodec_decode_audio2(pCodecCtx, aBuffer, &numBytes, packet.data, packet.size);
#endif
							//
							
							// TODO: write a wave or aiff file. 
							
							// PANIC: how to determine bytes per sample?
							numSamples = numBytes / BYTES_PER_SAMPLE;
							
							if (!tc_in) {
								write (1, aBuffer, numBytes);
								
								// whole decoded frame within range.
							} else if (sampleCounter >= startFrame * samplesFrame &&
									   sampleCounter+numSamples <= (endFrame+1) * samplesFrame  ) {
								write (1, aBuffer, numBytes);
								
								// start of buffer outside range, end of buffer in range
							} else if (sampleCounter+numSamples >= startFrame * samplesFrame &&
									   sampleCounter+numSamples <= (endFrame+1) * samplesFrame ) {
								// write a subset
								
								write(1,aBuffer+(startFrame-sampleCounter)*BYTES_PER_SAMPLE,numBytes-(startFrame*samplesFrame-sampleCounter)*BYTES_PER_SAMPLE);
								
								// start of buffer in range, end of buffer outside range.
							} else if (sampleCounter >= startFrame * samplesFrame &&
									   sampleCounter <= (endFrame+1) * samplesFrame ) {
								// write a subset
								
								write(1,aBuffer,((endFrame+1)*samplesFrame-sampleCounter)*BYTES_PER_SAMPLE);
								
								// entire range contained within buffer
							} else if (sampleCounter < startFrame * samplesFrame &&
									   sampleCounter+numSamples > (endFrame+1) * samplesFrame ) {
								// write a subset
								
								write(1,aBuffer+(startFrame-sampleCounter)*BYTES_PER_SAMPLE,((endFrame+1)-startFrame)*samplesFrame*BYTES_PER_SAMPLE);
							} 
							sampleCounter += numSamples;
							numBytes  = AVCODEC_MAX_AUDIO_FRAME_SIZE;	
							
							if (sampleCounter > (endFrame+1) * samplesFrame) {
								finishedit = 1;
							}
							
						}
						
						/* else {
						 fprintf (stderr,"SKIPPED COUNTING FRAME...\n");
						 }
						 */
					}
				
#ifdef HAVE_AV_FREE_PACKET
					av_free_packet(&packet);
#else
					av_freep(&packet);
#endif
					
				}
			}
			
			// Free the packet that was allocated by av_read_frame
			//	av_free_packet(&packet);
			avcodec_close(pCodecCtx);
			av_close_input_file(pFormatCtx);

		}
	
	}
	
	if (audioWrite==0) {
		// Free the YUV frame
		mjpeg_debug("Freeing pFrame: %x",pFrame);
		av_free(pFrame);
	} else {
		mjpeg_debug("Freeing aBuffer: %x",aBuffer);
		free (aBuffer);
	}
	
	
	// Close the codec
	//avcodec_close(pCodecCtx);
	
	// Close the video file
	//av_close_input_file(pFormatCtx);
	
	
	if (img_convert_ctx) {
		mjpeg_debug("Freeing img_convert_ctx: %x",img_convert_ctx);
		av_free(img_convert_ctx);
		if (pFrame444 != NULL)
			av_free(pFrame444);
	}
	
	if(buffer) {
		mjpeg_debug("Freeing buffer: %x",buffer);
		free(buffer);
	}
	mjpeg_debug("Freeing av_packet");

#ifdef HAVE_AV_FREE_PACKET
                av_free_packet(&packet);
#else
                av_freep(&packet);
#endif


	// END of file loop

	if (audioWrite == 0) {

		y4m_fini_stream_info(&streaminfo);
		y4m_fini_frame_info(&frameinfo);
		
		mjpeg_debug("Freeing yuv_data: %x,%x,%x",yuv_data[0],yuv_data[1],yuv_data[2]);

		free(yuv_data[0]);
		free(yuv_data[1]);
		free(yuv_data[2]);
		
		mjpeg_info ("%d Frames processed",frameCounter);
	} else {
		mjpeg_info ("%d Samples processed",sampleCounter);
	}
    return 0;
}
