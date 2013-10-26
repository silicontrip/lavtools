CC?=gcc
CXX?=g++
CFLAGS?=-g
CXXFLAGS?=-g
BINDIR?=/usr/local/bin

FFMPEG_LIBS=-lavcodec -lavformat -lavutil
MATH_LIBS=-lm
MJPEG_LIBS=-lmjpegutils
FREETYPE_LIBS=-lfreetype
JPEG_LIBS=-ljpeg
OPENCV_LIBS=-lopencv_core -lopencv_highgui -lopencv_imgproc
COCOA_LIBS=-framework QuartzCore -framework Foundation -framework AppKit

DEPRECATED_TARGETS=libav2yuv libavmux
DARWIN_TARGETS=yuvCIFilter
MAIN_TARGETS=libav-bitrate metadata-example yuv2jpeg yuvaddetect yuvadjust yuvaifps \
	yuvbilateral yuvconvolve yuvcrop yuvdeinterlace yuvdiag yuvdiff yuvfade yuvfieldrev \
	yuvfieldseperate yuvhsync yuvilace yuvnlmeans yuvopencv yuvpixelgraph yuvrfps \
	yuvsubtitle yuvtbilateral yuvtout yuvtshot yuvvalues yuvwater yuvyadif

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
	 rm -f *.o libav2yuv/*.o $(TARGETS)

install:
	install -d $(DESTDIR)$(BINDIR)
	for i in $(TARGETS) ; do install -m0755 $$i $(DESTDIR)$(BINDIR) ; done


yuvfieldseperate: yuvfieldseperate.o libav2yuv/Libyuv.o libav2yuv/AVException.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvopencv: yuvopencv.o libav2yuv/Libyuv.o libav2yuv/AVException.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(OPENCV_LIBS)

yuvhsync: utilyuv.o yuvhsync.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvcrop: utilyuv.o yuvcrop.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvconvolve: yuvconvolve.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS)

yuvadjust: utilyuv.o yuvadjust.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS)

yuvdeinterlace: utilyuv.o yuvdeinterlace.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvtshot: yuvtshot.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS)

yuvdiff: yuvdiff.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvfieldrev: yuvfieldrev.o utilyuv.o 
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvpixelgraph: yuvpixelgraph.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvbilateral: yuvbilateral.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS)

yuvtbilateral: yuvtbilateral.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS)

yuvtout: yuvtout.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvyadif: yuvyadif.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvnlmeans: yuvnlmeans.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(MATH_LIBS)

yuvvalues: yuvvalues.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuv2jpeg: yuv2jpeg.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(JPEG_LIBS)

yuvaddetect: yuvaddetect.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvfade: yuvfade.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvaifps: yuvaifps.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvrfps: yuvrfps.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvwater: yuvwater.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

yuvsubtitle: yuvsubtitle.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(FREETYPE_LIBS)

yuvdiag: yuvdiag.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(FREETYPE_LIBS)

yuvCIFilter: yuvCIFilter.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS) $(COCOA_LIBS)

yuvilace: yuvilace.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(MJPEG_LIBS)

libav2yuv: libav2yuv.o utilyuv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS) $(MJPEG_LIBS)

libav-bitrate: libav-bitrate.o progress.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS)

metadata-example: metadata-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS)

libavmux: libavmux.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(FFMPEG_LIBS)
