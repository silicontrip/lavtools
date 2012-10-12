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
//gcc -O3 -I/usr/local/include/ffmpeg -I/usr/local/include/mjpegtools -lavcodec -lavformat -lavutil -lmjpegutils libav2yuv.c -o libav2yuv
//

//#include <yuv4mpeg.h>
//#include <mpegconsts.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>


static void print_usage() 
{
  fprintf (stderr,
           "usage: libav-bitrate <filename> [<smoothing window length>]\n"
           "produces a text bandwidth graph for any media file recognised by libav\n"
           "\n"
         );
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

	int *stream_size;
	int frame_counter=0;
	int total_size;
	int ave_len=1;
	int tave=0;

	double framerate;

    // Register all formats and codecs
    av_register_all();

	if (argc == 1) {
          print_usage (argv);
          return 0 ;
	}

	if (argc == 3) {
		ave_len=atoi(argv[2]);
	}

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

    if(videoStream==-1)
        return -1; // Didn't find a video stream


    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
        return -1; // Codec not found

	if (framerate == 0) 
	{
	
		fprintf(stderr,"frame rate %d:%d\n",pCodecCtx->time_base.num,pCodecCtx->time_base.den);
		framerate = pCodecCtx->time_base.den;
		if (pCodecCtx->time_base.den != 0)
			framerate /= pCodecCtx->time_base.num;

	}
	
	fprintf (stderr,"Video Stream: %d Frame Rate: %g\n",videoStream,framerate);

	
    // Open codec
#if LIBAVCODEC_VERSION_MAJOR < 52 
	if(avcodec_open(pCodecCtx, *pCodec)<0) 
#else
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0) 
#endif
			return -1; // Could not open codec


    // Allocate video frame
    pFrame=avcodec_alloc_frame();

	// Loop until nothing read
    while(av_read_frame(pFormatCtx, &packet)>=0)
    {

	//printf("pts: %ld ",packet.pts);
	//printf("dts: %ld ",packet.dts);
	//printf("size: %d ",packet.size);
	//printf("id: %d ",packet.stream_index);
	//printf("dur: %d ",packet.duration);
	//printf("pos: %d\n",packet.pos);

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
					tave = ((tave * (ave_len-1)) + total_size) / ave_len;
					printf ("%g ",frame_counter/framerate);
					printf ("%g ",tave*8*framerate);
					for(i=0; i<pFormatCtx->nb_streams; i++) {
						printf ("%g ",stream_size[i]*8*framerate);
						stream_size[i]=0;
					}
					printf("\n");
				//}
				frame_counter++;
            
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }


    // Free the YUV frame
    av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

	free(stream_size);
	
    // Close the video file
#if LIBAVCODEC_VERSION_MAJOR < 53
    av_close_input_file(pFormatCtx);
#else
	avformat_close_input(&pFormatCtx);
#endif
	
    return 0;
}
