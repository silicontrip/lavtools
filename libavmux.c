// libavmux
// adapted from
// avcodec_sample.0.4.9.cpp

// A small sample program that shows how to use libavformat and libavcodec to
// read video from a file.
//
// This version is for the 0.4.9-pre1 release of ffmpeg. This release adds the
// av_read_frame() API call, which simplifies the reading of video frames 
// considerably. 
//
//gcc -O3 -I/usr/local/include/ffmpeg -I/sw/include/mjpegtools -L/sw/lib -lavcodec -lavformat -lavutil -lmjpegutils libavmux.c -o libavmux
//

#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


static void print_usage() 
{
  fprintf (stderr,
           "usage: libavmux <filename1> [<filenameN> -o <output file>] [-F <format>] [-h]\n"
           "Muxes or demuxes video files to/from elementary streams\n"
		   "Single input file will demux into streams\n"
		   "Multiple files mux into output file specified by -o\n"
		   "-F specifies the container format (avi, mov, asf, mpegps...)\n"
           "\n"
         );
}

/* libav demuxer
** expects an array of open files, one per stream
*/


int mux_files (char *outname, char **inname, int len, char *format) {

	int i;
	AVFormatContext **pFormatCtx;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *audio_st, *video_st;
	AVPacket packet;
	double audio_pts, video_pts;
	int audio,video;
	int read_bytes=0;
	unsigned int audio_bytes=0;
	unsigned int video_bytes=0;
	


	if (format) {
		fmt = guess_format(format, NULL, NULL);
	} else {
		fmt = guess_format(NULL, outname, NULL);
	}
	
	if (!fmt) {
        fprintf(stderr, "Could not find suitable output format\n");
		return -1;
    }


	oc = av_alloc_format_context();
    if (!oc) {
        fprintf(stderr, "couldn't allocate memory for output format\n");
		return -1;
    }
    oc->oformat = fmt;
    snprintf(oc->filename, sizeof(oc->filename), "%s", outname);

	//fprintf (stderr,"list len: %d\n",len);
	pFormatCtx = (AVFormatContext **) malloc (sizeof(AVFormatContext *) * len);


// TODO: Need to re order the streams so that the video is added first
// otherwise crash ensues...

	for (i =1; i < len; i++) {
		//fprintf (stderr,"file %d: %s\n",i,inname[i]);
		if(av_open_input_file(&pFormatCtx[i], inname[i], NULL, 0, NULL)!=0)
			return -1;
		if(av_find_stream_info(pFormatCtx[i])<0)
			return -1; // Couldn't find stream information

		
		dump_format(pFormatCtx[i], i, inname[i], 0);
		
		if(pFormatCtx[i]->streams[0]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			
			fprintf (stderr,"Video FR: %d/%d\n",pFormatCtx[i]->streams[0]->codec ->time_base.num , pFormatCtx[i]->streams[0]->codec ->time_base.den);

			
			video_st = av_new_stream(oc,0);

			
			if (!video_st) {
				fprintf(stderr, "Could not alloc stream\n");
				return -1;
			}
			video = i;
			video_st->codec = pFormatCtx[i]->streams[0]->codec;
			video_st->codec->time_base.num = 1; // pFormatCtx[i]->streams[0]->codec ->time_base.num;
			video_st->codec->time_base.den = 25; // pFormatCtx[i]->streams[0]->codec ->time_base.den;
	//		video_st->codec->codec_id = pFormatCtx[i]->streams[0]->codec->codec_id;
			
			fprintf (stderr,"Video codec FR %d/%d\n", video_st->codec->time_base.num , video_st->codec->time_base.den);

			
			
		//	if(!strcmp(oc->oformat->name, "mp4") || !strcmp(oc->oformat->name, "mov") || !strcmp(oc->oformat->name, "3gp"))
			if(oc->oformat->flags & AVFMT_GLOBALHEADER)
				video_st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			// fprintf (stderr,"video codec: %s\n",oc->streams[0]->codec->codec->name);
			fprintf (stderr,"video format: %s\n",pFormatCtx[i]->iformat->name);
			
			

		} else if (pFormatCtx[i]->streams[0]->codec->codec_type==CODEC_TYPE_AUDIO) {
			audio_st = av_new_stream(oc,1);			
		//	audio_st = add_audio_stream(oc,pFormatCtx[i]->streams[0]->codec);

			if (!audio_st) {
				fprintf(stderr, "Could not alloc stream\n");
				return -1;
			}
			audio=i;
			audio_st->codec = pFormatCtx[i]->streams[0]->codec;
			
			// fprintf (stderr,"audio codec: %s\n",oc->streams[1]->codec->codec->name);
						fprintf (stderr,"audio format: %s\n",pFormatCtx[i]->iformat->name);

		}
	}
	
	// Need to set correct frame rate
	
	if (av_set_parameters(oc, NULL) < 0) {
        fprintf(stderr, "Invalid output format parameters\n");
        return -1;
    }


	dump_format(oc, 0, outname, 1);
	
	if (!(fmt->flags & AVFMT_NOFILE)) {
        if (url_fopen(&oc->pb, outname, URL_WRONLY) < 0) {
            fprintf(stderr, "Could not open '%s'\n", outname);
			return -1;
        }
    }

	
fprintf (stderr,"Video file: %d Audio File: %d\n",video,audio);

	
	av_write_header(oc);

//fprintf (stderr,"while...\n");

/* need to re-engineer this part of the muxing code */
    while(read_bytes>=0) {
        /* compute current audio and video time */
        if (audio_st)
            audio_pts = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
        else
            audio_pts = 0.0;

        if (video_st)
            video_pts = (double)video_st->pts.val * video_st->codec->time_base.num / video_st->codec->time_base.den;
        else
            video_pts = 0.0;

/*
        if ((!audio_st || audio_pts >= STREAM_DURATION) &&
            (!video_st || video_pts >= STREAM_DURATION))
            break;
*/

	//	fprintf (stderr,"audio or video...\n");

//fprintf (stderr,"PTS A: %d  V: %d \n",audio_st->pts.val,video_st->pts.val);


        /* write interleaved audio and video frames */
        if (!video_st || (video_st && audio_st && audio_pts < video_pts)) {
           // write_audio_frame(oc, audio_st);

			//fprintf (stderr,"read audio...\n");

			read_bytes=av_read_frame(pFormatCtx[audio], &packet);
				audio_bytes += packet.size;
				packet.stream_index=1;
		//	fprintf (stderr,"OUT A: PTS %d DTS %d\n",packet.pts,packet.dts);

			//	packet.pts = audio_st->pts.val;
			//	packet.dts = audio_st->cur_dts;
				
				av_interleaved_write_frame(oc, &packet);

//				av_write_frame(oc, &packet);
				av_free_packet(&packet);
			
        } else {
           // write_video_frame(oc, video_st);


			//fprintf (stderr,"read video...\n");

		read_bytes=av_read_frame(pFormatCtx[video], &packet);
			
				video_bytes+=packet.size;
			
				packet.stream_index=0;
	//		fprintf (stderr,"OUT V: PTS %d DTS %d\n",packet.pts,packet.dts);

			//	packet.pts = video_st->pts.val;
			//	packet.dts = video_st->cur_dts;

			//fprintf (stderr,"write video...\n");

			// a
			
//			packet.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);

			
				//av_write_frame(oc, &packet);
				av_interleaved_write_frame(oc, &packet);

				av_free_packet(&packet);

        }
		
    }

	// post calculate the bitrate...

//fprintf (stderr,"PTS A: %g %u V: %g %u\n",audio_pts,audio_bytes,video_pts,video_bytes);

	av_write_trailer(oc);
    for(i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* close the output file */
        url_fclose(oc->pb);
    }


	av_free(oc);

    return 0;


}

