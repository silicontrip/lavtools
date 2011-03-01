
#OPT_FLAG=-g
OPT_FLAG=-O3 -ftree-vectorize
#CODECFLAGS=-DHAVE_AVCODEC_DECODE_VIDEO2 -DHAVE_AVCODEC_DECODE_AUDIO3 -DHAVE_AV_FREE_PACKET
#CODECFLAGS=-DHAVE_AV_FREE_PACKET
AM_LDFLAGS=-L/usr/X11/lib -L/usr/local/lib -lmjpegutils -L/opt/local/lib 
AM_CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/opt/local/include -I/usr/local/include -I/opt/local/include/freetype2

bin_PROGRAMS= yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop \
	yuvdeinterlace yuvdiff yuvfade yuvhsync yuvrfps yuvtshot \
	yuvwater yuvbilateral  yuvtbilateral yuvpixelgraph 

if HAVE_FFMPEG

FFMPEG_FLAGS= $(CODECFLAGS) -lswscale -lavcodec -lavformat -lavutil
bin_PROGRAMS += libav-bitrate libav2yuv libavmux 

libav_bitrate_SOURCES = libav-bitrate.c
libav2yuv_SOURCES = libav2yuv.c
libavmux_SOURCES = libavmux.c

libav2yuv: libav2yuv.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav2yuv utilyuv.o $<

libav-bitrate: libav-bitrate.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav-bitrate  $<

libavmux: libavmux.c utilyuv.o
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libavmux $<

endif

if HAVE_FREETYPE

FREETYPEFLAGS=-L/usr/X11/lib -lfreetype
bin_PROGRAMS += yuvdiag yuvsubtitle
yuvsubtitle_SOURCES = yuvsubtitle.c utilyuv.c
yuvdiag_SOURCES = yuvdiag.c

yuvsubtitle: yuvsubtitle.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvsubtitle $<

yuvdiag: yuvdiag.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(FREETYPEFLAGS) -o yuvdiag utilyuv.o $<

endif

if HAVE_COREIMAGE

COCOAFLAGS=-framework QuartzCore -framework Foundation -framework AppKit
bin_PROGRAMS += yuvCIFilter 
yuvCIFilter_SOURCES = yuvCIFilter.m

yuvCIFilter: yuvCIFilter.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(COCOAFLAGS) -o yuvCIFilter $<

endif

if HAVE_FFTW

bin_PROGRAMS += yuvilace
yuvilace_SOURCES = yuvilace.c

yuvilace: yuvilace.o  utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) $(FFTWFLAGS) -o yuvilace utilyuv.o $<

endif

yuvaddetect_SOURCES =  yuvaddetect.c 
yuvadjust_SOURCES =  yuvadjust.c 
yuvaifps_SOURCES = yuvaifps.c
yuvconvolve_SOURCES = yuvconvolve.c
yuvcrop_SOURCES = yuvcrop.c
yuvdeinterlace_SOURCES = yuvdeinterlace.c utilyuv.c
yuvdiff_SOURCES = yuvdiff.c utilyuv.c
yuvfade_SOURCES = yuvfade.c
yuvhsync_SOURCES = yuvhsync.c
yuvrfps_SOURCES = yuvrfps.c
yuvtshot_SOURCES = yuvtshot.c utilyuv.c
yuvwater_SOURCES = yuvwater.c
yuvbilateral_SOURCES = yuvbilateral.c utilyuv.c
yuvtbilateral_SOURCES = yuvtbilateral.c utilyuv.c
yuvpixelgraph_SOURCES = yuvpixelgraph.c utilyuv.c

clean:
	 rm -f *.o $(TARGETS)

install:
	cp $(TARGETS) /usr/local/bin
