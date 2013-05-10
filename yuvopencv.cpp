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
#include "Libyuv.h"
#include "AVException.h"

#include <opencv2/imgproc/imgproc.hpp>

int main (int argc, char **argv)
{
    
    Libyuv iyuv,oyuv;
    int y;
    iyuv.setExtensions(1);
    oyuv.setExtensions(1);
    
    cv::Mat img1, img2, imgout;
    
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
        int cheight = iyuv.getChromaHeight();
        int height = iyuv.getHeight();

        memset (oyuv.getYUVFrame()[1],128,cwidth*cheight);
        memset (oyuv.getYUVFrame()[2],128,cwidth*cheight);

        
        while(1) {
            iyuv.read();
        

            img1 = cv::Mat(width,height,CV_8UC1,iyuv.getYUVFrame()[0],width);

          imgout = cv::Mat(width,height,CV_8UC1,oyuv.getYUVFrame()[0],width);
            
     //       blur(img1,imgout,cv::Size(2,2),cv::Point(-1,-1));
            
            
            
            oyuv.write();
        }
    } catch (AVException *e) {
		std::cerr << "ERROR occurred: " << e->getMessage() << "\n";
	}
}