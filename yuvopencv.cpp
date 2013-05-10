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
        
    //    oyuv.writeHeader();
    
        iyuv.allocFrameData();
        oyuv.allocFrameData();
        
        int width = iyuv.getWidth();
        int height = iyuv.getHeight();
        
//        std::cerr << "width: " << width <<" height: " << height << "\n";
        
        int cwidth = iyuv.getChromaWidth();
        int cheight = iyuv.getChromaHeight();

        
        // set to black
        memset (oyuv.getYUVFrame()[0],16,width*height);
        memset (oyuv.getYUVFrame()[1],128,cwidth*cheight);
        memset (oyuv.getYUVFrame()[2],128,cwidth*cheight);

     //   img1 = cv::Mat(height,width,CV_8UC1,oyuv.getYUVFrame()[0],width);
        img1.create(height,width,CV_32FC1);
        //imgout = img1.clone();
        
        cv::Point2d shift;
        
        cv::Point2d acc (0,0);

        while(1) {
            iyuv.read();
        
            imgout = cv::Mat(height,width,CV_8UC1,iyuv.getYUVFrame()[0],width);
            
            imgout.assignTo(img2,CV_32FC1);
            
        //    imgout = img2.clone();
  
        //    cv::accumulate(img2, imgout);
            
       //     std::cerr << "img1: " << img1.type() << " / img2: " << img2.type() << "\n";
        
            shift = cv::phaseCorrelate(img1,img2);
      
            acc = acc + shift;
            
       //     std::cout << shift << " - " << acc  << "\n";
         
            std::cout << acc << "\n";   
            
            img1 = img2.clone();
            
  /*
            if (imgout.step == width)
            {
                oyuv.getYUVFrame()[0] = imgout.ptr(0);
            }
            else
            {
                int y;
                for (y=0;y<height;y++)
                {
                    memcpy(oyuv.getYUVFrame()[0] + y * width,imgout.ptr(y),width);
                }
            }
            oyuv.write();
   */
        }
    } catch (AVException *e) {
		std::cerr << "ERROR occurred: " << e->getMessage() << "\n";
	} catch (cv::Exception *e) {
        std::cerr << "Exception occurred: " << e  << "\n";
    }
}