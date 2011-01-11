
CC=gcc
OPT_FLAG=-g
#OPT_FLAG=-O3 -ftree-vectorize
FREETYPEFLAGS=-lfreetype
COCOAFLAGS=-framework QuartzCore -framework Foundation -framework AppKit
CODECFLAGS=-DHAVE_AVCODEC_DECODE_VIDEO2 -DHAVE_AVCODEC_DECODE_AUDIO3 -DHAVE_AV_FREE_PACKET
#CODECFLAGS=-DHAVE_AV_FREE_PACKET
LDFLAGS=-L/usr/local/lib -lmjpegutils -L/opt/local/lib utilyuv.o
CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/opt/local/include -I/usr/local/include -I/opt/local/include/freetype2
FFMPEG_FLAGS= $(CODECFLAGS) -lswscale -lavcodec -lavformat -lavutil
#FFMPEG_FLAGS= $(CODECFLAGS) -lavcodec -lavformat -lavutil


TARGETS=libav-bitrate libav2yuv libavmux yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop \
		yuvdeinterlace yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater yuvbilateral \
		yuvtbilateral yuvCIFilter yuvdiag yuvpixelgraph 


all: $(TARGETS)



yuvtshot: yuvtshot.o utilyuv.o

yuvdiff: yuvdiff.o utilyuv.o

yuvpixelgraph: yuvpixelgraph.o utilyuv.o

yuvbilateral: yuvbilateral.o utilyuv.o

yuvtbilateral: yuvtbilateral.o utilyuv.o

yuvsubtitle: yuvsubtitle.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvsubtitle $<

yuvdiag: yuvdiag.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvdiag $<

yuvCIFilter: yuvCIFilter.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(COCOAFLAGS) -o yuvCIFilter $<

yuvilace: yuvilace.o  utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) -lfftw3 -o yuvilace utilyuv.o $<

libav2yuv: libav2yuv.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav2yuv $<

libav-bitrate: libav-bitrate.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav-bitrate  $<

libavmux: libavmux.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libavmux $<

clean:
	 rm -f *.o $(TARGETS)

install:
	cp $(TARGETS) /usr/local/bin
