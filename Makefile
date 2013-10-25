CC?=gcc
CXX?=g++
CFLAGS?=-g
CXXFLAGS?=-g
BINDIR?=/usr/local/bin

FFMPEG_LIBS=-lswscale -lavcodec -lavformat -lavutil
MATH_LIBS=-lm
MJPEG_LIBS=-lmjpegutils
FREETYPE_LIBS=-lfreetype
FFTW_LIBS=-lfftw3
JPEG_LIBS=-ljpeg
OPENCV_LIBS=-lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_gpu -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_nonfree -lopencv_objdetect -lopencv_ocl -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab
COCOA_LIBS=-framework QuartzCore -framework Foundation -framework AppKit

DEPRECATED_TARGETS=libav2yuv libavmux
DARWIN_TARGETS=yuvCIFilter
MAIN_TARGETS=libav-bitrate yuvaddetect yuvadjust yuvaifps yuvconvolve yuvcrop \
	yuvdeinterlace yuvdiff yuvfade yuvhsync yuvrfps yuvtshot yuvwater yuvbilateral \
	yuvtbilateral yuvdiag yuvpixelgraph yuvfieldrev yuvtout \
	yuvyadif yuvnlmeans yuvvalues yuvfieldseperate yuvopencv metadata-example yuvilace

UNAME:=$(shell uname)
ifeq ($(UNAME), Darwin)
	CPPFLAGS= $(OPT_FLAG) -I/usr/local/include/mjpegtools -I/usr/local/include -I/usr/X11/include -I/usr/X11/include/freetype2 -D__STDC_CONSTANT_MACROS
	LDFLAGS=-L/usr/X11/lib -L/usr/local/lib
	TARGETS=$(MAIN_TARGETS) $(DARWIN_TARGETS)
else
	CPPFLAGS=-I/usr/include/mjpegtools -I/usr/include/freetype2 -D__STDC_CONSTANT_MACROS
	TARGETS=$(MAIN_TARGETS)
endif

.PHONY: clean install
all: $(TARGETS)
clean:
	 rm -f *.o $(TARGETS) $(DEPRECATED_TARGETS)

install:
	install -d $(DESTDIR)$(BINDIR)
	for i in $(TARGETS) ; do install -m0755 $$i $(DESTDIR)$(BINDIR) ; done


yuvfieldseperate: yuvfieldseperate.o libav2yuv/Libyuv.o libav2yuv/AVException.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvopencv: yuvopencv.o libav2yuv/Libyuv.o libav2yuv/AVException.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(OPENCV_LIBS) -o $@ $^

yuvhsync: utilyuv.o yuvhsync.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvcrop: utilyuv.o yuvcrop.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvconvolve: yuvconvolve.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS) -o $@ $^

yuvadjust: utilyuv.o yuvadjust.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS) -o $@ $^

yuvTHREADED: utilyuv.o yuvTHREADED.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvdeinterlace: utilyuv.o yuvdeinterlace.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvtshot: yuvtshot.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS) -o $@ $^

yuvdiff: yuvdiff.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvfieldrev: yuvfieldrev.o utilyuv.o 
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvpixelgraph: yuvpixelgraph.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvbilateral: yuvbilateral.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS) -o $@ $^

yuvtbilateral: yuvtbilateral.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS) -o $@ $^

yuvtout: yuvtout.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvyadif: yuvyadif.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvnlmeans: yuvnlmeans.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS) -o $@ $^

yuvvalues: yuvvalues.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuv2jpeg: yuv2jpeg.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(JPEG_LIBS) -o $@ $^

yuvaddetect: yuvaddetect.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvfade: yuvfade.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvaifps: yuvaifps.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvrfps: yuvrfps.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvwater: yuvwater.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) -o $@ $^

yuvsubtitle: yuvsubtitle.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(FREETYPE_LIBS) -o $@ $^

yuvdiag: yuvdiag.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(FREETYPE_LIBS) -o $@ $^

yuvCIFilter: yuvCIFilter.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(COCOA_LIBS) -o $@ $^

yuvilace: yuvilace.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(FFTW_LIBS) -o $@ $^

libav2yuv: libav2yuv.o utilyuv.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS) $(MJPEG_LIBS) -o $@ $^

libav-bitrate: libav-bitrate.o progress.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS) -o $@ $^

metadata-example: metadata-example.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS) -o $@ $^

libavmux: libavmux.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS) -o $@ $^
