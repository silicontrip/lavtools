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
//
// quadrant gcc -O3 -I/sw/include -I/sw/include/mjpegtools -L/sw/lib -lavcodec -lavformat -lavutil -lmjpegutils libav2yuv.c -o libav2yuv 
//
// I really should put history here
// 7th July 2008 - Added Force Format option 
// 4th July 2008 - Added Aspect Ratio Constants
// 3rd July 2008 - Will choose the first stream found if no stream is specified  
// 24th Feb 2008 - Found an unexpected behaviour where frames were being dropped. libav said that no frame was decoded. Have output the previous frame in this instance.
//
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

#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>


#define PAL "PAL"
#define PAL_WIDE "PAL_WIDE"
#define NTSC "NTSC"
#define NTSC_WIDE "NTSC_WIDE"


int64_t parseTimecode (char *tc, int frn,int frd) {
	
	// My only concern here is that some people use approximations for NTSC frame rates.
	
	int h=0,m=0,s=0,f=0;
	char *stc[4];
	int i,cc=0;
	float fps,frameNumber;
	int fn;
	int64_t fn64;
	
	// I'm trying to remember if the ; represents 29.97 fps displayed as if running at 30 fps.
	// So therefore doesn't display the correct time.
	// or if it represents 29.97 rounded to the nearest frame. IE catches up a frame every 1001 frames.
	
	if (strlen(tc) == 0 ) 
		return 0;
	
	// determine ntsc drop timecode format
	
	//	fprintf(stderr,"parser: passing '%s'\n",tc);
	
	if (strlen(tc) > 2) {
		if ( 1.0 * frn / frd == 30000.0 / 1001.0) {
			
			// or is this a : ?
			if (tc[strlen(tc)-3] == ';') {
				fprintf (stderr,"parser: NTSC Drop Code\n");
				frn = 30;
				frd = 1;
				tc[strlen(tc)-3] == ':';
			}
		}
	}
	
	fps = 1.0 * frn / frd;
	
	for (i=strlen(tc)-1; i>=0; i--) 
	{
		
		if ( tc[i] == ':') {
			if (cc > 4) { 
				// too many : 
				fprintf (stderr,"parse error: too many ':' in timecode\n");
				return -1;
			}
			stc[cc++] = tc+i + 1;
			tc[i]='\0';
		} else if (tc[i]<'0' || tc[i]>'9') {
			// illegal character
			fprintf (stderr,"parse error: illegal character in timecode\n");
			return -1;
		}
	}
	if (cc > 4) { 
		// too many : 
		fprintf (stderr,"parse error: too many ':' in timecode\n");
		return -1;
	}
	stc[cc++] = tc;
	
	// debug
#ifdef DEBUG
	fprintf (stderr,"parser: (%d): ",cc);
	
	for (i=0; i < cc; i++) 
		fprintf (stderr,"%s, ",stc[i]);
	
	fprintf (stderr,"\n");
#endif	

	f = atoi(stc[0]);
	if (cc>1) 
		s = atoi(stc[1]);
	if (cc>2) 
		m = atoi(stc[2]);
	if (cc>3)
		h = atoi(stc[3]);
	
	//	fprintf (stderr,"parser: atoi %d %d %d %d\n",h,m,s,f);
	
	// validate time
	
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

int parseTimecodeRange(int64_t *s, int64_t *e, char *rs, int frn,int frd) {
	
	
	int dashcount = 0;
	int dashplace = 0;
	char *re;
	int i;
	int64_t ls,le;
	
	for (i=0; i<strlen(rs); i++) {
		if (rs[i] == '-') {
			dashcount ++;
			dashplace = i;
		}
	}
	
	if (dashcount == 1) {
		
		re = rs + dashplace + 1;
		rs[dashplace] = '\0';
		
		ls = parseTimecode(rs,frn,frd);
		le = parseTimecode(re,frn,frd);
		
		//		fprintf (stderr,"parser: frame range: %lld - %lld\n",ls,le);
		
		if (le < ls) {
			fprintf (stderr,"End before start\n");
			return -1;
		}
		
		if (le==-1 || ls == -1)
			return -1;
		
		*e = le;
		*s = ls;
		
	} else {
		return -1;
	}
	
	return 0;
	
}

void chromacpy (uint8_t *dst[3], AVFrame *src, y4m_stream_info_t *sinfo)
{
	
	int y,h,w;
	int cw,ch;
	
	w = y4m_si_get_plane_width(sinfo,0);
	h = y4m_si_get_plane_height(sinfo,0);
	cw = y4m_si_get_plane_width(sinfo,1);
	ch = y4m_si_get_plane_height(sinfo,1);
	
	for (y=0; y<h; y++) {
#ifdef DEBUG
	fprintf (stderr,"copy %d bytes to: %x from: %x\n",w,dst[0]+y*w,(src->data[0])+y*src->linesize[0]);
#endif
		
		memcpy(dst[0]+y*w,(src->data[0])+y*src->linesize[0],w);
		if (y<ch) {
#ifdef DEBUG
	fprintf (stderr,"copy %d bytes to: %x from: %x\n",cw,dst[1]+y*cw,(src->data[1])+y*src->linesize[1]);
#endif

			memcpy(dst[1]+y*cw,(src->data[1])+y*src->linesize[1],cw);
			memcpy(dst[2]+y*cw,(src->data[2])+y*src->linesize[2],cw);
		}
	}
	
}

void chromalloc(uint8_t *m[3],y4m_stream_info_t *sinfo)
{
	
	int fs,cfs;
	
	fs = y4m_si_get_plane_length(sinfo,0);
	cfs = y4m_si_get_plane_length(sinfo,1);
	
#ifdef DEBUG
	fprintf (stderr,"Allocatting: %d and %d bytes\n",fs,cfs);
#endif
	
	m[0] = (uint8_t *)malloc( fs );
	m[1] = (uint8_t *)malloc( cfs);
	m[2] = (uint8_t *)malloc( cfs);
	
}

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
			 "\t -h print this help\n"
			 );
}


