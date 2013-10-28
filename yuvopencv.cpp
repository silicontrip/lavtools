/*
 * yuv scanning picture assembler
 * This program takes a horizontal and vertically scrolling video. (say a view of a map from a computer game)
 * And attempts to assemble it into a single large image.  Uses the opencv2 library. http://http://opencv.org
 *
 *
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


#include <vector>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define IMAGEDIM 7200
#define CROP 32

using namespace cv;
using namespace std;


int main (int argc, char **argv)
{

    Mat image;
    Libyuv iyuv,oyuv;
    int y;
    iyuv.setExtensions(1);
    oyuv.setExtensions(1);

    Mat img1, img2, imgout;

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

        image.create(IMAGEDIM,IMAGEDIM,CV_8UC1);
        //imgout = img1.clone();

        cv::Point2d shift;

        cv::Point2d acc (3600,6400);

        while(1) {
            iyuv.read();

            imgout = cv::Mat(height,width,CV_8UC1,iyuv.getYUVFrame()[0],width);

            imgout.assignTo(img2,CV_32FC1);

        //    imgout = img2.clone();

        //    cv::accumulate(img2, imgout);

       //     std::cerr << "img1: " << img1.type() << " / img2: " << img2.type() << "\n";

            shift = cv::phaseCorrelate(img1,img2);
            img1 = img2.clone();


            acc = acc + shift;

       //     std::cout << shift << " - " << acc  << "\n";

//std::cout  << shift << " - " << acc << "\n";

            int xl = IMAGEDIM - acc.x;
            int yl = IMAGEDIM - acc.y;

            cout << xl << "," << yl << "\n";

            int y;


            for (y=0;y<16;y++)
            {
                memcpy(image.ptr (CROP+y+yl) + xl + CROP,imgout.ptr(y+CROP)+CROP,width-CROP-CROP);
                memcpy(image.ptr (y+height-CROP+yl-16) + xl + CROP,imgout.ptr(y+height-CROP-16)+CROP,width-CROP-CROP);

            }

            for (y=CROP;y<height-CROP;y++)
            {
                //*(image.ptr (y+yl) + xl + CROP) = * (imgout.ptr(y)+CROP);

                memcpy(image.ptr (y+yl) + xl + CROP,imgout.ptr(y)+CROP,10);
                memcpy(image.ptr (y+yl) + xl + width - CROP -10 ,imgout.ptr(y)+width-CROP-10,10);
            }




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

    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);

    imwrite("out.png", image, compression_params);


}