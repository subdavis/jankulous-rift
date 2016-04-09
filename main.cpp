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
void transformSetup(int x, int y);
void init(std::string path);
void drawRoom();
void draw();
void tokenize(char* string, std::vector<std::string>& tokens, const char* delimiter);
int face_index(const char* string);

int main(int argc, char **argv) {
    
    /*
     * Initialize the GUI Window
     */

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(50, 100);
    glutInitWindowSize(1500, 800);
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
    glutPassiveMotionFunc(transformSetup);
    init_timer();
    glutMainLoop();
    return 0;
}

void init(std::string path) {

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

    transformSetup(0, 0);
    
    /*
     * Set up light parameters.
     * 
     */
     
    GLfloat Ia[] = { .2, .2, .2, 1 };
    GLfloat light_pos[] = { 1, 1, 1, 0 };
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

    GLfloat ka[] = { 1, 1, 1}; //ambient
    GLfloat kd[] = { 1, 1, 1}; //specular
    GLfloat ks[] = { 0, 0, 0}; //diffuse
    GLfloat p = 0;

    glMaterialfv(GL_FRONT, GL_AMBIENT, ka);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, kd);
    glMaterialfv(GL_FRONT, GL_SPECULAR, ks);
    glMaterialf(GL_FRONT, GL_SHININESS, p);
    
    //glShadeModel(GL_FLAT); //Uncomment for flat shading
    
}

void transformSetup(int x, int y) {
    float centerFrustumX = -((x - 750) / 750.) * .2;
    float centerFrustumY = ((y - 400) / 400.) * .1;
    
    float scale = -5;
    
    /*
     * Set up Model Transform
     */
     std::cout << x << " " << y << " " << std::endl;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(-scale * centerFrustumX,-scale * centerFrustumY,0.0,0.0,0.0,-1.0,0.0,1.0,0.0);
    glTranslatef(0.1,-1.0,-1.5);
    glScalef(5.0,5.0,5.0);

    /*
     * Set up Projection Transform
     */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-.2 + centerFrustumX, .2 + centerFrustumX, -.1 + centerFrustumY, .1 + centerFrustumY, .2, 1000);
    glViewport(0, 0, 1500, 800);
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
