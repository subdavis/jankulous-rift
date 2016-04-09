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

struct v3 {
    int x, y, z;
};

v2 *red;
v2 *green;
v2 *blue;
v2 *orange;
v2 *midp;
v2 *lastmid;

//Diffs
v2* yz_midpoint;// the height of purple from midpoint.
v2* xz_midpoint;// the left-right distance of purple from midpoint
v2* xy_midpoint;// hight diff of red from midpoint.

int getCoords(Mat imgHSV, v2 *color, int* range){
  Mat imgThresholded;
  inRange(imgHSV, Scalar(range[0], range[2], range[4]), Scalar(range[1], range[3], range[5]), imgThresholded);
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
    color->x = posX;
    color->y = posY; 
  } else {
    color->x = 0;
    color->y = 0;
  }
  // imshow("Thresholded Image", imgThresholded); //show the thresholded image
  return 0;
}

int trackColor(v2 *ret, Mat imgLines, VideoCapture cap, int *range_r, int *range_g, int *range_b, int *range_o){

  Mat imgOriginal;

  bool bSuccess = cap.read(imgOriginal); // read a new frame from video

  if (!bSuccess){ //if not success, break loop{
    cout << "Cannot read a frame from video stream" << endl;
  }

  Mat imgHSV; 

  cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

  getCoords(imgHSV, red, range_r);
  getCoords(imgHSV, green, range_g);
  getCoords(imgHSV, blue, range_b);
  getCoords(imgHSV, orange, range_o);

  if (red->x > 0  && red->y > 0 && green->x > 0 && green->y > 0){
      midp->x = (green->x + red->x) / 2; 
      midp->y = (green->y + red->y) / 2;
      //set the distance between thingies.
      //distance formula
      width = sqrt( pow((green->y - red->y), 2) + pow((green->x - red->x), 2) );

      //set purple height diff
      yz_midpoint->x = blue->x - midp->x;
      yz_midpoint->y = blue->y - midp->y;
  }   

  if (lastmid->x > 0 && lastmid->y > 0 && midp->x > 0 && midp->y > 0 && blue->x > 0){
      //Draw a red line from the previous point to the current point
      // line(imgLines, Point(midp->x, midp->y), Point(lastmid->x, lastmid->y), Scalar(0,0,255), 2);
      line(imgLines, Point(blue->x, blue->y), Point(midp->x, midp->y), Scalar(0,0,255), 2);
      line(imgLines, Point(orange->x, orange->y), Point(midp->x, midp->y), Scalar(0,255,0), 2);
      line(imgLines, Point(red->x, red->y), Point(midp->x, midp->y), Scalar(255,0,0), 2);
  }
  
  //TODO - anything to do here?
  lastmid->x = midp->x;
  lastmid->y = midp->y;

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
  blue = new v2();
  orange = new v2();
  midp = new v2();
  lastmid = new v2();
  xz_midpoint = new v2();
  yz_midpoint = new v2();
  xy_midpoint = new v2();

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
  int blue_range[] = {118, 138, 124, 255, 81, 228};
  int orange_range[] = {0, 23, 117, 255, 161, 255};

  //Capture a temporary image from the camera
  Mat imgTmp;
  cap.read(imgTmp); 

  //Create a black image with the size as the camera output
  // Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );

  while (true){
    Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );;
    trackColor(red, imgLines, cap, red_range, green_range, blue_range, orange_range);
    //position of midpoint will be in midp
    //distance, in pixels, between the endpoints will be in width

    // std::cout << width << std::endl;
  }
  
  return 0;
}