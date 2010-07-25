
LDFLAGS=-L/usr/local/lib -lmjpegutils -lavcodec -lavformat -lavutil
CFLAGS=-O3 -I/usr/local/include/mjpegtools -I/usr/local/include/ffmpeg

all: libav-bitrate libav2yuv libavmux yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop yuvdeinterlace yuvdiag yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater

yuvtshot: yuvtshot.o utilyuv.o
