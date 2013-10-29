/*
 * yuv field seperator
 * Mark Heath
 * http://github.com/silicontrip/lavtools/
 *
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include <string.h>
#include "libav2yuv/Libyuv.h"
#include "libav2yuv/AVException.h"

int main (int argc, char **argv)
{

    Libyuv iyuv,oyuv;
    int y;
    iyuv.setExtensions(1);
    oyuv.setExtensions(1);

    try {

        iyuv.readHeader();
        iyuv.dumpInfo();
        // copy from in to out.
        oyuv.copyStreamInfo(&iyuv);

        oyuv.dumpInfo();
        oyuv.writeHeader();

        iyuv.allocFrameData();
        oyuv.allocFrameData();

        int width = iyuv.getWidth();
        int cwidth = iyuv.getChromaWidth();

        while(1) {
            iyuv.read();

            for (y=0; y<iyuv.getHeight();y++)
            {
                int ny = (y >> 1) + (iyuv.getHeight() >> 1) * (y % 2);

             //   std::cerr << "in: " << y << " out: " << ny << "\n";

                memcpy (&(oyuv.getYUVFrame()[0][ny*width]),
                        &(iyuv.getYUVFrame()[0][y*width]),width);
                // do chroma
                if (y < iyuv.getChromaHeight())
                {
                    int ny = (y >> 1) + (iyuv.getChromaHeight() >> 1) * (y % 2);

                    memcpy (&oyuv.getYUVFrame()[1][ny*cwidth],
                            &iyuv.getYUVFrame()[1][y*cwidth],cwidth);

                    memcpy (&oyuv.getYUVFrame()[2][ny*cwidth],
                            &iyuv.getYUVFrame()[2][y*cwidth],cwidth);


                }

            }

            oyuv.write();
        }
    } catch (AVException *e) {
		std::cerr << "ERROR occurred: " << e->getMessage() << "\n";
	}
}