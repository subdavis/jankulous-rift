#include <stdio.h>
#include "cv.h"
#include "highgui.h"


typedef IplImage* (*callback_prototype)(IplImage*);


/* 
 * make_it_gray: custom callback to convert a colored frame to its grayscale version.
 * Remember that you must deallocate the returned IplImage* yourself after calling this function.
 */
IplImage* make_it_gray(IplImage* frame)
{
    // Allocate space for a new image
    IplImage* gray_frame = 0;
    gray_frame = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, 1);
    if (!gray_frame)
    {
      fprintf(stderr, "!!! cvCreateImage failed!\n" );
      return NULL;
    }

    cvCvtColor(frame, gray_frame, CV_RGB2GRAY);
    return gray_frame; 
}

/*
 * process_video: retrieves frames from camera and executes a callback to do individual frame processing.
 * Keep in mind that if your callback takes too much time to execute, you might loose a few frames from 
 * the camera.
 */
void process_video(callback_prototype custom_cb)
{           
    // Initialize camera
    CvCapture *capture = 0;
    capture = cvCaptureFromCAM(-1);
    if (!capture) 
    {
      fprintf(stderr, "!!! Cannot open initialize webcam!\n" );
      return;
    }

    // Create a window for the video 
    cvNamedWindow("result", CV_WINDOW_AUTOSIZE);

    IplImage* frame = 0;
    char key = 0;
    while (key != 27) // ESC
    {    
      frame = cvQueryFrame(capture);
      if(!frame) 
      {
          fprintf( stderr, "!!! cvQueryFrame failed!\n" );
          break;
      }

      // Execute callback on each frame
      IplImage* processed_frame = (*custom_cb)(frame);

      // Display processed frame
      cvShowImage("result", processed_frame);

      // Release resources
      cvReleaseImage(&processed_frame);

      // Exit when user press ESC
      key = cvWaitKey(10);
    }

    // Free memory
    cvDestroyWindow("result");
    cvReleaseCapture(&capture);
}

int main( int argc, char **argv )
{
    process_video(make_it_gray);

    return 0;
}
