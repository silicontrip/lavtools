
CC=gcc
OPT_FLAG=-g
#OPT_FLAG=-O3 -ftree-vectorize
FREETYPEFLAGS=-L/usr/X11/lib -lfreetype
COCOAFLAGS=-framework QuartzCore -framework Foundation -framework AppKit
CODECFLAGS=
#CODECFLAGS=-DHAVE_AV_FREE_PACKET
LDFLAGS=-L/usr/X11/lib  -L/usr/local/lib -lmjpegutils
CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/usr/local/include  
FFMPEG_FLAGS= $(CODECFLAGS) -lswscale -lavcodec -lavformat -lavutil
#FFMPEG_FLAGS= $(CODECFLAGS) -lavcodec -lavformat -lavutil
JPEGFLAGS= -ljpeg


TARGETS=libav-bitrate libav2yuv libavmux yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop \
		yuvdeinterlace yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater yuvbilateral \
		yuvtbilateral yuvCIFilter yuvdiag yuvpixelgraph yuvfieldrev yuvtemporal yuvtout \
		yuvyadif yuvnlmeans yuvvalues


all: $(TARGETS)

yuvTHREADED: utilyuv.o yuvTHREADED.o

yuvdeinterlace: utilyuv.o yuvdeinterlace.o

yuvtshot: yuvtshot.o utilyuv.o

yuvdiff: yuvdiff.o utilyuv.o

yuvfieldrev: yuvfieldrev.o utilyuv.o 

yuvpixelgraph: yuvpixelgraph.o utilyuv.o

yuvbilateral: yuvbilateral.o utilyuv.o

yuvtbilateral: yuvtbilateral.o utilyuv.o

yuvtemporal: yuvtemporal.o utilyuv.o

yuvtout: yuvtout.o utilyuv.o

yuvyadif: yuvyadif.o utilyuv.o

yuvnlmeans: yuvnlmeans.o utilyuv.o

yuvvalues: yuvvalues.o utilyuv.o

yuv2jpeg: yuv2jpeg.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(JPEGFLAGS) -o yuv2jpeg utilyuv.o $<

yuvsubtitle: yuvsubtitle.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvsubtitle utilyuv.o $<

yuvdiag: yuvdiag.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvdiag utilyuv.o $<

yuvCIFilter: yuvCIFilter.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(COCOAFLAGS) -o yuvCIFilter $<

yuvilace: yuvilace.o  utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) -lfftw3 -o yuvilace utilyuv.o $<

libav2yuv.o: libav2yuv.c
	gcc $(FFMPEG_FLAGS) $(CFLAGS) -c -o libav2yuv.o $<

libav2yuv: libav2yuv.o utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav2yuv utilyuv.o $<

libav-bitrate: libav-bitrate.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav-bitrate  $<

libavmux: libavmux.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libavmux $<

clean:
	 rm -f *.o $(TARGETS)

install:
	cp $(TARGETS) /usr/local/bin
