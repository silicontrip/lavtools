/* libav-cc
 * adapted from
 * avcodec_sample.0.4.9.cpp

 * extract subtitles in OP-47 format from smpte 436 ancillary data in MXF media files

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
#include "hamming.h"

#if LIBAVFORMAT_VERSION_MAJOR >= 55
#include <libavutil/frame.h>
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

uint8_t page_buffer[24][40];

uint16_t LATIN_G0[96] = 
{ // Latin G0 Primary Set
                0x0020, 0x0021, 0x0022, 0x00a3, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
                0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
                0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
                0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x00ab, 0x00bd, 0x00bb, 0x005e, 0x0023,
                0x002d, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
                0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x00bc, 0x00a6, 0x00be, 0x00f7, 0x007f
        };

struct __attribute__ ((__packed__)) vanc_header {
	uint16_t vanc_linenumber;
	uint8_t interlace_type;
	uint8_t payload_format;
	uint16_t payload_size;
	uint32_t pad;
	uint32_t footer;
};	

struct vbi { uint8_t line_number :5 ; uint8_t reserved: 2; uint8_t field: 1; };

struct __attribute__ ((__packed__)) udw {
	uint8_t did;
	uint8_t sdid;
	uint8_t cdp_size;
	uint16_t id;
	uint8_t length;
	uint8_t format;
	struct vbi vbi_packet[5];
	uint16_t run_in_code;
	uint8_t framing_code;
	uint8_t mrag_address[2];
	uint8_t data[40];
	uint8_t footer;	
	uint16_t fsc;
	uint8_t sdp_checksum;	
};

// ETS 300 706, chapter 8.2
uint8_t unham_8_4(uint8_t a)
{
        uint8_t r = UNHAM_8_4[a];
        if (r == 0xff) {
                printf( "- Unrecoverable data error; UNHAM8/4(%02x)\n", a);
        }
        return (r & 0x0f);
}

// ETS 300 706, chapter 8.3
uint32_t unham_24_18(uint32_t a)
{
        uint8_t test = 0;
	uint8_t i;

        //Tests A-F correspond to bits 0-6 respectively in 'test'.
        for (i = 0; i < 23; i++) test ^= ((a >> i) & 0x01) * (i + 33);
        //Only parity bit is tested for bit 24
        test ^= ((a >> 23) & 0x01) * 32;

        if ((test & 0x1f) != 0x1f)
        {
                //Not all tests A-E correct
                if ((test & 0x20) == 0x20)
                {
                        //F correct: Double error
                        return 0xffffffff;
                }
                //Test F incorrect: Single error
                a ^= 1 << (30 - test);
        }

        return (a & 0x000004) >> 2 | (a & 0x000070) >> 3 | (a & 0x007f00) >> 4 | (a & 0x7f0000) >> 5;
}

uint16_t telx_to_ucs2(uint8_t c)
{
        if (PARITY_8[c] == 0)
        {
                printf ("- Unrecoverable data error; PARITY(%02x)\n", c);
                return 0x20;
        }

        uint16_t r = c & 0x7f;
        if (r >= 0x20)
                r = LATIN_G0[r - 0x20];
        return r;
}

void hexDump (void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

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
void telxDump (void *addr) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;
	int len = 40;

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
	buff[i % 16] = telx_to_ucs2(pc[i]);
        if ((buff[i % 16] < 0x20) || (buff[i % 16] > 0x7e))
            buff[i % 16] = '.';
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

	uint16_t page_number;
	uint8_t charset;

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
	uint8_t new_frame = 0;
	while(av_read_frame(pFormatCtx, &packet)>=0)
        {
		if (packet.stream_index == selStream) 
		{
			
			uint16_t  packets; 
			uint16_t offset = 2;
			memcpy(&packets,packet.data,2);
			packets = BIGEND2(packets);
			/*
			printf("index: %d",packet.stream_index);
			printf(" pts: %lld",packet.pts);
			printf(" size: %d\n",packet.size);
			hexDump(packet.data,188);
			printf ("ANC packets: %d\n", packets);
			*/
			while (packets > 0) 
			{
				vanc = (struct vanc_header *)(packet.data+offset);
				/*
				printf ("PACKETS Remain: %d\n",packets);
				printf ("line number: %d\n", BIGEND2(vanc->vanc_linenumber));
				printf ("interlace: %d\n", vanc->interlace_type);
				printf ("Payload config: %d\n", vanc->payload_format);
				printf ("Payload size: %d\n", BIGEND2(vanc->payload_size));
				printf ("Pad size: %d\n", BIGEND4(vanc->pad));
				printf ("footer: %d\n", BIGEND4(vanc->footer));
				*/

				offset += sizeof(struct vanc_header);
				struct udw *udw_packet;
				udw_packet = (struct udw *) packet.data + offset ;
				/*
				printf ("DID: %d\n", udw_packet->did);
				printf ("SDID: %d\n", udw_packet->sdid);
				printf ("CDP size: %d\n", udw_packet->cdp_size);
				*/
				
				//hexDump(NULL,udw_packet,BIGEND4(vanc->pad));
				if (udw_packet->did == 67 && udw_packet->sdid==2) // Look for OP-47 header
				{
					//memcpy (udw_packet,vanc+19,vanc->cdp_size);
					/*
					printf ("  ID: %x\n" , udw_packet->id);
					printf ("  length: %d\n" , udw_packet->length);
					printf ("  format: %d\n" , udw_packet->format);
					printf ("  VBI1: %d\n" , udw_packet->vbi_packet[0].line_number);
					printf ("  VBI2: %d\n" , udw_packet->vbi_packet[1].line_number);
					printf ("  VBI3: %d\n" , udw_packet->vbi_packet[2].line_number);
					printf ("  VBI4: %d\n" , udw_packet->vbi_packet[3].line_number);
					printf ("  VBI5: %d\n" , udw_packet->vbi_packet[4].line_number);
					printf ("  run in: %d\n" , udw_packet->run_in_code);
					printf ("  framing: %d\n" , udw_packet->framing_code);
					*/

					uint16_t address = (unham_8_4(udw_packet->mrag_address[1]) << 4) | unham_8_4(udw_packet->mrag_address[0]);
					uint8_t m = address & 0x7;
					if (m == 0) m = 8;
					uint16_t y = (address >> 3) & 0x1f;

					/*
					printf ("  mrag: %x%x\n" , udw_packet->mrag_address[0],udw_packet->mrag_address[1]);
					printf ("    magazine: %d\n", m);
					printf ("    packet: %d\n", y);
					printf ("  footer: %d\n" , udw_packet->footer);
					printf ("  FSC: %d\n" , udw_packet->fsc);
					printf ("  chksum: %d\n" , udw_packet->sdp_checksum);
					*/

					if (y == 0)
					{
						new_frame = 1;
						uint8_t i = (unham_8_4(udw_packet->data[1]) << 4) | unham_8_4(udw_packet->data[0]);
						uint8_t flag_subtitle = (unham_8_4(udw_packet->data[5]) & 0x08) >> 3;
						page_number = (m << 8) | (unham_8_4(udw_packet->data[1]) << 4) | unham_8_4(udw_packet->data[0]);
						charset =  ((unham_8_4(udw_packet->data[7]) & 0x08) | (unham_8_4(udw_packet->data[7]) & 0x04) | (unham_8_4(udw_packet->data[7]) & 0x02)) >> 1;
						
						/*
						printf ("    i: %u\n",i);
						printf ("    sub flag: %u\n",flag_subtitle);
						printf ("    page: %u\n",page_number);
						printf ("    charset: %u\n",charset);
						*/
						
						uint8_t yt;
						uint8_t it;

						for(yt = 1; yt <= 23; ++yt)
						{
							for(it = 0; it < 40; it++)
							{
								if (page_buffer[yt][it] != 0x00)
									page_buffer[yt][it] = telx_to_ucs2(page_buffer[yt][it]);
							}
						}


					} else {
						if (new_frame == 1) {
							printf ("--- FRAME ---\n");
							new_frame = 0;
						}
						printf ("Line: %d\n",y);
						telxDump(udw_packet->data);
					}
						
						
		
				}
				packets --;
				offset += BIGEND4(vanc->pad);
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
