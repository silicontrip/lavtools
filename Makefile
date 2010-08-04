
LDFLAGS=-L/usr/local/lib -lmjpegutils 
CFLAGS=-O3 -I/usr/local/include/mjpegtools -I/usr/local/include/ffmpeg

all: libav-bitrate libav2yuv libavmux yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop yuvdeinterlace yuvdiag yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater

yuvtshot: yuvtshot.o utilyuv.o

yuvdiff: yuvdiff.o utilyuv.o

libav2yuv: libav2yuv.c
	gcc -lavcodec -lavformat -lavutil $(LDFLAGS) $(CFLAGS) -o libav2yuv

libav-bitrate: libav-bitrate.c
	gcc -lavcodec -lavformat -lavutil $(LDFLAGS) $(CFLAGS) -o libav-bitrate

libavmux: libavmux.c
	gcc -lavcodec -lavformat -lavutil $(LDFLAGS) $(CFLAGS) -o libavmux
