AC_INIT([MarkYUV], [1.4], [mjpeg0@silicontrip.org])
AM_INIT_AUTOMAKE([MarkYUV],[1.4])
AC_PROG_CC
AC_PROG_OBJC
AC_LANG_PUSH([Objective C])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
AC_CHECK_LIB([mjpegutils],[mjpeg_default_handler_verbosity],have_mjpegutils="yes",[echo "Error! mjpegutils must be installed" exit -1])
AC_CHECK_LIB([avcodec],[avcodec_decode_audio2],have_audio2="yes")
AC_CHECK_LIB([avcodec],[avcodec_decode_audio3],have_audio3="yes")
AC_CHECK_LIB([avcodec],[avcodec_decode_video],have_video="yes")
AC_CHECK_LIB([avcodec],[avcodec_decode_video2],have_video2="yes")
AC_CHECK_LIB([avformat],[open_av_file],have_avformat="yes")
AC_CHECK_LIB([avutil],[av_free_packet],have_av_free_packet="yes")
AC_CHECK_LIB([fftw3],[fftw_plan_r2r_2d],have_fftw="yes")
AC_CHECK_LIB([freetype2],[FT_New_Face],have_freetype="yes")
AC_CHECK_HEADER([QuartzCore/CIFilter.h],have_coreimage="yes")

AM_CONDITIONAL([HAVE_COREIMAGE], test "x$have_coreimage" = xyes)
AM_CONDITIONAL(HAVE_FFTW, test "x$have_fftw" = xyes)
AM_CONDITIONAL(HAVE_FFMPEG, test "x$have_avformat" = xyes)
AM_CONDITIONAL(HAVE_FREETYPE, test "x$have_freetype" = xyes)

