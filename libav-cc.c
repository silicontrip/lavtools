/* libav-bitrate
 * adapted from
 * avcodec_sample.0.4.9.cpp

 ** <h3>Bitrate graph producer</h3>
 ** <p>Produces an ASCII file suitable for plotting in gnuplot of the
 ** bitrate per frame. Also includes a rolling average algorithm that
 ** helps smooth out differences between I frame, P frame and B frames.
 ** Constant bitrate is not so constant. Requires about 50 frames
 ** rolling average window to get an idea of the average bitrate.</p>
 **
 ** <p> using a post processing tool such as octave to average the graph
 ** may be useful</p>
 **

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *

 */


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "byteswap.h"

#if LIBAVFORMAT_VERSION_MAJOR >= 55
#include <libavutil/frame.h>
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


struct vanc_header {
	uint16_t packet_count;
	uint16_t vanc_linenumber;
	uint8_t interlace_type;
	uint8_t payload_format;
	uint16_t payload_size;
	uint32_t pad;
	uint32_t footer;
	uint8_t did;
	uint8_t sdid;
	uint8_t cdp_size;
};	

struct udw {
	uint16_t id;
	uint8_t length;
	uint8_t format;
	uint8_t vbi_packet[5];
	uint16_t run_in_code;
	uint8_t framing_code;
	uint16_t mrag_address;
	uint8_t data[40];
	uint8_t footer;	
	uint16_t psc;
	uint8_t sdp_checksum;	
	
};

void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

int main(int argc, char *argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	int i, selStream;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
	AVPacket packet;
	int frameFinished;

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
	int last_type=0;
	int gop_count=0;

	struct stat fileStat;
	off_t total_file_size;
	double framerate;
	struct vanc_header *vanc;

	// parse commandline options
	const static char *legal_flags = "h";

	int c;
	char *error=NULL;
	while ((c = getopt (argc, argv, legal_flags)) != -1) {
		switch (c) {
			case 'h':
			case '*':
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
		return -1; // Couldn't open file
	}


	// Open video file
#if LIBAVFORMAT_VERSION_MAJOR < 53
	if(av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL)!=0)
#else
	if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
#endif
	{
			fprintf(stderr,"Error: could not open file.\n");
			return -1; // Couldn't open file
	}

	// Retrieve stream information
#if LIBAVFORMAT_VERSION_MAJOR < 53
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

	numberStreams = pFormatCtx->nb_streams;

	for(i=0; i<numberStreams; i++) {
		printf ("stream: %d = %d (%d)\n",i,pFormatCtx->streams[i]->codecpar->codec_type ,pFormatCtx->streams[i]->codecpar->codec_id);
		if (pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_DATA) {
			selStream= i;
		}
	}
	printf("Selected Stream: %d\n",selStream);
	while(av_read_frame(pFormatCtx, &packet)>=0)
        {
		if (packet.stream_index == selStream) 
		{
			printf("index: %d",packet.stream_index);
			printf(" pts: %lld",packet.pts);
			printf(" size: %d\n",packet.size);
			vanc = packet.data;
			printf ("ANC packets: %d\n", LILEND2(vanc->packet_count));
			printf ("line number: %d\n", LILEND2(vanc->vanc_linenumber));
			printf ("interlace: %d\n", vanc->interlace_type);
			printf ("Payload config: %d\n", vanc->payload_format);
			printf ("Payload size: %d\n", LILEND2(vanc->payload_size));
			printf ("Pad: %x\n", vanc->pad);
			printf ("Pad: %x\n", vanc->footer);
			printf ("DID: %d\n", vanc->did);
			printf ("SDID: %d\n", vanc->sdid);
			printf ("CDP size: %d\n", vanc->cdp_size);
			printf ("size: %lx\n", (void *)(vanc+19));
			//hexDump(NULL,vanc,vanc->cdp_size);
			//hexDump(NULL,(uint8_t*)vanc+19,vanc->cdp_size);
			//hexDump(NULL,(&vanc->data),vanc->cdp_size);
			if (vanc->did == 67 && vanc->sdid==2) // Look for OP-47 header
			{
				struct udw *udw_packet;
				udw_packet = (uint8_t *)vanc+19;
				//memcpy (udw_packet,vanc+19,vanc->cdp_size);
				printf ("ID: %x\n" , udw_packet->id);
				printf ("length: %d\n" , udw_packet->length);
				printf ("format: %d\n" , udw_packet->format);
				printf ("VBI1: %d\n" , udw_packet->vbi_packet[0]);
				printf ("VBI2: %d\n" , udw_packet->vbi_packet[1]);
				printf ("VBI3: %d\n" , udw_packet->vbi_packet[2]);
				printf ("VBI4: %d\n" , udw_packet->vbi_packet[3]);
				printf ("VBI5: %d\n" , udw_packet->vbi_packet[4]);
				printf ("run in: %d\n" , udw_packet->run_in_code);
				printf ("framing: %d\n" , udw_packet->framing_code);
				printf ("mrag: %d\n" , udw_packet->mrag_address);
				printf ("footer: %d\n" , udw_packet->footer);
				printf ("psc: %d\n" , udw_packet->psc);
				printf ("chksum: %d\n" , udw_packet->sdp_checksum);
				hexDump(NULL,udw_packet->data,40);
		
			}

		}
	}

	// Close the video file
#if LIBAVCODEC_VERSION_MAJOR < 53
	av_close_input_file(pFormatCtx);
#else
	avformat_close_input(&pFormatCtx);
#endif

	return 0;
}
