
OPT_FLAG=-g
#OPT_FLAG=-O3
LDFLAGS=-L/usr/local/lib -lmjpegutils -L/opt/local/lib
CFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/opt/local/include -I/usr/local/include
FFMPEG_FLAGS=-lswscale -lavcodec -lavformat -lavutil

all: libav-bitrate libav2yuv libavmux yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop yuvdeinterlace yuvdiag yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater


yuvtshot: yuvtshot.o utilyuv.o

yuvdiff: yuvdiff.o utilyuv.o

yuvbilateral: yuvbilateral.o utilyuv.o

yuvtbilateral: yuvtbilateral.o utilyuv.o

yuvCIFilter: yuvCIFilter.o
	gcc $(LDFLAGS) $(CFLAGS) -framework QuartzCore -framework Foundation -framework AppKit -o yuvCIFilter $<

yuvilace: yuvilace.o utilyuv.o
	gcc $(LDFLAGS) $(CFLAGS) -lfftw3 -o yuvilace utilyuv.o $<

libav2yuv: libav2yuv.c
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav2yuv $<

libav-bitrate: libav-bitrate.c
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libav-bitrate  $<

libavmux: libavmux.c
	gcc $(FFMPEG_FLAGS) $(LDFLAGS) $(CFLAGS) -o libavmux $<
