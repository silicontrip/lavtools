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


static void print_usage() 
{
	fprintf (stderr,
			 "usage: libav-bitrate [-s <smoothing window length>] [-e] [-i <output interval>] [-I <output interval>] <filename>\n"
			 "\t -s <window length> smoothing window length in frames\n"
			 "\t -e print to stderr also\n"
			 "\t -i <output interval> defaults to one frame.\n"
			 "\t -I <output interval> in seconds. Overrides -i.\n"
			 "produces a text bandwidth graph for any media file recognised by libav\n"
			 "\n"
			 );
}

struct settings {
	int ave_len;
	char output_stderr;
	int output_interval;
	double 	 output_interval_seconds;

};

int main(int argc, char *argv[])
{
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame; 
    AVPacket        packet;
    int             frameFinished;
	
	int *stream_size;
	int frame_counter=0;
	int total_size;
	int tave=0;

	struct settings programSettings;
	
	
	double framerate;
	
	// default settings
	programSettings.ave_len=1;
	programSettings.output_stderr=0;
	programSettings.output_interval=1;
	programSettings.output_interval_seconds=0;

	const static char *legal_flags = "s:i:I:eh";

	int c;
	
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 's':
				// been programming in java too much recently
				// I want to catch a number format exception here
				// And tell the user of their error.
				programSettings.ave_len=atoi(optarg);
				break;
			case 'e':
				programSettings.output_stderr=1;
				break;
			case 'i':
				programSettings.output_interval=atoi(optarg);
				break;
			case 'I':
				programSettings.output_interval_seconds=atof(optarg);
				break;
			case 'h':
			case '*':
				print_usage(argv);
				return 0;
				break;
		}
	}
	
	
	optind--;
	argc -= optind;
	argv += optind;
	
	//fprintf (stderr, "optind = %d. Trying file: %s\n",optind,argv[1]);
	
    // Register all formats and codecs
    av_register_all();
		
    // Open video file
#if LIBAVFORMAT_VERSION_MAJOR  < 53
    if(av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL)!=0)
#else
		if(avformat_open_input(&pFormatCtx, argv[1], NULL,  NULL)!=0)
#endif
			return -1; // Couldn't open file
	
    // Retrieve stream information
#if LIBAVFORMAT_VERSION_MAJOR  < 53
	if(av_find_stream_info(pFormatCtx)<0)
#else
		if(avformat_find_stream_info(pFormatCtx,NULL)<0)
#endif
			return -1; // Couldn't find stream information
	
    // Dump information about file onto standard error
#if LIBAVFORMAT_VERSION_MAJOR < 53
	dump_format(pFormatCtx, 0, argv[1], 0);
#else
	av_dump_format(pFormatCtx, 0, argv[1], 0);
#endif
	
    // Find the first video stream
    videoStream=-1;
	
	stream_size = (int *)malloc(pFormatCtx->nb_streams * sizeof(int));
	
	for(i=0; i<pFormatCtx->nb_streams; i++) {
		//	fprintf (stderr,"stream: %d = %d (%s)\n",i,pFormatCtx->streams[i]->codec->codec_type ,pFormatCtx->streams[i]->codec->codec_name);
		stream_size[i] = 0;
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
		free (stream_size);
        return -1; // Didn't find a video stream
	}
	
    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;
	
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
		free (stream_size);
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
		programSettings.output_interval = programSettings.output_interval_seconds * framerate;

	
//	fprintf (stderr,"Video Stream: %d Frame Rate: %g\n",videoStream,framerate);
	
	
    // Open codec
#if LIBAVCODEC_VERSION_MAJOR < 52 
	if(avcodec_open(pCodecCtx, *pCodec)<0) 
#else
		if(avcodec_open2(pCodecCtx, pCodec,NULL)<0) 
#endif
		{
			free (stream_size);
			return -1; // Could not open codec
		}
	
    // Allocate video frame
    pFrame=avcodec_alloc_frame();
	
	int counter_interval=0;
	
	// Loop until nothing read
    while(av_read_frame(pFormatCtx, &packet)>=0)
    {
		stream_size[packet.stream_index] += packet.size;
		
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
			
			//if (!(frame_counter % ave_len)) {
			// print the statistics in gnuplot friendly format...
			total_size = 0;
			for(i=0; i<pFormatCtx->nb_streams; i++) {
				total_size += stream_size[i];
			}
			// if (tave == -1) { tave = total_size; }
			tave = ((tave * (programSettings.ave_len-1)) + total_size) / programSettings.ave_len / programSettings.output_interval;
			
			if (counter_interval++ >= programSettings.output_interval) {
				printf ("%f ",frame_counter/framerate);
				if (programSettings.output_stderr==1) 
					fprintf (stderr,"%f ",frame_counter/framerate);

				printf ("%f ",tave*8*framerate);
				if (programSettings.output_stderr==1) 
					fprintf (stderr,"%f ",tave*8*framerate);

				for(i=0; i<pFormatCtx->nb_streams; i++) {
					printf ("%f ",stream_size[i]*8*framerate/ programSettings.output_interval);
					if (programSettings.output_stderr==1) 
						fprintf (stderr,"%f ",stream_size[i]*8*framerate/ programSettings.output_interval);

					stream_size[i]=0;
				}
				printf("\n");

				if (programSettings.output_stderr==1) 
					fprintf(stderr,"\n");
			//}
				counter_interval = 1;
            }
			frame_counter++;

        }
		
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
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
	
    return 0;
}
