#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <math.h>

using namespace cv;
using namespace std;

//http://opencv-srf.blogspot.com/2010/09/object-detection-using-color-seperation.html

int width;

struct v2{
   int x, y;
};

v2 *red;
v2 *green;
v2 *midp;
v2 *lastmid;

int trackColor(v2 *ret, Mat imgLines, VideoCapture cap, int *range){

  Mat imgOriginal;

  bool bSuccess = cap.read(imgOriginal); // read a new frame from video

  if (!bSuccess){ //if not success, break loop{
    cout << "Cannot read a frame from video stream" << endl;
  }

  Mat imgHSV; 

  cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
 
  Mat imgThresholded;

  inRange(imgHSV, Scalar(range[0], range[2], range[4]), Scalar(range[1], range[3], range[5]), imgThresholded); //Threshold the image
      
  //morphological opening (removes small objects from the foreground)
  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 

   //morphological closing (removes small holes from the foreground)
  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );

   //Calculate the moments of the thresholded image
  Moments oMoments = moments(imgThresholded);

  double dM01 = oMoments.m01;
  double dM10 = oMoments.m10;
  double dArea = oMoments.m00;

   // if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 
  if (dArea > 10000){
    //calculate the position of the ball
    int posX = dM10 / dArea;
    int posY = dM01 / dArea;

    //TODO set RET values 
    ret->x = posX;
    ret->y = posY; 

    if (red->x > 0  && red->y > 0 && green->x > 0 && green->y > 0){
      midp->x = (green->x + red->x) / 2; 
      midp->y = (green->y + red->y) / 2;
      //set the distance between thingies.
      //distance formula
      width = sqrt( pow((green->y - red->y), 2) + pow((green->x - red->x), 2) );
    }   

    if (lastmid->x > 0 && lastmid->y > 0 && midp->x > 0 && midp->y > 0){
      //Draw a red line from the previous point to the current point
      line(imgLines, Point(midp->x, midp->y), Point(lastmid->x, lastmid->y), Scalar(0,0,255), 2);
    }
      //TODO - anything to do here?
    lastmid->x = midp->x;
    lastmid->y = midp->y;
  }

  imshow("Thresholded Image", imgThresholded); //show the thresholded image

  imgOriginal = imgOriginal + imgLines;
  imshow("Original", imgOriginal); //show the original image

  if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
  {
    cout << "esc key is pressed by user" << endl;
  }

  return 0;
}

int main( int argc, char** argv ){
  red = new v2();
  green = new v2();
  midp = new v2();
  lastmid = new v2();

  VideoCapture cap(0); //capture the video from webcam

  if ( !cap.isOpened() )  // if not success, exit program
  {
    cout << "Cannot open the web cam" << endl;
    return -1;
  }

  // namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"

  // lowH, highH, lowS, highS, lowV, HighV
  int red_range[] = {170, 179, 150, 255, 60, 255}; 
  int green_range[] = {69, 98, 120, 255, 62, 180};

  //Capture a temporary image from the camera
  Mat imgTmp;
  cap.read(imgTmp); 

  //Create a black image with the size as the camera output
  Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );;

  while (true){

    trackColor(red, imgLines, cap, red_range);
    trackColor(green, imgLines, cap, green_range);

    //position of midpoint will be in midp
    //distance, in pixels, between the endpoints will be in width

    // std::cout << width << std::endl;
  }
  
  return 0;
}