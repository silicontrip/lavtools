/* libav-bitrate
 * adapted from
 * avcodec_sample.0.4.9.cpp
 
 ** <h3>Bitrate graph producer</h3>
 ** <p>Produces an ASCII file suitable for plotting in gnuplot of the
 ** bitrate per frame.  Also includes a rolling average algorithm that
 ** helps smooth out differences between I frame, P frame and B frames.
 ** Constant bitrate is not so constant.  Requires about 50 frames
 ** rolling average window to get an idea of the average bitrate.</p>
 **
 ** <p> using a post processing tool such as octave to average the graph 
 ** may be useful</p>
 **
 
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
 
 */

/* this program is a purely libav implementation, no requirement for mjpegutils */
//#include <yuv4mpeg.h>
//#include <mpegconsts.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

void progress_init(off_t b, off_t t);
void progress_loadBar(off_t bytes);

static void print_usage() 
{
	fprintf (stderr,
			 "usage: libav-bitrate [-s <smoothing window length>] [-e] [-i <output interval>] [-I <output interval>] <filename>\n"
			 "\t -s <window length> smoothing window length in frames (min 1)\n"
			 "\t -e print to stderr also\n"
			 "\t -i <output interval> defaults to one frame. (min 1)\n"
			 "\t -I <output interval> in seconds. Overrides -i. (larger than 0)\n"
			 "\t -P print progress bar.\n"
			 "produces a text bandwidth graph for any media file recognised by libav\n"
			 "\n"
			 );
}

struct settings {
	int ave_len;
	char output_stderr;
	char output_progress;
	int output_interval;
	double 	 output_interval_seconds;
	
};

void free_streams () {
	
}

int main(int argc, char *argv[])
{
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame; 
    AVPacket        packet;
    int             frameFinished;
	
	int *stream_size=NULL;
	int *stream_max=NULL;
	int *stream_min=NULL;
	double *stream_ave=NULL;
	int total_max=0, total_min=INT32_MAX;
	double total_ave=0;
	int numberStreams;
	int frame_counter=0;
	int total_size=0;
	int tave=0;
	
	struct settings programSettings;
	struct stat fileStat;
	off_t total_file_size;
	double framerate;
	
	// default settings
	programSettings.ave_len=1;
	programSettings.output_stderr=0;
	programSettings.output_interval=1;
	programSettings.output_interval_seconds=0;
	programSettings.output_progress=0;

	
	// parse commandline options
	const static char *legal_flags = "s:i:I:ePh";
	
	int c;
	char *error=NULL;
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 's':
				// been programming in java too much recently
				// I want to catch a number format exception here
				// And tell the user of their error.
				programSettings.ave_len=(int)strtol(optarg, &error, 10);
				if (*error || programSettings.ave_len < 1) {
					fprintf(stderr,"Smoothing value is invalid\n");
					print_usage();
					return -1;
				}
				
				break;
			case 'e':
				programSettings.output_stderr=1;
				break;
			case 'P':
				programSettings.output_progress=1;
				break;

			case 'i':
				programSettings.output_interval=(int)strtol(optarg, &error, 10);
				if (*error || programSettings.output_interval<1) {
					fprintf(stderr,"Interval is invalid\n");
					print_usage();
					
					return -1;
				}
				break;
			case 'I':
				programSettings.output_interval_seconds=strtod(optarg, &error);
				
				if (*error || programSettings.output_interval_seconds <= 0) {
					fprintf(stderr,"Interval Seconds is invalid\n");
					print_usage();
					
					return -1;
				}
				
				break;
			case 'h':
			case '*':
				print_usage();
				return 0;
				break;
		}
	}
	
	
	optind--;
	// argc -= optind;
	argv += optind;
	
	//fprintf (stderr, "optind = %d. Trying file: %s\n",optind,argv[1]);
	
    // Register all formats and codecs
    av_register_all();
	
	if (argv[1] == NULL) 
	{
		fprintf(stderr,"Error: No filename.\n");
		print_usage();
		return -1; // Couldn't open file
	}
	
	
	if (programSettings.output_progress) {
		stat(argv[1], &fileStat);
		// check for return error
		progress_init(0,fileStat.st_size);
	}
	
	
    // Open video file
