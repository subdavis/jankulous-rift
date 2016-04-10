#include <stdio.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

//Just for the OBJ loader
#include <string.h>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <float.h>
#include <stdio.h>

//for openCV
#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <math.h>
#include "opencv2/core/internal.hpp"
#include "opencv2/core/core.hpp"
// #include <opencv/highgui.h>
#include <cv.h>
#include <GL/glut.h>
#include <vector>

#define FOCAL_LENGTH 760.0

using namespace cv;
using namespace std;

int width;
float *rotation_m;
int counter;

struct v2{
   int x, y;
};

struct v3 {
    int x, y, z;
};

v2 *red;
// v2 **red;
v2 *green;
// v2 **green;
v2 *blue;
// v2 **blue;
v2 *orange;
v2 *midp;
v2 *lastmid;

int average_over = 4;

//Diffs
v2* yz_midpoint;// the height of purple from midpoint.
v2* xz_midpoint;// the left-right distance of purple from midpoint
v2* xy_midpoint;// hight diff of red from midpoint.

// lowH, highH, lowS, highS, lowV, HighV
int red_range[] = {170, 179, 150, 255, 60, 255}; 
int green_range[] = {69, 98, 120, 255, 62, 180};
int blue_range[] = {118, 138, 124, 255, 81, 228};
int orange_range[] = {5, 23, 117, 255, 161, 255};

Mat imgTmp;
Mat imgLines;
VideoCapture cap;

//decide whether to buffered render
bool buffered_render;

//For buffered render
float *gNorms, *gVerts;
GLuint *vertex_indices;
int num_verts;

//for timer
float gTotalTimeElapsed = 0;
int gTotalFrames = 0;
GLuint gTimer;
float fps;

//for obj reader
struct Triangle {unsigned int indices[3];};
struct Vector3 {float x, y, z;};
std::vector<Vector3> gPositions;
std::vector<Vector3> gNormals;
std::vector<Triangle> gTriangles;

void load_mesh(std::string fileName);
void init_timer();
void start_timing();
float stop_timing();
void transformSetup();
void init(std::string path);
void drawRoom();
void draw();
void tokenize(char* string, std::vector<std::string>& tokens, const char* delimiter);
int face_index(const char* string);
int trackColor( Mat imgLines, int *range_r, int *range_g, int *range_b, int *range_o);
int pose();
int getCoords(Mat imgHSV, v2 *color, int* range);

int main(int argc, char **argv) {
    
    /*
     * Initialize the GUI Window
     */
    counter = 0;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(50, 100);
    glutInitWindowSize(1024, 1024);
    glutCreateWindow("Bunny");
        
    if (argc == 3){
        //buffered render
        buffered_render = true;
        std::cout << "Rendering with Vertex Arrays." << std::endl;
        init(argv[1]);
    } else if (argc == 2) {
        //non-buffered render
        buffered_render = false;
        std::cout << "Rendering in Immediate mode." << std::endl;
        init(argv[1]);
    } else {
        //bad args
        std::cout << "Usage PA3 /path/to/mesh.obj [-buffered]" << std::endl;
        exit(1);
    }
    
    glutDisplayFunc(draw);
    // glutPassiveMotionFunc(transformSetup);
    init_timer();
    glutMainLoop();
    return 0;
}

