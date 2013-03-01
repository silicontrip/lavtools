
CC=gcc
OPT_FLAG=-g 
#OPT_FLAG=-O3 -ftree-vectorize
FREETYPEFLAGS=-L/usr/X11/lib -lfreetype
COCOAFLAGS=-framework QuartzCore -framework Foundation -framework AppKit
CODECFLAGS=
#CODECFLAGS=-DHAVE_AV_FREE_PACKET
#LDFLAGS=-L/usr/local/lib -lmjpegutils -L/opt/local/lib 
CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/usr/local/include  -I/usr/X11/include -I/usr/X11/include/freetype2
#CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/opt/local/include -I/usr/local/include -I/opt/local/include/freetype2
FFMPEG_FLAGS= $(CODECFLAGS) -lswscale -lavcodec -lavformat -lavutil
#FFMPEG_FLAGS= $(CODECFLAGS) -lavcodec -lavformat -lavutil
JPEGFLAGS= -ljpeg
MJPEGFLAGS= -lmjpegutils
LDFLAGS=-L/usr/X11/lib  -L/usr/local/lib  $(MJPEGFLAGS)


DEPRECATED_TARGETS=libav2yuv libavmux

TARGETS=libav-bitrate yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop \
		yuvdeinterlace yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater yuvbilateral \
		yuvtbilateral yuvCIFilter yuvdiag yuvpixelgraph yuvfieldrev yuvtemporal yuvtout \
		yuvyadif yuvnlmeans yuvvalues


all: $(TARGETS)

yuvadjust: utilyuv.o yuvadjust.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvadjust  $^

yuvTHREADED: utilyuv.o yuvTHREADED.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvTHREADED  $^

yuvdeinterlace: utilyuv.o yuvdeinterlace.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvdeinterlace  $^

yuvtshot: yuvtshot.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvtshot  $^

yuvdiff: yuvdiff.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvdiff  $^

yuvfieldrev: yuvfieldrev.o utilyuv.o 
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvfieldrev  $^

yuvpixelgraph: yuvpixelgraph.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvpixelgraph  $^

yuvbilateral: yuvbilateral.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvbilateral  $^

yuvtbilateral: yuvtbilateral.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvtbilateral  $^

yuvtemporal: yuvtemporal.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS)  -o yuvtemporal  $^


yuvtout: yuvtout.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS)  -o yuvtout  $^

yuvyadif: yuvyadif.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvyadif  $^


yuvnlmeans: yuvnlmeans.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvnlmeans  $^


yuvvalues: yuvvalues.o utilyuv.o
	$(CC)  $(CFLAGS) $(LDFLAGS)  -o yuvvalues  $^


yuv2jpeg: yuv2jpeg.o utilyuv.o
	$(CC) $(LDFLAGS) $(CFLAGS)  $(JPEGFLAGS) -o yuv2jpeg $^

yuvsubtitle: yuvsubtitle.o utilyuv.o
	$(CC) $(LDFLAGS) $(CFLAGS) $(MJPEGFLAGS) $(FREETYPEFLAGS) -o yuvsubtitle  $^

yuvdiag: yuvdiag.o utilyuv.o
	$(CC)  $(LDFLAGS) $(MJPEGFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvdiag  $^

yuvCIFilter: yuvCIFilter.o utilyuv.o
	$(CC) $(LDFLAGS) $(CFLAGS) $(MJPEGFLAGS) $(COCOAFLAGS) -o yuvCIFilter $^

yuvilace: yuvilace.o  utilyuv.o
	$(CC) $(LDFLAGS) $(CFLAGS) $(MJPEGFLAGS) -lfftw3 -o yuvilace  $^

libav2yuv.o: libav2yuv.c
	$(CC) $(FFMPEG_FLAGS) $(CFLAGS) -c -o libav2yuv.o $^

libav2yuv: libav2yuv.o utilyuv.o
	$(CC) $(FFMPEG_FLAGS) $(MJPEGFLAGS) $(LDFLAGS) $(CFLAGS) -o libav2yuv $^

libav-bitrate: libav-bitrate.c progress.o
	$(CC) $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav-bitrate  $^

libavmux: libavmux.c 
	$(CC) $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libavmux $^

clean:
	 rm -f *.o $(TARGETS)

install:
	cp $(TARGETS) /usr/local/bin
