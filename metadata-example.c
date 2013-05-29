/*
 * Copyright (c) 2011 Reinhard Tartler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 gcc -I/opt/local/include -L/opt/local/lib -std=gnu99 -lavformat  -lavutil  metadata-example.c   -o metadata-example
 
 */

/**
 * @file
 * @example libavformat/metadata-example.c
 * Shows how the metadata API can be used in application programs.
 */

#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>

const char *av_get_codecid(enum AVCodecID codec)
{
	switch (codec)
	{
		case AV_CODEC_ID_NONE: return "NONE";
		case AV_CODEC_ID_MPEG1VIDEO: return "MPEG1VIDEO";
		case AV_CODEC_ID_MPEG2VIDEO: return "MPEG2VIDEO";
		case AV_CODEC_ID_MPEG2VIDEO_XVMC: return "MPEG2VIDEO_XVMC";
		case AV_CODEC_ID_H261: return "H261";
		case AV_CODEC_ID_H263: return "H263";
		case AV_CODEC_ID_RV10: return "RV10";
		case AV_CODEC_ID_RV20: return "RV20";
		case AV_CODEC_ID_MJPEG: return "MJPEG";
		case AV_CODEC_ID_MJPEGB: return "MJPEGB";
		case AV_CODEC_ID_LJPEG: return "LJPEG";
		case AV_CODEC_ID_SP5X: return "SP5X";
		case AV_CODEC_ID_JPEGLS: return "JPEGLS";
		case AV_CODEC_ID_MPEG4: return "MPEG4";
		case AV_CODEC_ID_RAWVIDEO: return "RAWVIDEO";
		case AV_CODEC_ID_MSMPEG4V1: return "MSMPEG4V1";
		case AV_CODEC_ID_MSMPEG4V2: return "MSMPEG4V2";
		case AV_CODEC_ID_MSMPEG4V3: return "MSMPEG4V3";
		case AV_CODEC_ID_WMV1: return "WMV1";
		case AV_CODEC_ID_WMV2: return "WMV2";
		case AV_CODEC_ID_H263P: return "H263P";
		case AV_CODEC_ID_H263I: return "H263I";
		case AV_CODEC_ID_FLV1: return "FLV1";
		case AV_CODEC_ID_SVQ1: return "SVQ1";
		case AV_CODEC_ID_SVQ3: return "SVQ3";
		case AV_CODEC_ID_DVVIDEO: return "DVVIDEO";
		case AV_CODEC_ID_HUFFYUV: return "HUFFYUV";
		case AV_CODEC_ID_CYUV: return "CYUV";
		case AV_CODEC_ID_H264: return "H264";
		case AV_CODEC_ID_INDEO3: return "INDEO3";
		case AV_CODEC_ID_VP3: return "VP3";
		case AV_CODEC_ID_THEORA: return "THEORA";
		case AV_CODEC_ID_ASV1: return "ASV1";
		case AV_CODEC_ID_ASV2: return "ASV2";
		case AV_CODEC_ID_FFV1: return "FFV1";
		case AV_CODEC_ID_4XM: return "4XM";
		case AV_CODEC_ID_VCR1: return "VCR1";
		case AV_CODEC_ID_CLJR: return "CLJR";
		case AV_CODEC_ID_MDEC: return "MDEC";
		case AV_CODEC_ID_ROQ: return "ROQ";
		case AV_CODEC_ID_INTERPLAY_VIDEO: return "INTERPLAY_VIDEO";
		case AV_CODEC_ID_XAN_WC3: return "XAN_WC3";
		case AV_CODEC_ID_XAN_WC4: return "XAN_WC4";
		case AV_CODEC_ID_RPZA: return "RPZA";
		case AV_CODEC_ID_CINEPAK: return "CINEPAK";
		case AV_CODEC_ID_WS_VQA: return "WS_VQA";
		case AV_CODEC_ID_MSRLE: return "MSRLE";
		case AV_CODEC_ID_MSVIDEO1: return "MSVIDEO1";
		case AV_CODEC_ID_IDCIN: return "IDCIN";
		case AV_CODEC_ID_8BPS: return "8BPS";
		case AV_CODEC_ID_SMC: return "SMC";
		case AV_CODEC_ID_FLIC: return "FLIC";
		case AV_CODEC_ID_TRUEMOTION1: return "TRUEMOTION1";
		case AV_CODEC_ID_VMDVIDEO: return "VMDVIDEO";
		case AV_CODEC_ID_MSZH: return "MSZH";
		case AV_CODEC_ID_ZLIB: return "ZLIB";
		case AV_CODEC_ID_QTRLE: return "QTRLE";
		case AV_CODEC_ID_SNOW: return "SNOW";
		case AV_CODEC_ID_TSCC: return "TSCC";
		case AV_CODEC_ID_ULTI: return "ULTI";
		case AV_CODEC_ID_QDRAW: return "QDRAW";
		case AV_CODEC_ID_VIXL: return "VIXL";
		case AV_CODEC_ID_QPEG: return "QPEG";
		case AV_CODEC_ID_PNG: return "PNG";
		case AV_CODEC_ID_PPM: return "PPM";
		case AV_CODEC_ID_PBM: return "PBM";
		case AV_CODEC_ID_PGM: return "PGM";
		case AV_CODEC_ID_PGMYUV: return "PGMYUV";
		case AV_CODEC_ID_PAM: return "PAM";
		case AV_CODEC_ID_FFVHUFF: return "FFVHUFF";
		case AV_CODEC_ID_RV30: return "RV30";
		case AV_CODEC_ID_RV40: return "RV40";
		case AV_CODEC_ID_VC1: return "VC1";
		case AV_CODEC_ID_WMV3: return "WMV3";
		case AV_CODEC_ID_LOCO: return "LOCO";
		case AV_CODEC_ID_WNV1: return "WNV1";
		case AV_CODEC_ID_AASC: return "AASC";
		case AV_CODEC_ID_INDEO2: return "INDEO2";
		case AV_CODEC_ID_FRAPS: return "FRAPS";
		case AV_CODEC_ID_TRUEMOTION2: return "TRUEMOTION2";
		case AV_CODEC_ID_BMP: return "BMP";
		case AV_CODEC_ID_CSCD: return "CSCD";
		case AV_CODEC_ID_MMVIDEO: return "MMVIDEO";
		case AV_CODEC_ID_ZMBV: return "ZMBV";
		case AV_CODEC_ID_AVS: return "AVS";
		case AV_CODEC_ID_SMACKVIDEO: return "SMACKVIDEO";
		case AV_CODEC_ID_NUV: return "NUV";
		case AV_CODEC_ID_KMVC: return "KMVC";
		case AV_CODEC_ID_FLASHSV: return "FLASHSV";
		case AV_CODEC_ID_CAVS: return "CAVS";
		case AV_CODEC_ID_JPEG2000: return "JPEG2000";
		case AV_CODEC_ID_VMNC: return "VMNC";
		case AV_CODEC_ID_VP5: return "VP5";
		case AV_CODEC_ID_VP6: return "VP6";
		case AV_CODEC_ID_VP6F: return "VP6F";
		case AV_CODEC_ID_TARGA: return "TARGA";
		case AV_CODEC_ID_DSICINVIDEO: return "DSICINVIDEO";
		case AV_CODEC_ID_TIERTEXSEQVIDEO: return "TIERTEXSEQVIDEO";
		case AV_CODEC_ID_TIFF: return "TIFF";
		case AV_CODEC_ID_GIF: return "GIF";
		case AV_CODEC_ID_DXA: return "DXA";
		case AV_CODEC_ID_DNXHD: return "DNXHD";
		case AV_CODEC_ID_THP: return "THP";
		case AV_CODEC_ID_SGI: return "SGI";
		case AV_CODEC_ID_C93: return "C93";
		case AV_CODEC_ID_BETHSOFTVID: return "BETHSOFTVID";
		case AV_CODEC_ID_PTX: return "PTX";
		case AV_CODEC_ID_TXD: return "TXD";
		case AV_CODEC_ID_VP6A: return "VP6A";
		case AV_CODEC_ID_AMV: return "AMV";
		case AV_CODEC_ID_VB: return "VB";
		case AV_CODEC_ID_PCX: return "PCX";
		case AV_CODEC_ID_SUNRAST: return "SUNRAST";
		case AV_CODEC_ID_INDEO4: return "INDEO4";
		case AV_CODEC_ID_INDEO5: return "INDEO5";
		case AV_CODEC_ID_MIMIC: return "MIMIC";
		case AV_CODEC_ID_RL2: return "RL2";
		case AV_CODEC_ID_ESCAPE124: return "ESCAPE124";
		case AV_CODEC_ID_DIRAC: return "DIRAC";
		case AV_CODEC_ID_BFI: return "BFI";
		case AV_CODEC_ID_CMV: return "CMV";
		case AV_CODEC_ID_MOTIONPIXELS: return "MOTIONPIXELS";
		case AV_CODEC_ID_TGV: return "TGV";
		case AV_CODEC_ID_TGQ: return "TGQ";
		case AV_CODEC_ID_TQI: return "TQI";
		case AV_CODEC_ID_AURA: return "AURA";
		case AV_CODEC_ID_AURA2: return "AURA2";
		case AV_CODEC_ID_V210X: return "V210X";
		case AV_CODEC_ID_TMV: return "TMV";
		case AV_CODEC_ID_V210: return "V210";
		case AV_CODEC_ID_DPX: return "DPX";
		case AV_CODEC_ID_MAD: return "MAD";
		case AV_CODEC_ID_FRWU: return "FRWU";
		case AV_CODEC_ID_FLASHSV2: return "FLASHSV2";
		case AV_CODEC_ID_CDGRAPHICS: return "CDGRAPHICS";
		case AV_CODEC_ID_R210: return "R210";
		case AV_CODEC_ID_ANM: return "ANM";
		case AV_CODEC_ID_BINKVIDEO: return "BINKVIDEO";
		case AV_CODEC_ID_IFF_ILBM: return "IFF_ILBM";
		case AV_CODEC_ID_IFF_BYTERUN1: return "IFF_BYTERUN1";
		case AV_CODEC_ID_KGV1: return "KGV1";
		case AV_CODEC_ID_YOP: return "YOP";
		case AV_CODEC_ID_VP8: return "VP8";
		case AV_CODEC_ID_PICTOR: return "PICTOR";
		case AV_CODEC_ID_ANSI: return "ANSI";
		case AV_CODEC_ID_A64_MULTI: return "A64_MULTI";
		case AV_CODEC_ID_A64_MULTI5: return "A64_MULTI5";
		case AV_CODEC_ID_R10K: return "R10K";
		case AV_CODEC_ID_MXPEG: return "MXPEG";
		case AV_CODEC_ID_LAGARITH: return "LAGARITH";
		case AV_CODEC_ID_PRORES: return "PRORES";
		case AV_CODEC_ID_JV: return "JV";
		case AV_CODEC_ID_DFA: return "DFA";
		case AV_CODEC_ID_WMV3IMAGE: return "WMV3IMAGE";
		case AV_CODEC_ID_VC1IMAGE: return "VC1IMAGE";
		case AV_CODEC_ID_UTVIDEO: return "UTVIDEO";
		case AV_CODEC_ID_BMV_VIDEO: return "BMV_VIDEO";
		case AV_CODEC_ID_VBLE: return "VBLE";
		case AV_CODEC_ID_DXTORY: return "DXTORY";
		case AV_CODEC_ID_V410: return "V410";
		case AV_CODEC_ID_XWD: return "XWD";
		case AV_CODEC_ID_CDXL: return "CDXL";
		case AV_CODEC_ID_XBM: return "XBM";
		case AV_CODEC_ID_ZEROCODEC: return "ZEROCODEC";
		case AV_CODEC_ID_MSS1: return "MSS1";
		case AV_CODEC_ID_MSA1: return "MSA1";
		case AV_CODEC_ID_TSCC2: return "TSCC2";
		case AV_CODEC_ID_MTS2: return "MTS2";
		case AV_CODEC_ID_CLLC: return "CLLC";
		case AV_CODEC_ID_MSS2: return "MSS2";
		case AV_CODEC_ID_PCM_S16LE: return "PCM_S16LE ";
		case AV_CODEC_ID_PCM_S16BE: return "PCM_S16BE";
		case AV_CODEC_ID_PCM_U16LE: return "PCM_U16LE";
		case AV_CODEC_ID_PCM_U16BE: return "PCM_U16BE";
		case AV_CODEC_ID_PCM_S8: return "PCM_S8";
		case AV_CODEC_ID_PCM_U8: return "PCM_U8";
		case AV_CODEC_ID_PCM_MULAW: return "PCM_MULAW";
		case AV_CODEC_ID_PCM_ALAW: return "PCM_ALAW";
		case AV_CODEC_ID_PCM_S32LE: return "PCM_S32LE";
		case AV_CODEC_ID_PCM_S32BE: return "PCM_S32BE";
		case AV_CODEC_ID_PCM_U32LE: return "PCM_U32LE";
		case AV_CODEC_ID_PCM_U32BE: return "PCM_U32BE";
		case AV_CODEC_ID_PCM_S24LE: return "PCM_S24LE";
		case AV_CODEC_ID_PCM_S24BE: return "PCM_S24BE";
		case AV_CODEC_ID_PCM_U24LE: return "PCM_U24LE";
		case AV_CODEC_ID_PCM_U24BE: return "PCM_U24BE";
		case AV_CODEC_ID_PCM_S24DAUD: return "PCM_S24DAUD";
		case AV_CODEC_ID_PCM_ZORK: return "PCM_ZORK";
		case AV_CODEC_ID_PCM_S16LE_PLANAR: return "PCM_S16LE_PLANAR";
		case AV_CODEC_ID_PCM_DVD: return "PCM_DVD";
		case AV_CODEC_ID_PCM_F32BE: return "PCM_F32BE";
		case AV_CODEC_ID_PCM_F32LE: return "PCM_F32LE";
		case AV_CODEC_ID_PCM_F64BE: return "PCM_F64BE";
		case AV_CODEC_ID_PCM_F64LE: return "PCM_F64LE";
		case AV_CODEC_ID_PCM_BLURAY: return "PCM_BLURAY";
		case AV_CODEC_ID_PCM_LXF: return "PCM_LXF";
		case AV_CODEC_ID_S302M: return "S302M";
		case AV_CODEC_ID_PCM_S8_PLANAR: return "PCM_S8_PLANAR";
		case AV_CODEC_ID_ADPCM_IMA_QT: return "ADPCM_IMA_QT ";
		case AV_CODEC_ID_ADPCM_IMA_WAV: return "ADPCM_IMA_WAV";
		case AV_CODEC_ID_ADPCM_IMA_DK3: return "ADPCM_IMA_DK3";
		case AV_CODEC_ID_ADPCM_IMA_DK4: return "ADPCM_IMA_DK4";
		case AV_CODEC_ID_ADPCM_IMA_WS: return "ADPCM_IMA_WS";
		case AV_CODEC_ID_ADPCM_IMA_SMJPEG: return "ADPCM_IMA_SMJPEG";
		case AV_CODEC_ID_ADPCM_MS: return "ADPCM_MS";
		case AV_CODEC_ID_ADPCM_4XM: return "ADPCM_4XM";
		case AV_CODEC_ID_ADPCM_XA: return "ADPCM_XA";
		case AV_CODEC_ID_ADPCM_ADX: return "ADPCM_ADX";
		case AV_CODEC_ID_ADPCM_EA: return "ADPCM_EA";
		case AV_CODEC_ID_ADPCM_G726: return "ADPCM_G726";
		case AV_CODEC_ID_ADPCM_CT: return "ADPCM_CT";
		case AV_CODEC_ID_ADPCM_SWF: return "ADPCM_SWF";
		case AV_CODEC_ID_ADPCM_YAMAHA: return "ADPCM_YAMAHA";
		case AV_CODEC_ID_ADPCM_SBPRO_4: return "ADPCM_SBPRO_4";
		case AV_CODEC_ID_ADPCM_SBPRO_3: return "ADPCM_SBPRO_3";
		case AV_CODEC_ID_ADPCM_SBPRO_2: return "ADPCM_SBPRO_2";
		case AV_CODEC_ID_ADPCM_THP: return "ADPCM_THP";
		case AV_CODEC_ID_ADPCM_IMA_AMV: return "ADPCM_IMA_AMV";
		case AV_CODEC_ID_ADPCM_EA_R1: return "ADPCM_EA_R1";
		case AV_CODEC_ID_ADPCM_EA_R3: return "ADPCM_EA_R3";
		case AV_CODEC_ID_ADPCM_EA_R2: return "ADPCM_EA_R2";
		case AV_CODEC_ID_ADPCM_IMA_EA_SEAD: return "ADPCM_IMA_EA_SEAD";
		case AV_CODEC_ID_ADPCM_IMA_EA_EACS: return "ADPCM_IMA_EA_EACS";
		case AV_CODEC_ID_ADPCM_EA_XAS: return "ADPCM_EA_XAS";
		case AV_CODEC_ID_ADPCM_EA_MAXIS_XA: return "ADPCM_EA_MAXIS_XA";
		case AV_CODEC_ID_ADPCM_IMA_ISS: return "ADPCM_IMA_ISS";
		case AV_CODEC_ID_ADPCM_G722: return "ADPCM_G722";
		case AV_CODEC_ID_ADPCM_IMA_APC: return "ADPCM_IMA_APC";
		case AV_CODEC_ID_AMR_NB: return "AMR_NB ";
		case AV_CODEC_ID_AMR_WB: return "AMR_WB";
		case AV_CODEC_ID_RA_144: return "RA_144 ";
		case AV_CODEC_ID_RA_288: return "RA_288";
		case AV_CODEC_ID_ROQ_DPCM: return "ROQ_DPCM ";
		case AV_CODEC_ID_INTERPLAY_DPCM: return "INTERPLAY_DPCM";
		case AV_CODEC_ID_XAN_DPCM: return "XAN_DPCM";
		case AV_CODEC_ID_SOL_DPCM: return "SOL_DPCM";
		case AV_CODEC_ID_MP2: return "MP2";
		case AV_CODEC_ID_MP3: return "MP3";
		case AV_CODEC_ID_AAC: return "AAC";
		case AV_CODEC_ID_AC3: return "AC3";
		case AV_CODEC_ID_DTS: return "DTS";
		case AV_CODEC_ID_VORBIS: return "VORBIS";
		case AV_CODEC_ID_DVAUDIO: return "DVAUDIO";
		case AV_CODEC_ID_WMAV1: return "WMAV1";
		case AV_CODEC_ID_WMAV2: return "WMAV2";
		case AV_CODEC_ID_MACE3: return "MACE3";
		case AV_CODEC_ID_MACE6: return "MACE6";
		case AV_CODEC_ID_VMDAUDIO: return "VMDAUDIO";
		case AV_CODEC_ID_FLAC: return "FLAC";
		case AV_CODEC_ID_MP3ADU: return "MP3ADU";
		case AV_CODEC_ID_MP3ON4: return "MP3ON4";
		case AV_CODEC_ID_SHORTEN: return "SHORTEN";
		case AV_CODEC_ID_ALAC: return "ALAC";
		case AV_CODEC_ID_WESTWOOD_SND1: return "WESTWOOD_SND1";
		case AV_CODEC_ID_GSM: return "GSM";
		case AV_CODEC_ID_QDM2: return "QDM2";
		case AV_CODEC_ID_COOK: return "COOK";
		case AV_CODEC_ID_TRUESPEECH: return "TRUESPEECH";
		case AV_CODEC_ID_TTA: return "TTA";
		case AV_CODEC_ID_SMACKAUDIO: return "SMACKAUDIO";
		case AV_CODEC_ID_QCELP: return "QCELP";
		case AV_CODEC_ID_WAVPACK: return "WAVPACK";
		case AV_CODEC_ID_DSICINAUDIO: return "DSICINAUDIO";
		case AV_CODEC_ID_IMC: return "IMC";
		case AV_CODEC_ID_MUSEPACK7: return "MUSEPACK7";
		case AV_CODEC_ID_MLP: return "MLP";
		case AV_CODEC_ID_GSM_MS: return "GSM_MS";
		case AV_CODEC_ID_ATRAC3: return "ATRAC3";
		case AV_CODEC_ID_VOXWARE: return "VOXWARE";
		case AV_CODEC_ID_APE: return "APE";
		case AV_CODEC_ID_NELLYMOSER: return "NELLYMOSER";
		case AV_CODEC_ID_MUSEPACK8: return "MUSEPACK8";
		case AV_CODEC_ID_SPEEX: return "SPEEX";
		case AV_CODEC_ID_WMAVOICE: return "WMAVOICE";
		case AV_CODEC_ID_WMAPRO: return "WMAPRO";
		case AV_CODEC_ID_WMALOSSLESS: return "WMALOSSLESS";
		case AV_CODEC_ID_ATRAC3P: return "ATRAC3P";
		case AV_CODEC_ID_EAC3: return "EAC3";
		case AV_CODEC_ID_SIPR: return "SIPR";
		case AV_CODEC_ID_MP1: return "MP1";
		case AV_CODEC_ID_TWINVQ: return "TWINVQ";
		case AV_CODEC_ID_TRUEHD: return "TRUEHD";
		case AV_CODEC_ID_MP4ALS: return "MP4ALS";
		case AV_CODEC_ID_ATRAC1: return "ATRAC1";
		case AV_CODEC_ID_BINKAUDIO_RDFT: return "BINKAUDIO_RDFT";
		case AV_CODEC_ID_BINKAUDIO_DCT: return "BINKAUDIO_DCT";
		case AV_CODEC_ID_AAC_LATM: return "AAC_LATM";
		case AV_CODEC_ID_QDMC: return "QDMC";
		case AV_CODEC_ID_CELT: return "CELT";
		case AV_CODEC_ID_G723_1: return "G723_1";
		case AV_CODEC_ID_G729: return "G729";
		case AV_CODEC_ID_8SVX_EXP: return "8SVX_EXP";
		case AV_CODEC_ID_8SVX_FIB: return "8SVX_FIB";
		case AV_CODEC_ID_BMV_AUDIO: return "BMV_AUDIO";
		case AV_CODEC_ID_RALF: return "RALF";
		case AV_CODEC_ID_IAC: return "IAC";
		case AV_CODEC_ID_ILBC: return "ILBC";
		case AV_CODEC_ID_OPUS: return "OPUS";
		case AV_CODEC_ID_DVD_SUBTITLE: return "DVD_SUBTITLE ";
		case AV_CODEC_ID_DVB_SUBTITLE: return "DVB_SUBTITLE";
		case AV_CODEC_ID_TEXT: return "TEXT";
		case AV_CODEC_ID_XSUB: return "XSUB";
		case AV_CODEC_ID_SSA: return "SSA";
		case AV_CODEC_ID_MOV_TEXT: return "MOV_TEXT";
		case AV_CODEC_ID_HDMV_PGS_SUBTITLE: return "HDMV_PGS_SUBTITLE";
		case AV_CODEC_ID_DVB_TELETEXT: return "DVB_TELETEXT";
		case AV_CODEC_ID_SRT: return "SRT";
		case AV_CODEC_ID_TTF: return "TTF ";
		case AV_CODEC_ID_PROBE: return "PROBE ";
		case AV_CODEC_ID_MPEG2TS: return "MPEG2TS ";
		case AV_CODEC_ID_MPEG4SYSTEMS: return "MPEG4SYSTEMS ";
		case AV_CODEC_ID_FFMETADATA: return "FFMETADATA ";
		default: return NULL;
	}
}