void init(std::string path) {

    /*
     * Do CV init
     */
    red = new v2();
    green = new v2();
    blue = new v2();
    orange = new v2();
    midp = new v2();
    lastmid = new v2();
    xz_midpoint = new v2();
    yz_midpoint = new v2();
    xy_midpoint = new v2();

    // rotation_m= new float[] {1,0,0,1,   0,1,0,1,  0,0,1,1,  1,1,1,1 };

    cap = VideoCapture(0); //capture the video from webcam
    rotation_m = new float[16];

    if ( !cap.isOpened() )  // if not success, exit program
    {
    cout << "Cannot open the web cam" << endl;
    // return -1;
    }

    //Capture a temporary image from the camera
    cap.read(imgTmp); 

    //Create a black image with the size as the camera output
    imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );

    /*
     * Nobody really knows what this does.
     * Vodoo magic
     */
    
    glewInit();
    
    /* 
     * Load the mesh into memory
     */
    
    load_mesh(path);
    

    /*
     * Set up buffers IF command line args 
     * This magnificent trash can be fixed by looking at the code on:
     * http://docs.gl/gl3/glDrawElements
     *
     * Currently, there are gVerts, gNorms, and vertex_indices
     */
     
    if (buffered_render){
    
        // Step 1 : Create the VAO

        GLuint vao;
        GLuint vert_buff;
        GLuint norm_buff;
        GLuint eab;

        glGenBuffers(1, &vert_buff);
        glGenBuffers(1, &norm_buff);
        
        // Step 2 :  Bind the normal and vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, vert_buff);
        glBufferData(GL_ARRAY_BUFFER, 
            sizeof(float)*3*gPositions.size(), 
            gVerts, 
            GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, norm_buff);
        glBufferData(GL_ARRAY_BUFFER, 
            sizeof(float)*3*gNormals.size(), 
            gNorms, 
            GL_STATIC_DRAW);

        // Setp 3 : Gen and Bind the VAO

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Step 4: Enable the vertex array buffers.

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        // Step 5 : Set pointers.  
        // We have to rebind so GL knows what we're talking about

        glBindBuffer(GL_ARRAY_BUFFER, vert_buff);
        glVertexPointer(3, GL_FLOAT, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, norm_buff);
        glNormalPointer(GL_FLOAT, 0, 0);

        // Step 6 : Set up element array buffer

        glGenBuffers(1, &eab);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
            sizeof(GLuint)*num_verts, 
            vertex_indices, 
            GL_STATIC_DRAW);

        glBindVertexArray(vao);
         
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

    }
    
    /*
     * Set up Transform pipeline
     * 
     */ 

    transformSetup();
    
    /*
     * Set up light parameters.
     * 
     */
     
    GLfloat Ia[] = { .2, .2, .2, 0};
    GLfloat light_pos[] = { 0, 2, 0, 1 };
    GLfloat la[] = {0,0,0}; //ambient
    GLfloat ld[] = {1,1,1}; //diffuse
    GLfloat ls[] = {0,0,0}; //sepcular
    
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Ia);
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, la);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, ld);
    glLightfv(GL_LIGHT0, GL_SPECULAR,ls);

    /*
     * Set up Material Parameters
     * 
     */

    GLfloat ka[] = { .8, .8, .8}; //ambient
    GLfloat kd[] = { .5, .5, .5}; //specular
    GLfloat ks[] = { .2, .2, .2}; //diffuse
    GLfloat p = 0;

    glMaterialfv(GL_FRONT, GL_AMBIENT, ka);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, kd);
    glMaterialfv(GL_FRONT, GL_SPECULAR, ks);
    glMaterialf(GL_FRONT, GL_SHININESS, p);
    
    //glShadeModel(GL_FLAT); //Uncomment for flat shading
    
}

