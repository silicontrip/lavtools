
CC ?= gcc
OPT_FLAG=-g 
#OPT_FLAG=-O3 -ftree-vectorize
FREETYPEFLAGS=-L/usr/X11/lib -lfreetype
COCOAFLAGS=-framework QuartzCore -framework Foundation -framework AppKit
CODECFLAGS=
#CODECFLAGS=-DHAVE_AV_FREE_PACKET
LDFLAGS=-L/usr/X11/lib  -L/usr/local/lib -L/opt/local/lib
#LDFLAGS=-L/usr/local/lib -lmjpegutils -L/opt/local/lib 
CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/usr/local/include  -I/usr/X11/include -I/usr/X11/include/freetype2 -I/opt/local/include
#CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/opt/local/include -I/usr/local/include -I/opt/local/include/freetype2
FFMPEG_FLAGS= $(CODECFLAGS) -lswscale -lavcodec -lavformat -lavutil
#FFMPEG_FLAGS= $(CODECFLAGS) -lavcodec -lavformat -lavutil
JPEGFLAGS= -ljpeg
MJPEGFLAGS= -lmjpegutils utilyuv.o


TARGETS=libav-bitrate libav2yuv libavmux yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop \
		yuvdeinterlace yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater yuvbilateral \
		yuvtbilateral yuvCIFilter yuvdiag yuvpixelgraph yuvfieldrev yuvtemporal yuvtout \
		yuvyadif yuvnlmeans yuvvalues


all: $(TARGETS)

yuvadjust: utilyuv.o yuvadjust.o

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
	$(CC) $(LDFLAGS) $(CFLAGS) $(MJPEGFLAGS) $(JPEGFLAGS) -o yuv2jpeg $<

yuvsubtitle: yuvsubtitle.o utilyuv.o
	$(CC) $(LDFLAGS) $(CFLAGS) $(MJPEGFLAGS) $(FREETYPEFLAGS) -o yuvsubtitle  $<

yuvdiag: yuvdiag.o utilyuv.o
	$(CC)  $(LDFLAGS) $(MJPEGFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvdiag  $<

yuvCIFilter: yuvCIFilter.o utilyuv.o
	$(CC) $(LDFLAGS) $(CFLAGS) $(MJPEGFLAGS) $(COCOAFLAGS) -o yuvCIFilter $<

yuvilace: yuvilace.o  utilyuv.o
	$(CC) $(LDFLAGS) $(CFLAGS) $(MJPEGFLAGS) -lfftw3 -o yuvilace  $<

libav-bitrate: libav-bitrate.c progress.o
	$(CC) $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav-bitrate  progress.o $<

libavmux: libavmux.c 
	$(CC) $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libavmux $<

metadata-example: metadata-example.c
	$(CC) $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o metadata-example $^


clean:
	 rm -f *.o $(TARGETS)

install:
	cp $(TARGETS) /usr/local/bin