const char *av_get_field_order(enum AVFieldOrder field_order) 
{

	switch (field_order) 
	{
		case AV_FIELD_UNKNOWN: return "UNKNOWN";
		case AV_FIELD_PROGRESSIVE: return "progressive";
		case AV_FIELD_TT: return "tt";
		case AV_FIELD_BB: return "bb";
		case AV_FIELD_TB: return "tb"; //< Top coded first, bottom displayed first
		case AV_FIELD_BT: return "bt"; //< Bottom coded first, top displayed first          
		default: return NULL;
	}
}

const char *av_get_media_type_string(enum AVMediaType media_type)
{
	    switch (media_type) {
			    case AVMEDIA_TYPE_VIDEO:      return "VIDEO";
			    case AVMEDIA_TYPE_AUDIO:      return "AUDIO";
			    case AVMEDIA_TYPE_DATA:       return "DATA";
			    case AVMEDIA_TYPE_SUBTITLE:   return "SUBTITLE";
			    case AVMEDIA_TYPE_ATTACHMENT: return "ATTACHMENT";
			    default:                      return NULL;
		    }
	}

const char *av_get_colorrange(enum AVColorRange color_range)
{

	switch(color_range)
	{
		case AVCOL_RANGE_UNSPECIFIED: "UNSPECIFIED";
		case AVCOL_RANGE_MPEG: return "mpeg";
		case AVCOL_RANGE_JPEG: return "jpeg";
		default: return NULL;
	}
			
}