void transformSetup() {
    
    /*
     * Set up Model Transform
     */

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0);
    glTranslatef(0,-1,-1.5);
    glScalef(10.0,10.0,10.0);
    glMultMatrixf(rotation_m);

    /*
     * Set up Projection Transform
     */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-.1, .1, -.1, .1, .1, 1000);
    glViewport(0, 0, 1024, 1024);
}
void drawRoom(){

    /*
     * Code for doing the regular implicit declaration of object coordinates
     */
    
    if (!buffered_render){

        int k0, k1, k2;
        Vector3 v0, v1, v2;
        Vector3 n0, n1, n2;

        glBegin(GL_TRIANGLES);
        
        for (unsigned int i = 0 ; i < gTriangles.size() ; i++ ){

            k0 = vertex_indices[3*i + 0];
            k1 = vertex_indices[3*i + 1];
            k2 = vertex_indices[3*i + 2];

            v0 = gPositions[k0];
            v1 = gPositions[k1];
            v2 = gPositions[k2];
            
            n0 = gNormals[k0];
            n1 = gNormals[k1];
            n2 = gNormals[k2];

            glNormal3f(n0.x, n0.y, n0.z);
            glVertex3f(v0.x, v0.y, v0.z);

            glNormal3f(n1.x, n1.y, n1.z);
            glVertex3f(v1.x, v1.y, v1.z);

            glNormal3f(n2.x, n2.y, n2.z);
            glVertex3f(v2.x, v2.y, v2.z);
        }

        glEnd();
    }
    
    /*
     * Code for doing glDrawElements
     */

    else {
        glDrawElements(GL_TRIANGLES , num_verts , GL_UNSIGNED_INT , 0);
        glutSolidCube(.1);
    }
}

void draw() {
    
    /*
     * Clear depth buffer
     * enable depth test, lighting, light0, normailze
     * 
     */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearDepth(1000);

    //do the enables
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    
    //Start timer ever frame
    start_timing();

    //Recompute the transform
    Mat imgLines = Mat::zeros( imgTmp.size(), CV_8UC3 );;
    trackColor( imgLines, red_range, green_range, blue_range, orange_range);
    pose();
    transformSetup();
    counter++;

    //Do the draw
    drawRoom();
    
    float timeElapsed = stop_timing();
    gTotalFrames++;
    gTotalTimeElapsed += timeElapsed;
    fps = gTotalFrames / gTotalTimeElapsed;
    char string[1024] = {0};
    sprintf(string, "OpenGL Bunny: %0.2f FPS", fps);
    glutSetWindowTitle(string);

    //Reload the scene
    glutPostRedisplay();
    glutSwapBuffers();
}

void init_timer()
{
    glGenQueries(1, &gTimer);
}

void start_timing()
{
    glBeginQuery(GL_TIME_ELAPSED, gTimer);
}

float stop_timing()
{
    glEndQuery(GL_TIME_ELAPSED);

    GLint available = GL_FALSE;
    while (available == GL_FALSE)
        glGetQueryObjectiv(gTimer, GL_QUERY_RESULT_AVAILABLE, &available);

    GLint result;
    glGetQueryObjectiv(gTimer, GL_QUERY_RESULT, &result);

    float timeElapsed = result / (1000.0f * 1000.0f * 1000.0f);
    return timeElapsed;
}
 
/*
 * Code for doing the mesh loading
 * Should move this to another file, 
 * but i probably wont
 */ 

void tokenize(char* string, std::vector<std::string>& tokens, const char* delimiter)
{
    char* token = strtok(string, delimiter);
    while (token != NULL)
    {
        tokens.push_back(std::string(token));
        token = strtok(NULL, delimiter);
    }
}

int face_index(const char* string)
{
    int length = strlen(string);
    char* copy = new char[length + 1];
    memset(copy, 0, length+1);
    strcpy(copy, string);

    std::vector<std::string> tokens;
    tokenize(copy, tokens, "/");
    delete[] copy;
    if (tokens.front().length() > 0 && tokens.back().length() > 0 && atoi(tokens.front().c_str()) == atoi(tokens.back().c_str()))
    {
        return atoi(tokens.front().c_str());
    }
    else
    {
        printf("ERROR: Bad face specifier!\n");
        exit(0);
    }
}