#if LIBAVFORMAT_VERSION_MAJOR  < 53
    if(av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL)!=0)
#else
		if(avformat_open_input(&pFormatCtx, argv[1], NULL,  NULL)!=0)
#endif
		{
			fprintf(stderr,"Error: could not open file.\n");
			return -1; // Couldn't open file
		}
	
    // Retrieve stream information
#if LIBAVFORMAT_VERSION_MAJOR  < 53
	if(av_find_stream_info(pFormatCtx)<0)
#else
		if(avformat_find_stream_info(pFormatCtx,NULL)<0)
#endif
		{
			fprintf(stderr,"Error: could not interpret file.\n");
			return -1; // Couldn't find stream information
		}
	
    // Dump information about file onto standard error
#if LIBAVFORMAT_VERSION_MAJOR < 53
	dump_format(pFormatCtx, 0, argv[1], 0);
#else
	av_dump_format(pFormatCtx, 0, argv[1], 0);
#endif
	
	// As this program outputs based on video frames.
    // Find the first video stream
	// To determine the bitrate. 
    videoStream=-1;
	
	numberStreams = pFormatCtx->nb_streams;
	
	stream_size = (int *)malloc(numberStreams * sizeof(int));
	stream_min = (int *)malloc(numberStreams * sizeof(int));
	stream_max = (int *)malloc(numberStreams * sizeof(int));
	stream_ave = (double *)malloc(numberStreams * sizeof(double));
	
	
	for(i=0; i<numberStreams; i++) {
		//	fprintf (stderr,"stream: %d = %d (%s)\n",i,pFormatCtx->streams[i]->codec->codec_type ,pFormatCtx->streams[i]->codec->codec_name);
		// Initialise statistic counters
		stream_size[i] = 0;
		stream_min[i]=INT32_MAX;
		stream_max[i]=0;
		stream_ave[i] = 0;
		
#if LIBAVFORMAT_VERSION_MAJOR < 53
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
#else		
			if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
#endif
			{
				videoStream=i;
				framerate = pFormatCtx->streams[i]->r_frame_rate.num;
				if (pFormatCtx->streams[i]->r_frame_rate.den != 0)
					framerate /= pFormatCtx->streams[i]->r_frame_rate.den;
				//	fprintf (stderr,"Video Stream: %d Frame Rate: %d:%d\n",videoStream,pFormatCtx->streams[i]->r_frame_rate.num,pFormatCtx->streams[i]->r_frame_rate.den);
				
			}
	}
	
    if(videoStream==-1) {
		free (stream_size); free (stream_min); free (stream_max); free (stream_ave);
        return -1; // Didn't find a video stream
	}
	
    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;
	
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
		free (stream_size); free (stream_min); free (stream_max); free (stream_ave);
        return -1; // Codec not found
	}
	
	if (framerate == 0) 
	{
		//fprintf(stderr,"frame rate %d:%d\n",pCodecCtx->time_base.num,pCodecCtx->time_base.den);
		framerate = pCodecCtx->time_base.den;
		if (pCodecCtx->time_base.den != 0)
			framerate /= pCodecCtx->time_base.num;
	}
	
	
	
	if (programSettings.output_interval_seconds >0)
    {
		if (INT32_MAX / framerate > programSettings.output_interval_seconds)
		{
			programSettings.output_interval = programSettings.output_interval_seconds * framerate;
		} else {
			fprintf(stderr,"Interval seconds too large\n");
			free (stream_size); free (stream_min); free (stream_max); free (stream_ave);
			return -1; 
		}
	}
	//	fprintf (stderr,"Video Stream: %d Frame Rate: %g\n",videoStream,framerate);
	
	
    // Open codec