int demux_file (char *filen)
{

	AVFormatContext *pFormatCtx;
	int *stream_fh;
	char *tname;
	AVPacket packet;
	int i;
	
    // Open video file
    if(av_open_input_file(&pFormatCtx, filen, NULL, 0, NULL)!=0)
        return -1; // Couldn't open file

    // Retrieve stream information
    if(av_find_stream_info(pFormatCtx)<0)
        return -1; // Couldn't find stream information

    // Dump information about file onto standard error
    dump_format(pFormatCtx, 0, filen, 0);

	stream_fh = (int *)malloc(pFormatCtx->nb_streams * sizeof(int));
	tname = (char *) malloc(strlen(filen)+10); // 10 characters should be enough???

	printf ("Number streams: %d\n",pFormatCtx->nb_streams);

	for(i=0; i<pFormatCtx->nb_streams; i++) {
		//fprintf (stderr,"stream: %d = %d (%s)\n",i,pFormatCtx->streams[i]->codec->codec_type ,pFormatCtx->streams[i]->codec->codec_name);
		sprintf (tname,"%s.%d",filen,pFormatCtx->streams[i]->codec->codec_id); // buffer overflow might occur. Please help!
		stream_fh[i] = open (tname,O_WRONLY|O_CREAT);
		fchmod(stream_fh[i], 0644);
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			printf ("Stream %d video %d\n",i,pFormatCtx->streams[i]->codec->codec_id);
		} else {
			printf ("Stream %d audio %d\n",i,pFormatCtx->streams[i]->codec->codec_id);
		}

	}

	free(tname);

	// Loop until nothing read
    while(av_read_frame(pFormatCtx, &packet)>=0)
    {
	
        // Is this a packet from the video stream?
		write(stream_fh[packet.stream_index],packet.data,packet.size);
       
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }


	free(stream_fh);

    // Close the video file
    av_close_input_file(pFormatCtx);

    return 0;
}



#define MODE_DEMUX 0
#define MODE_MUX 1

int main(int argc, char *argv[])
{

    int i;   
	int mode = MODE_DEMUX;
	char *containername = NULL;
	char *outname;

	// Parse Args
	
	const static char *legal_flags = "F:o:h?";

	while ((i = getopt (argc, argv, legal_flags)) != -1) {
		fprintf (stderr,"checking opt: %c\n",i);
		switch (i) {
			case 'o':
				outname = (char *) malloc (strlen(optarg)+1);
				strcpy(outname,optarg);
				mode = MODE_MUX;
				break;
			case 'F':
				containername = (char *) malloc (strlen(optarg)+1);
				strcpy(containername,optarg);
				break;
			case 'h':
			case '?':
			default:
				print_usage (argv);
				return 0 ;
				break;
		}
	}
	
	argc -= --optind;
	argv += optind;

	
	if ((argc != 2 && mode==MODE_DEMUX) || (argc<2 && mode==MODE_MUX)) {
		fprintf (stderr,"argc = %d mode = %d\n",argc,mode);

          print_usage (argv);
          return 0 ;
	}

    // Register all formats and codecs
    av_register_all();

	if (mode == MODE_DEMUX) {
		demux_file(argv[1]);
	}
	
	if (mode == MODE_MUX) {
		mux_files(outname, argv, argc, containername);
	}
	
}

