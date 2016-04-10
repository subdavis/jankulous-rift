#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <math.h>
#include "opencv2/core/internal.hpp"
#include "opencv2/core/core.hpp"
// #include <opencv/highgui.h>
#include <cv.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>


#define FOCAL_LENGTH 760.0

using namespace cv;
using namespace std;

void transpose(float *src, float *dst, const int N, const int M);

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

float *rotation_m;

int pose(){
  // float wandSize s= 10;
  std::vector<CvPoint3D32f> modelPoints;
  modelPoints.push_back(cvPoint3D32f(0.0f, 0.0f, 0.0f));
  modelPoints.push_back(cvPoint3D32f(0.0f, 550.0f, 0.0f));
  modelPoints.push_back(cvPoint3D32f(500.0f, 0.0f, 0.0f)); //red
  modelPoints.push_back(cvPoint3D32f(-500.0f, 0.0f, 0.0f)); //green
  modelPoints.push_back(cvPoint3D32f(0.0f, 0.0f, 800.0f));
  CvPOSITObject *positObject = cvCreatePOSITObject( &modelPoints[0], static_cast<int>(modelPoints.size()) );
  
  //Shoudl be with respect to 0,0 in screen coordinates in the middle
  std::vector<CvPoint2D32f> projectedPoints;
  int halfx = 310, halfy = 240;
  projectedPoints.push_back(cvPoint2D32f(halfx - midp->x, halfy - midp->y));
  projectedPoints.push_back(cvPoint2D32f(halfx - orange->x, halfy - orange->y));
  projectedPoints.push_back(cvPoint2D32f(halfx - red->x, halfy - red->y));
  projectedPoints.push_back(cvPoint2D32f(halfx - green->x, halfy - green->y));
  projectedPoints.push_back(cvPoint2D32f(halfx - blue->x, halfy - blue->y));

  CvMatr32f rotation_matrix = new float[9];
  CvVect32f translation_vector = new float[3];
  CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 100, 1.0e-4f);
  cvPOSIT( positObject, &projectedPoints[0], FOCAL_LENGTH, criteria, rotation_matrix, translation_vector );
  // createOpenGLMatrixFrom( rotation_matrix, translation_vector);

  std::cout << rotation_matrix[0] << " " << rotation_matrix[1] << " " << rotation_matrix[2] << " "
      << rotation_matrix[3] << " " << rotation_matrix[4] << " " << rotation_matrix[5]<< " "
      << rotation_matrix[6] << " " << rotation_matrix[7] << " " << rotation_matrix[8] << std::endl;
  // std::cout << red->x << " " << red->y << std::endl;
  bool valid = true;
  for (int i = 0; i < 9; i++){
    if(rotation_matrix[i] > -0.00001 && rotation_matrix[i] < 0.00001){
      valid = false;
    }
  }
  if (valid) {
      rotation_m[0] = rotation_matrix[0];
      rotation_m[1] = rotation_matrix[1];
      rotation_m[2] = rotation_matrix[2];
      rotation_m[3] = 0;
      rotation_m[4] = rotation_matrix[3];
      rotation_m[5] = rotation_matrix[4];
      rotation_m[6] = rotation_matrix[5];
      rotation_m[7] = 0;
      rotation_m[8] = rotation_matrix[6];
      rotation_m[9] = rotation_matrix[7];
      rotation_m[10] = rotation_matrix[8];
      rotation_m[11] = 0;
      rotation_m[12] = 0;
      rotation_m[13] = 0;
      rotation_m[14] = 0;
      rotation_m[15] = 1;
    } else {
      // for (int i = 0; i < 12; i++){
      //   rotation_m[i] = 0;
      // }
    }
  return 0;
}

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
    // dot not found.
    // assume dot is in the same spot as purple
    color->x = blue->x;
    color->y = blue->y;
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
  rotation_m = new float[16];
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
  int green_range[] = {58, 104, 137, 234, 49, 154};
  int blue_range[] = {121, 151, 149, 255, 81, 228};
  int orange_range[] = {0, 20, 156, 255, 194, 255};

  //Capture a temporary image from the camera
  Mat imgTmp;
  cap.read(imgTmp); 

  //Create a black image with the size as the camera output
  // Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );

  while (true){
    Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );;
    trackColor(red, imgLines, cap, red_range, green_range, blue_range, orange_range);
    pose();
    //position of midpoint will be in midp
    //distance, in pixels, between the endpoints will be in width

    // std::cout << width << std::endl;
  }
  
  return 0;
}

void transpose(float *src, float *dst, const int N, const int M) {
    for(int n = 0; n<N*M; n++) {
        int i = n/N;
        int j = n%N;
        dst[n] = src[M*j + i];
    }
}