int main(int argc, char *argv[])
{
    AVFormatContext *pFormatCtx;
	AVInputFormat *avif = NULL;
    int             i, avStream;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame; 
    AVFrame         *pFrame444; 
    AVPacket        packet;
    int             frameFinished;
    int             numBytes;
	int audioWrite = 0,search_codec_type=CODEC_TYPE_VIDEO;
    uint8_t         *buffer;
	int16_t		*aBuffer;
	
	int fdOut = 1 ;
	int yuv_interlacing = Y4M_UNKNOWN;
	int yuv_ss_mode = Y4M_UNKNOWN;
	y4m_ratio_t yuv_frame_rate;
	y4m_ratio_t yuv_aspect;
	// need something for chroma subsampling type.
	int write_error_code;
	int header_written = 0;
	int convert = 0;
	int stream = 0,subRange=0;
	enum PixelFormat convert_mode;
	int64_t frameCounter=0,startFrame=0,endFrame=1<<30;
	char *rangeString = NULL;
	
	const static char *legal_flags = "wchI:F:A:S:o:s:f:r:";
	
	int y;
	int                frame_data_size ;
	uint8_t            *yuv_data[3] ;      
	
	y4m_stream_info_t streaminfo;
	y4m_frame_info_t frameinfo;
	
	y4m_init_stream_info(&streaminfo);
	y4m_init_frame_info(&frameinfo);
	
	yuv_frame_rate.d = 0;
	yuv_aspect.d = 0;
	
    // Register all formats and codecs
    av_register_all();
	
	while ((i = getopt (argc, argv, legal_flags)) != -1) {
		switch (i) {
			case 'I':
				switch (optarg[0]) {
					case 'p':  yuv_interlacing = Y4M_ILACE_NONE;  break;
					case 't':  yuv_interlacing = Y4M_ILACE_TOP_FIRST;  break;
					case 'b':  yuv_interlacing = Y4M_ILACE_BOTTOM_FIRST;  break;
					default:
						mjpeg_error("Unknown value for interlace: '%c'", optarg[0]);
						return -1;
						break;
				}
				break;
			case 'F':
				if( Y4M_OK != y4m_parse_ratio(&yuv_frame_rate, optarg) )
					mjpeg_error_exit1 ("Syntax for frame rate should be Numerator:Denominator");
				
                break;
				case 'A':
				if( Y4M_OK != y4m_parse_ratio(&yuv_aspect, optarg) ) {
					if (!strcmp(optarg,PAL)) {
						y4m_parse_ratio(&yuv_aspect, "128:117");
					} else if (!strcmp(optarg,PAL_WIDE)) {
						y4m_parse_ratio(&yuv_aspect, "640:351");
					} else if (!strcmp(optarg,NTSC)) {
						y4m_parse_ratio(&yuv_aspect, "4320:4739");
					} else if (!strcmp(optarg,NTSC_WIDE)) {
						y4m_parse_ratio(&yuv_aspect, "5760:4739");
					} else {
						mjpeg_error_exit1 ("Syntax for aspect ratio should be Numerator:Denominator");
					}
				}
				break;
				case 'S':
				yuv_ss_mode = y4m_chroma_parse_keyword(optarg);
				if (yuv_ss_mode == Y4M_UNKNOWN) {
					mjpeg_error("Unknown subsampling mode option:  %s", optarg);
					mjpeg_error("Try: 420mpeg2 444 422 411");
					return -1;
				}
				break;
				case 'o':
				fdOut = open (optarg,O_CREAT|O_WRONLY,0644);
				if (fdOut == -1) {
					mjpeg_error_exit1 ("Cannot open file for writing");
				}
				break;	
				case 'w':
				audioWrite=1;
				search_codec_type=CODEC_TYPE_AUDIO;
				break;
				case 'c':
				convert = 1;
				break;
				case 's':
				stream = atoi(optarg);
				break;
				case 'f':
				avif = av_find_input_format	(optarg);
				break;
				case 'r':
				rangeString = (char *) malloc (strlen(optarg)+1);
				strcpy(rangeString,optarg);
				subRange=1;
				break;
				case 'h':
				case '?':
				print_usage (argv);
				return 0 ;
				break;
		}
	}
	
	//fprintf (stderr,"optind: %d\n",optind);
	optind--;
	argc -= optind;
	argv += optind;
	
	if (argc == 1) {
		print_usage (argv);
		return 0 ;
	}
	
    // Open video file
    if(av_open_input_file(&pFormatCtx, argv[1], avif, 0, NULL)!=0)
        return -1; // Couldn't open file
	
    // Retrieve stream information
    if(av_find_stream_info(pFormatCtx)<0)
        return -1; // Couldn't find stream information
	
    // Dump information about file onto standard error
    dump_format(pFormatCtx, 0, argv[1], 0);
	
    // Find the first video stream
	// not necessarily a video stream but this is legacy code
    avStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==search_codec_type)
        {
			// mark debug
			//fprintf (stderr,"Video Codec ID: %d (%s)\n",pFormatCtx->streams[i]->codec->codec_id ,pFormatCtx->streams[i]->codec->codec_name);
			if (avStream == -1 && stream == 0) {
				// May still be overridden by the -s option
				avStream=i;
			}
			if (stream == i) {
				avStream=i;
				break;
			}
        }
    if(avStream==-1) {
		fprintf (stderr,"Couldn't find Audio or Video stream\n");
        return -1; // Didn't find a video stream
	}
	
	
    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[avStream]->codec;
	
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
        return -1; // Codec not found
	
    // Open codec
    if(avcodec_open(pCodecCtx, pCodec)<0)
        return -1; // Could not open codec
	
	// get the frame rate of the first video stream, if cutting.
	if (audioWrite && rangeString) {
	    for(i=0; i<pFormatCtx->nb_streams; i++) {
			if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
			{
				if (yuv_frame_rate.d == 0) {
					yuv_frame_rate.n = pFormatCtx->streams[i]->r_frame_rate.num;
					yuv_frame_rate.d = pFormatCtx->streams[i]->r_frame_rate.den;
				}
			}
		}
	}
	if (audioWrite==0) {
		
		// All video related decoding
		
		// Read framerate, aspect ratio and chroma subsampling from Codec
		if (yuv_frame_rate.d == 0) {
			yuv_frame_rate.n = pFormatCtx->streams[avStream]->r_frame_rate.num;
			yuv_frame_rate.d = pFormatCtx->streams[avStream]->r_frame_rate.den;
		}
		if (yuv_aspect.d == 0) {
			yuv_aspect.n = pCodecCtx-> sample_aspect_ratio.num;
			yuv_aspect.d = pCodecCtx-> sample_aspect_ratio.den;
		}
		
		// 0:0 is an invalid aspect ratio default to 1:1
		if (yuv_aspect.d == 0 || yuv_aspect.n == 0 ) {
			yuv_aspect.n=1;
			yuv_aspect.d=1;
		}
		if (convert) {
	        if (yuv_ss_mode == Y4M_UNKNOWN) {
				print_usage();
				return 0;	
			} else {
				y4m_accept_extensions(1);
				switch (yuv_ss_mode) {
					case Y4M_CHROMA_420MPEG2: convert_mode = PIX_FMT_YUV420P; break;
					case Y4M_CHROMA_422: convert_mode = PIX_FMT_YUV422P; break;
					case Y4M_CHROMA_444: convert_mode = PIX_FMT_YUV444P; break;
					case Y4M_CHROMA_411: convert_mode = PIX_FMT_YUV411P; break;
					case Y4M_CHROMA_420JPEG: convert_mode = PIX_FMT_YUVJ420P; break;
					default:
						mjpeg_error_exit1("Cannot convert to this chroma mode");
						break;
						
				}
			}
		} else if (yuv_ss_mode == Y4M_UNKNOWN) {
			switch (pCodecCtx->pix_fmt) {
				case PIX_FMT_YUV420P: yuv_ss_mode=Y4M_CHROMA_420MPEG2; break;
				case PIX_FMT_YUV422P: yuv_ss_mode=Y4M_CHROMA_422; break;
				case PIX_FMT_YUV444P: yuv_ss_mode=Y4M_CHROMA_444; break;
				case PIX_FMT_YUV411P: yuv_ss_mode=Y4M_CHROMA_411; break;
				case PIX_FMT_YUVJ420P: yuv_ss_mode=Y4M_CHROMA_420JPEG; break;
				default:
					yuv_ss_mode=Y4M_CHROMA_444; 
					convert_mode = PIX_FMT_YUV444P;
					// is there a warning function
					mjpeg_error("Unsupported Chroma mode. Upsampling to YUV444\n");
					// enable advanced yuv stream
					y4m_accept_extensions(1);
					convert = 1;
					break;
			}
		}
		
		
		// Allocate video frame
		pFrame=avcodec_alloc_frame();
		
		// Output YUV format details
		// is there some mjpeg_info functions?
		fprintf (stderr,"YUV Aspect Ratio: %d:%d\n",yuv_aspect.n,yuv_aspect.d);
		fprintf (stderr,"YUV frame rate: %d:%d\n",yuv_frame_rate.n,yuv_frame_rate.d);
		fprintf (stderr,"YUV Chroma Subsampling: %d\n",yuv_ss_mode);
		
		// Set the YUV stream details
		// Interlace is handled when the first frame is read.
		y4m_si_set_sampleaspect(&streaminfo, yuv_aspect);
		y4m_si_set_framerate(&streaminfo, yuv_frame_rate);
		y4m_si_set_chroma(&streaminfo, yuv_ss_mode);
	} else {
		numBytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
		aBuffer = (int16_t *) malloc (numBytes);
		// allocate for audio
		
	}
	
			// convert cut range into frame numbers.
		// now do I remember how NTSC drop frame works?
		if (rangeString) {
			
			if (parseTimecodeRange(&startFrame,&endFrame,rangeString,yuv_frame_rate.n,yuv_frame_rate.d)) {
				fprintf (stderr,"Timecode range, incorrect format. Should be:\n\t[[[[hh:]mm:]ss:]ff]-[[[[hh:]mm:]ss:]ff]\n\t[[[[hh:]mm:]ss;]ff]-[[[[hh:]mm:]ss;]ff] for NTSC drop code\nmm and ss may be 60 or greater if they are the leading digit.\nff maybe FPS or greater if leading digit\n");
				return -1;
			}
		}

	
	//fprintf (stderr,"loop until nothing left\n");
	// Loop until nothing read
    while(av_read_frame(pFormatCtx, &packet)>=0 && frameCounter<= endFrame)
    {
        // Is this a packet from the desired stream?
        if(packet.stream_index==avStream)
        {
            // Decode video frame
	#ifdef DEBUG
		fprintf (stderr,"frame counter: %lld  (%lld - %lld)\n",frameCounter,startFrame,endFrame);
	#endif
			if (frameCounter >= startFrame && frameCounter<= endFrame) {
				if (audioWrite==0) {
			#ifdef DEBUG
					fprintf (stderr,"decode video\n");
			#endif
					avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, 
										 packet.data, packet.size);  
					// Did we get a video frame?
					// frameFinished does not mean decoder finished, means that the packet can be freed.
					#ifdef DEBUG
					fprintf (stderr,"frameFinished: %d\n",frameFinished);
					#endif
					if(frameFinished)
					{
					// Save the frame to disk
					
					// As we don't know interlacing until the first frame
					// we wait until the first frame is read before setting the interlace flag
					// and outputting the YUV header
					// It also appears that some codecs don't set width or height until the first frame either
					if (!header_written) {
						if (yuv_interlacing == Y4M_UNKNOWN) {
							if (pFrame->interlaced_frame) {
								if (pFrame->top_field_first) {
									yuv_interlacing = Y4M_ILACE_TOP_FIRST;
								} else {
									yuv_interlacing = Y4M_ILACE_BOTTOM_FIRST;
								}
							} else {
								yuv_interlacing = Y4M_ILACE_NONE;
							}
						}
						if (convert) {
							// initialise conversion to different chroma subsampling
							pFrame444=avcodec_alloc_frame();
							numBytes=avpicture_get_size(convert_mode, pCodecCtx->width, pCodecCtx->height);
							buffer=(uint8_t *)malloc(numBytes);
							avpicture_fill((AVPicture *)pFrame444, buffer, convert_mode, pCodecCtx->width, pCodecCtx->height);
						}
						
						y4m_si_set_interlace(&streaminfo, yuv_interlacing);
						y4m_si_set_width(&streaminfo, pCodecCtx->width);
						y4m_si_set_height(&streaminfo, pCodecCtx->height);
						
#ifdef DEBUG
						fprintf (stderr,"yuv_data: %x pFrame: %x\nchromalloc\n",yuv_data,pFrame);
#endif					
						chromalloc(yuv_data,&streaminfo);
#ifdef DEBUG
						fprintf (stderr,"yuv_data: %x pFrame: %x\n",yuv_data,pFrame);
#endif					
						
						fprintf (stderr,"YUV interlace: %d\n",yuv_interlacing);
						fprintf (stderr,"YUV Output Resolution: %dx%d\n",pCodecCtx->width, pCodecCtx->height);
						
						if ((write_error_code = y4m_write_stream_header(fdOut, &streaminfo)) != Y4M_OK)
						{
							mjpeg_error("Write header failed: %s", y4m_strerr(write_error_code));
						} 
						header_written = 1;
					}
					
					if (convert) {
						// convert to 444
						img_convert((AVPicture *)pFrame444, convert_mode, (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
						chromacpy(yuv_data,pFrame444,&streaminfo);
					} else {
#ifdef DEBUG
						fprintf (stderr,"yuv_data: %x pFrame: %x\n",yuv_data,pFrame);
#endif					
						chromacpy(yuv_data,pFrame,&streaminfo);
					}
					write_error_code = y4m_write_frame( fdOut, &streaminfo, &frameinfo, yuv_data);
					  } /* frame finished */
					
				} else {
					// decode Audio
					avcodec_decode_audio2(pCodecCtx, 
										  aBuffer, &numBytes,
										  packet.data, packet.size);
					
					// TODO: write a wave or aiff file. 
					
					write (1, aBuffer, numBytes);
					numBytes  = AVCODEC_MAX_AUDIO_FRAME_SIZE;	
					
				}
			}
			frameCounter++;

		}
		
        // Free the packet that was allocated by av_read_frame
		if (frameFinished)
			av_free_packet(&packet);
    }
	
	if (audioWrite==0) {
		y4m_fini_stream_info(&streaminfo);
		y4m_fini_frame_info(&frameinfo);
		
		free(yuv_data[0]);
		free(yuv_data[1]);
		free(yuv_data[2]);
		
		// Free the YUV frame
		av_free(pFrame);
	} else {
		free (aBuffer);
	}
    // Close the codec
    avcodec_close(pCodecCtx);
	
    // Close the video file
    av_close_input_file(pFormatCtx);
	
    return 0;
}