#if LIBAVCODEC_VERSION_MAJOR < 52 
	if(avcodec_open(pCodecCtx, *pCodec)<0) 
#else
		if(avcodec_open2(pCodecCtx, pCodec,NULL)<0) 
#endif
		{
			free (stream_size); free (stream_min); free (stream_max); free (stream_ave);
			return -1; // Could not open codec
		}
	
    // Allocate video frame
    pFrame=avcodec_alloc_frame();
	
	int counter_interval=0;
	
	
	total_file_size=0;
	// Loop until nothing read
    while(av_read_frame(pFormatCtx, &packet)>=0)
    {
		stream_size[packet.stream_index] += packet.size;
		
		if (programSettings.output_progress) {
			total_file_size += packet.size;
			progress_loadBar(total_file_size);
		}
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream)
        {
            // Decode video frame
			// I'm not entirely sure when avcodec_decode_video was deprecated.  most likely earlier than 53
#if LIBAVCODEC_VERSION_MAJOR < 52 
            avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet.data, packet.size);
#else
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
#endif
			
			if (counter_interval++ >= programSettings.output_interval) {
				
				//if (!(frame_counter % ave_len)) {
				// print the statistics in gnuplot friendly format...
				total_size=0;
				for(i=0; i<numberStreams; i++) { total_size += stream_size[i]; }
				// if (tave == -1) { tave = total_size; }
				
				tave = ((tave * (programSettings.ave_len-1)) + total_size) / programSettings.ave_len / programSettings.output_interval;
				
				if(total_min > total_size) 
					total_min = total_size;
				
				if(total_max < total_size) 
					total_max = total_size;
				
				total_ave += total_size;
				
				
				printf ("%f ",frame_counter/framerate);
				printf ("%f ",tave*8*framerate);
				
				for(i=0; i<numberStreams; i++) {
					
					// double rate = stream_size[i]*8*framerate/ programSettings.output_interval;
					
					if(stream_min[i] > stream_size[i]) 
						stream_min[i] = stream_size[i];
					
					if(stream_max[i] < stream_size[i]) 
						stream_max[i] = stream_size[i];
					
					stream_ave[i] += stream_size[i];
					
					
					printf ("%f ",stream_size[i]*8*framerate/ programSettings.output_interval);
					
					stream_size[i]=0;
				}
				printf("\n");
				
				//}
				counter_interval = 1;
            }
			frame_counter++;
			
        }
		
        // Free the packet that was allocated by av_read_frame
#if LIBAVCODEC_VERSION_MAJOR < 52 
		av_freep(&packet);
#else
		av_free_packet(&packet);
#endif
    }
	
	free(stream_size);
	
	
    // Free the YUV frame
    av_free(pFrame);
	
    // Close the codec
	avcodec_close(pCodecCtx);
	
    // Close the video file
#if LIBAVCODEC_VERSION_MAJOR < 53
    av_close_input_file(pFormatCtx);
#else
	avformat_close_input(&pFormatCtx);
#endif
	
	// Print statistics
	
	if (programSettings.output_stderr) 
	{
		fprintf(stderr,"%20s %20s %20s %20s\n","Stream","Min Bitrate","Average bitrate","Max bitrate");
		
		fprintf(stderr,"%20s %20f %20f %20f\n","Total",total_min*8*framerate/ programSettings.output_interval,
				total_ave * 8*framerate/ programSettings.output_interval/(frame_counter/programSettings.output_interval),
				total_max*8*framerate/ programSettings.output_interval);
		for(i=0; i<numberStreams; i++) {
			fprintf(stderr,"%20d %20f %20f %20f\n",i,stream_min[i]*8*framerate/ programSettings.output_interval,
					stream_ave[i] *8*framerate/ programSettings.output_interval/(frame_counter/programSettings.output_interval),
					stream_max[i]*8*framerate/ programSettings.output_interval);
		}
	}
	free (stream_min); free (stream_max); free (stream_ave);
	
    return 0;
}