const char *av_get_colorspace(enum AVColorSpace colorspace)
{
	switch(colorspace)
	{
		case AVCOL_SPC_RGB: return "rgb";
		case AVCOL_SPC_BT709: return "bt709";
		case AVCOL_SPC_UNSPECIFIED: return "UNSPECIFIED";
		case AVCOL_SPC_FCC: return "fcc";
		case AVCOL_SPC_BT470BG:    return "bt470bg";
		case AVCOL_SPC_SMPTE170M:  return "smpte170m";
		case AVCOL_SPC_SMPTE240M:  return "smpte240m";
		case AVCOL_SPC_YCOCG: return "ycocg";	
		default: return NULL;
	}
}

int main (int argc, char **argv)
{
	AVCodecContext *codec_ctx = NULL;

	char * container = NULL;
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret,i;

    if (argc != 2) {
        printf("usage: %s <input_file>\n"
               "example program to demonstrate the use of the libavformat metadata API.\n"
               "\n", argv[0]);
        return 1;
    }

    av_register_all();
    if ((ret = avformat_open_input(&fmt_ctx, argv[1], NULL, NULL)))
        return ret;

	if ((ret=avformat_find_stream_info(fmt_ctx,NULL))<0)
		return ret;
	
	if (strncmp(fmt_ctx->iformat->name,"mov,",4)==0) { container = "mov"; }
	if (strncmp(fmt_ctx->iformat->name,"matroska",8)==0) { container = "mkv"; }

	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        printf("%s=%s\n", tag->key, tag->value);
		if (strcmp(tag->key,"major_brand")==0) {
				// mp4 or mov
			if (strcmp(tag->value,"qt  ")==0) { container = "mov"; }
			if (strcmp(tag->value,"mp42")==0 || strcmp(tag->value,"isom")==0) { container = "mp4"; }
		}
	}


	
	if (container == NULL) {
		printf ("FORMAT_NAME=%s\n",fmt_ctx->iformat->name);
	} else {
		printf ("FORMAT_NAME=%s\n",container);
	}
	
	printf("%s=%d\n","STREAMS",fmt_ctx->nb_streams);
	
	for( i=0; i<fmt_ctx->nb_streams; i++) {
	
		codec_ctx = fmt_ctx->streams[i]->codec;

		const char * stream_type = av_get_media_type_string(fmt_ctx->streams[i]->codec->codec_type);
		
		printf ("STREAM_%d_TYPE=%s\n",i,stream_type);
		printf ("STREAM_%s_CODEC_ID=%s\n",stream_type,av_get_codecid(codec_ctx->codec_id));
	//	printf ("STREAM_%d_PROFILE=%d\n",i,codec_ctx->profile);

		if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			printf("STREAM_%s_AFPS_RATIO=%d:%d\n",stream_type,fmt_ctx->streams[i]->avg_frame_rate.num,fmt_ctx->streams[i]->avg_frame_rate.den);
			printf("STREAM_%s_FPS_RATIO=%d:%d\n",stream_type,fmt_ctx->streams[i]->r_frame_rate.num,fmt_ctx->streams[i]->r_frame_rate.den);
			printf("STREAM_%s_FPS=%f\n",stream_type,1.0*fmt_ctx->streams[i]->r_frame_rate.num/fmt_ctx->streams[i]->r_frame_rate.den);

			printf ("STREAM_%s_WIDTH=%d\n",stream_type,codec_ctx->width);
			printf ("STREAM_%s_HEIGHT=%d\n",stream_type,codec_ctx->height);
			printf ("STREAM_%s_PIX_FMT=%s\n",stream_type,av_get_pix_fmt_name(codec_ctx->pix_fmt));
			printf("STREAM_%s_SAR=%d:%d\n",stream_type,codec_ctx->sample_aspect_ratio.num,codec_ctx->sample_aspect_ratio.den);
			printf ("STREAM_%s_REFFRAMES=%d\n",stream_type,codec_ctx->refs);			
			printf ("STREAM_%s_COLORSPACE=%s\n",stream_type,av_get_colorspace(codec_ctx->colorspace));
			printf ("STREAM_%s_COLORRANGE=%s\n",stream_type,av_get_colorrange(codec_ctx->color_range));
			printf ("STREAM_%s_FIELDORDER=%s\n",stream_type,av_get_field_order(codec_ctx->field_order));


		}
		if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			printf ("STREAM_%s_SAMPLERATE=%d\n",stream_type,codec_ctx->sample_rate);
			printf ("STREAM_%s_CHANNELS=%d\n",stream_type,codec_ctx->channels);
			printf ("STREAM_%s_SAMPLEFORMAT=%s\n",stream_type,av_get_sample_fmt_name(codec_ctx->sample_fmt));
		}
		
		
		while ((tag = av_dict_get(fmt_ctx->streams[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
			printf("STREAM_%s_%s=%s\n", stream_type,tag->key, tag->value);

	}
	

    avformat_free_context(fmt_ctx);
    return 0;
}
