AC_INIT([MarkYUV], [1.4], [mjpeg0@silicontrip.org])
AM_INIT_AUTOMAKE([MarkYUV],[1.4])
AC_PROG_CC
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
AC_ARG_WITH(mjpegtools, AC_HELP_STRING([--with-mjpegtools=PATH], [PATH where mjpegtools are installed.]),with_mjpegtools=$withval)
AC_ARG_WITH(ffmpeg, AC_HELP_STRING([--with-ffmpeg=PATH], [PATH where libavcodec is installed.]),with_ffmpeg=$withval)
AC_ARG_WITH(fftw, AC_HELP_STRING([--with-fftw=PATH], [PATH where FFTW is installed.]),with_fftw=$withval)
AC_ARG_WITH(freetype, AC_HELP_STRING([--with-freetype=PATH], [PATH where Freetype is installed.]),with_freetype=$withval)

if test x$with_mjpegtools = xyes ; then
  AC_MSG_ERROR([
*** A directory must be specified for --with-mjpegtools option.])
fi
if test x$with_mjpegtools = x ; then
with_mjpegtools=/usr/local
fi

if test x$with_ffmpeg = xyes ; then
  AC_MSG_ERROR([
*** A directory must be specified for --with-ffmpeg option.])
fi
if test x$with_ffmpeg = x ; then
with_ffmpeg=/usr/local
fi

if test x$with_freetype = xyes ; then
  AC_MSG_ERROR([
*** A directory must be specified for --with-freetype option.])
fi
if test x$with_freetype = x ; then
with_freetype=/usr/local
fi


OLD_CFLAGS="$CFLAGS"
OLD_LIBS="$LIBS"
LIBS="$LIBS -L$with_mjpegtools/lib"
CFLAGS="$CFLAGS -I$with_mjpegtools/include"
AC_CHECK_LIB([mjpegutils],[mjpeg_default_handler_verbosity],have_mjpegutils="yes",[echo "Error! mjpegutils must be installed" ; exit -1])
MJPEG_LIBS="-L$with_mjpegtools/lib"
MJPEG_CFLAGS="-I$with_mjpegtools/include"
LIBS="$OLD_LIBS"
CFLAGS="$OLD_CFLAGS"

LIBS="$LIBS -L$with_ffmpeg/lib"
CFLAGS="$CFLAGS -I$with_ffmpeg/include"
AC_CHECK_LIB([avcodec],[av_free_packet],have_ffmpeg="yes")
AC_CHECK_LIB([avcodec],[avcodec_decode_audio2],have_audio2="yes")
AC_CHECK_LIB([avcodec],[avcodec_decode_audio3],have_audio3="yes")
AC_CHECK_LIB([avcodec],[avcodec_decode_video],have_video="yes")
AC_CHECK_LIB([avcodec],[avcodec_decode_video2],have_video2="yes")
AC_CHECK_HEADER([libavcodec/avcodec.h],have_avcodec="yes")
AC_CHECK_HEADER([libavformat/avformat.h],have_avformat="yes")
AC_CHECK_HEADER([libswscale/swscale.h],have_swscale="yes")
FFMPEG_LIBS="-L$with_ffmpeg/lib"
FFMPEG_CFLAGS="-I$with_ffmpeg/include"
LIBS="$OLD_LIBS"
CFLAGS="$OLD_CFLAGS"

LIBS="$LIBS -L$with_fftw/lib"
CFLAGS="$CFLAGS -I$with_fftw/include"
AC_CHECK_LIB([fftw3],[fftw_plan_r2r_2d],have_fftw="yes")
FFTW_LIBS="-L$with_fftw/lib"
FFTW_CFLAGS="-I$with_fftw/include"
LIBS="$OLD_LIBS"
CFLAGS="$OLD_CFLAGS"

LIBS="$LIBS -L$with_freetype/lib"
CFLAGS="$CFLAGS -I$with_ffreetype/include"
AC_CHECK_LIB([freetype],[FT_New_Face],have_freetype="yes")
FREETYPE_LIBS="-L$with_freetype/lib"
FREETYPE_CFLAGS="-I$with_ffreetype/include"
LIBS="$OLD_LIBS"
CFLAGS="$OLD_CFLAGS"

AC_LANG_PUSH([Objective C])
AC_CHECK_HEADER([QuartzCore/CIFilter.h],have_coreimage="yes")
if (test "x$have_coreimage" == "xyes")
then
	AC_PROG_OBJC

fi


AM_CONDITIONAL(HAVE_COREIMAGE, test "x$have_coreimage" = xyes)
AM_CONDITIONAL(HAVE_FFTW, test "x$have_fftw" = xyes)
AM_CONDITIONAL(HAVE_FFMPEG, test "x$have_avformat" = xyes)
AM_CONDITIONAL(HAVE_FREETYPE, test "x$have_freetype" = xyes)

AC_SUBST(MJPEG_LIBS)
AC_SUBST(MJPEG_CFLAGS)
AC_SUBST(FFTW_LIBS)
AC_SUBST(FFTW_CFLAGS)
AC_SUBST(FFMPEG_LIBS)
AC_SUBST(FFMPEG_CFLAGS)
AC_SUBST(FREETYPE_LIBS)
AC_SUBST(FREETYPE_CFLAGS)