void load_mesh(std::string fileName)
{
    std::ifstream fin(fileName.c_str());
    if (!fin.is_open())
    {
        printf("ERROR: Unable to load mesh from %s!\n", fileName.c_str());
        exit(0);
    }

    float xmin = FLT_MAX;
    float xmax = -FLT_MAX;
    float ymin = FLT_MAX;
    float ymax = -FLT_MAX;
    float zmin = FLT_MAX;
    float zmax = -FLT_MAX;

    while (true)
    {
        char line[1024] = {0};
        fin.getline(line, 1024);

        if (fin.eof())
            break;

        if (strlen(line) <= 1)
            continue;

        std::vector<std::string> tokens;
        tokenize(line, tokens, " ");

        if (tokens[0] == "v")
        {
            float x = atof(tokens[1].c_str());
            float y = atof(tokens[2].c_str());
            float z = atof(tokens[3].c_str());

            xmin = std::min(x, xmin);
            xmax = std::max(x, xmax);
            ymin = std::min(y, ymin);
            ymax = std::max(y, ymax);
            zmin = std::min(z, zmin);
            zmax = std::max(z, zmax);

            Vector3 position = {x, y, z};
            gPositions.push_back(position);
        }
        else if (tokens[0] == "vn")
        {
            float x = atof(tokens[1].c_str());
            float y = atof(tokens[2].c_str());
            float z = atof(tokens[3].c_str());
            Vector3 normal = {x, y, z};
            gNormals.push_back(normal);
        }
        else if (tokens[0] == "f")
        {
            unsigned int a = face_index(tokens[1].c_str());
            unsigned int b = face_index(tokens[2].c_str());
            unsigned int c = face_index(tokens[3].c_str());
            Triangle triangle;
            triangle.indices[0] = a - 1;
            triangle.indices[1] = b - 1;
            triangle.indices[2] = c - 1;
            gTriangles.push_back(triangle);
        }
    }
    
    /*
     * Pack vertexes into the
     * 
     */

    num_verts = gTriangles.size()*3;
    vertex_indices = new GLuint[num_verts];
    gNorms = new float[gNormals.size()*3];
    gVerts = new float[gPositions.size()*3];

    for (unsigned int i = 0; i < gTriangles.size(); i++){
        vertex_indices[3*i+0] = gTriangles[i].indices[0];
        vertex_indices[3*i+1] = gTriangles[i].indices[1];
        vertex_indices[3*i+2] = gTriangles[i].indices[2];
    }
    for (unsigned int i = 0; i < gNormals.size(); i++){
        gNorms[3*i+0] = gNormals[i].x;
        gNorms[3*i+1] = gNormals[i].y;
        gNorms[3*i+2] = gNormals[i].z;
    }
    for (unsigned int i = 0; i < gPositions.size(); i++){
        gVerts[3*i+0] = gPositions[i].x;
        gVerts[3*i+1] = gPositions[i].y;
        gVerts[3*i+2] = gPositions[i].z;  
    }
    
    fin.close();

//    printf("Loaded mesh from %s. (%lu vertices, %lu normals, %lu triangles)\n", fileName.c_str(), gPositions.size(), gNormals.size(), gTriangles.size());
//    printf("Mesh bounding box is: (%0.4f, %0.4f, %0.4f) to (%0.4f, %0.4f, %0.4f)\n", xmin, ymin, zmin, xmax, ymax, zmax);
}

/*
 * Tracking Loop
 */

int pose(){
  // float wandSize s= 10;
  std::vector<CvPoint3D32f> modelPoints;
  modelPoints.push_back(cvPoint3D32f(0.0f, 0.0f, 0.0f));
  modelPoints.push_back(cvPoint3D32f(0.0f, 550.0f, 0.0f));
  modelPoints.push_back(cvPoint3D32f(500.0f, 0.0f, 0.0f)); //red
  modelPoints.push_back(cvPoint3D32f(-500.0f, 0.0f, 0.0f)); //green
  modelPoints.push_back(cvPoint3D32f(0.0f, 0.0f, -800.0f));
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
    // color->x = 0;
    // color->y = 0;
    // dot not found.
    // assume dot is in the same spot as purple
    color->x = blue->x;
    color->y = blue->y;
  }
  // imshow("Thresholded Image", imgThresholded); //show the thresholded image
  return 0;
}

int trackColor(Mat imgLines, int *range_r, int *range_g, int *range_b, int *range_o){

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