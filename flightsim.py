import OpenGL.GL as GL
import OpenGL.GLUT as GLUT
import OpenGL.GLU as GLU

from threading import Thread

from OpenGL.arrays import vbo

import OpenGL.arrays as arrays

import sys

import numpy as np
import scipy.linalg
import scipy.interpolate

import operator
import ctypes

import time
print "type y to use VBO, n to use immediate rendering"

from PIL import Image
GLUT.glutInit(sys.argv)

GLUT.glutInitWindowSize(1512,800)


GLUT.glutCreateWindow("BUNNY")


GLUT.glutInitDisplayMode(GLUT.GLUT_RGBA)
def mulmat3(m):
   GL.glMultMatrixf([m[0, 0], m[1, 0], m[2, 0], 0, 
                   m[0, 1], m[1, 1], m[2, 1], 0, 
                   m[0, 2], m[1, 2], m[2, 2], 0, 
                   0,      0,          0,      1])

useVBO = True

position = [0.1, -27, -1.5]
 
orientation = np.matrix([[1, 0, 0], [0, 1, 0], [0, 0, 1]], dtype = np.float)
rotator = np.matrix([[1, 0, 0], [0, 1, 0], [0, 0, 1]], dtype = np.float)
#load terrain for collisions
DIM = 500
heightmap = Image.open("terrainmesh/gc_dem.tif")

heightmap = np.array(heightmap, dtype = np.float)

heightmap = heightmap[1500:3500:4,500:2500:4] / 10000


x = np.linspace(-.2, .2, DIM)
z = np.linspace(-.2, .2, DIM)
#x, z = np.meshgrid(x, z)

x = x * 1000
z = z * 1000
heightmap = heightmap * 100

heightfunc = scipy.interpolate.RectBivariateSpline(x, z, heightmap, kx = 1, ky = 1)


#load skybox


im = Image.open("skyboxsun5deg.png")

print im
try: 
    ix, iy, image = im.size[0], im.size[1], im.tostring("raw", "RGBA", 0, -1) 
except SystemError: 
    print "whoops"
    ix, iy, image = im.size[0], im.size[1], im.tobytes("raw", "RGBX", 0, -1)



skytexID = GL.glGenTextures(1)

GL.glBindTexture(GL.GL_TEXTURE_2D, skytexID) 
GL.glTexEnvf( GL.GL_TEXTURE_ENV, GL.GL_TEXTURE_ENV_MODE, GL.GL_MODULATE );

GL.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_LINEAR)
GL.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_LINEAR)

GL.glPixelStorei(GL.GL_UNPACK_ALIGNMENT,1)

GL.glTexImage2D( GL.GL_TEXTURE_2D, 0, 3, ix, iy, 0, GL.GL_RGBA, GL.GL_UNSIGNED_BYTE, image)

# position teapots
teapots = []
def makeTea():
    global teapots
    teapots = []
    for _ in range(6):
        x = 1000 * (np.random.rand() * .4 - .2)
        z = 1000 * (np.random.rand() * .4 - .2)
        y = heightfunc(z, x) + 2
        teapots += [[x, y, z, False]]
makeTea()




#load terrain
 
def interpretLine(string):
    return map(float, string.split()[1:])
    
with open("terrainmesh/terrain.obj", "r") as file:
    for _ in range(7):
        file.readline()

    numverts =  int(file.readline().split()[-1])
    numtris =  int(file.readline().split()[-1])
    
    file.readline()
    file.readline()

    verts = [[69, 69, 69]]
    norms = [[69, 69, 69]]
    tris = []

    for _ in range(numverts):
        norms += [interpretLine(file.readline())]
        verts += [interpretLine(file.readline())]
    file.readline()
    file.readline()
    for _ in range(numtris):
        tris += [[int(a.split("//")[0]) for a in file.readline().split()[1:]]]
        
        
        



GL.glEnable(GL.GL_DEPTH_TEST)

GL.glEnable(GL.GL_LIGHTING)
GL.glEnable(GL.GL_LIGHT0)
GL.glLightfv(GL.GL_LIGHT0, GL.GL_POSITION, [1, 1, 1, 0])

GL.glLightfv(GL.GL_LIGHT0, GL.GL_AMBIENT, [.2, .2, .2, 1])

GL.glLightfv(GL.GL_LIGHT0, GL.GL_DIFFUSE, [1, 1, 1, 1])
GL.glEnable(GL.GL_NORMALIZE)


#GL.glEnable(GL.GL_CULL_FACE)
#GL.glCullFace(GL.GL_BACK)



vertsa = np.array(verts, dtype=np.float32)
trisa = np.array(tris, dtype = np.int32).flatten()
vertexVBO = vbo.VBO(vertsa)

normsa = np.array(norms, dtype=np.float32)
normVBO = vbo.VBO(normsa)
triBuffer = vbo.VBO(trisa, target=GL.GL_ELEMENT_ARRAY_BUFFER)
angle = 0
def draw():
   global orientation
   GL.glEnable(GL.GL_DEPTH_TEST)
   GL.glDepthFunc(GL.GL_LEQUAL)

   GL.glMatrixMode(GL.GL_PROJECTION)
   GL.glLoadIdentity()
   
   #GLU.gluLookAt(position[0], position[1], position[2], 
   #              position[0]  , position[1], position[2]  -  1,
   #              0, 1, 0)
   GLU.gluLookAt(0,0,0,0,0,-  1,
                 0, 1, 0)
   GL.glFrustum(-.2, .2, -.1, .1, .1, 1000)
   GL.glViewport(0, 0, 1512, 800)
   
   GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)
      

   
   
   GL.glMatrixMode(GL.GL_MODELVIEW)
   GL.glLoadIdentity()
   
   
   mulmat3(orientation)
   
   GL.glTranslatef(position[0], position[1], position[2])
   
   #draw sky
   GL.glEnable(GL.GL_TEXTURE_2D) 
   GL.glBindTexture(GL.GL_TEXTURE_2D, skytexID) 
   #GL.glDisable( GL.GL_LIGHTING)
   GL.glBlendFunc (GL.GL_SRC_ALPHA, GL.GL_ONE)
   GL.glEnable(GL.GL_TEXTURE_2D) 
   GL.glTexParameterf(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER, GL.GL_NEAREST)
   GL.glTexParameterf(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER, GL.GL_NEAREST) 
   GL.glTexEnvf(GL.GL_TEXTURE_ENV, GL.GL_TEXTURE_ENV_MODE, GL.GL_DECAL)
   
   
   
   d = 1000
   
   GL.glPushMatrix()
   GL.glScalef(-500, 500, 500)
   GL.glBegin(GL.GL_QUADS)
   GL.glTexCoord2f(0.0, 0.0)
   GL.glVertex3f(-1.0, -1.0, 1.0)
   GL.glTexCoord2f(1.0, 0.0)
   GL.glVertex3f( 1.0, -1.0, 1.0)
   GL.glTexCoord2f(1.0, 1.0)
   GL.glVertex3f( 1.0, 1.0, 1.0)
   GL.glTexCoord2f(0.0, 1.0)
   GL.glVertex3f(-1.0, 1.0, 1.0)
   GL.glTexCoord2f(1.0, 0.0)
   GL.glVertex3f(-1.0, -1.0, -1.0)
   GL.glTexCoord2f(1.0, 1.0)
   GL.glVertex3f(-1.0, 1.0, -1.0)
   GL.glTexCoord2f(0.0, 1.0)
   GL.glVertex3f( 1.0, 1.0, -1.0)
   GL.glTexCoord2f(0.0, 0.0)
   GL.glVertex3f( 1.0, -1.0, -1.0)
   GL.glTexCoord2f(0.0, 1.0)
   GL.glVertex3f(-1.0, 1.0, -1.0)
   GL.glTexCoord2f(0.0, 0.0)
   GL.glVertex3f(-1.0, 1.0, 1.0)
   GL.glTexCoord2f(1.0, 0.0)
   GL.glVertex3f( 1.0, 1.0, 1.0)
   GL.glTexCoord2f(1.0, 1.0)
   GL.glVertex3f( 1.0, 1.0, -1.0)
   GL.glTexCoord2f(1.0, 1.0)
   GL.glVertex3f(-1.0, -1.0, -1.0)
   GL.glTexCoord2f(0.0, 1.0)
   GL.glVertex3f( 1.0, -1.0, -1.0)
   GL.glTexCoord2f(0.0, 0.0)
   GL.glVertex3f( 1.0, -1.0, 1.0)
   GL.glTexCoord2f(1.0, 0.0)
   GL.glVertex3f(-1.0, -1.0, 1.0)
   GL.glTexCoord2f(1.0, 0.0)
   GL.glVertex3f( 1.0, -1.0, -1.0)
   GL.glTexCoord2f(1.0, 1.0)
   GL.glVertex3f( 1.0, 1.0, -1.0)
   GL.glTexCoord2f(0.0, 1.0)
   GL.glVertex3f( 1.0, 1.0, 1.0)
   GL.glTexCoord2f(0.0, 0.0)
   GL.glVertex3f( 1.0, -1.0, 1.0)
   GL.glTexCoord2f(0.0, 0.0)
   GL.glVertex3f(-1.0, -1.0, -1.0)
   GL.glTexCoord2f(1.0, 0.0)
   GL.glVertex3f(-1.0, -1.0, 1.0)
   GL.glTexCoord2f(1.0, 1.0)
   GL.glVertex3f(-1.0, 1.0, 1.0)
   GL.glTexCoord2f(0.0, 1.0)
   GL.glVertex3f(-1.0, 1.0, -1.0)
   GL.glEnd()
   GL.glEnable( GL.GL_LIGHTING)
   
   GL.glDisable(GL.GL_TEXTURE_2D) 
   
   
   GL.glPopMatrix()
   
   GL.glLightfv(GL.GL_LIGHT0, GL.GL_POSITION, [1, 1, 1, 0])
   
   
   for pot in teapots:
       
       GL.glPushMatrix()
       
       if pot[3]: #teapot is found
           GL.glMaterialfv(GL.GL_FRONT_AND_BACK,  GL.GL_AMBIENT,  [0, 1, 0, 1])
           GL.glTranslatef(pot[0], pot[1] + 5, pot[2])
           GLUT.glutSolidCube(5)
       else:
           GL.glMaterialfv(GL.GL_FRONT_AND_BACK,  GL.GL_AMBIENT,  [1, 0, 1, 1])
           GL.glTranslatef(pot[0], pot[1], pot[2])
           GLUT.glutSolidCube(2)
       GL.glPopMatrix()
   
   GL.glScalef(1000, 100, 1000)
   GL.glEnableClientState(GL.GL_VERTEX_ARRAY);
   vertexVBO.bind()
   # when GL_ARRAY_BUFFER is bound to vertex buffer
   triBuffer.bind()
   GL.glVertexPointer(3, GL.GL_FLOAT, 0, None);
   
   normVBO.bind()
   # when GL_ARRAY_BUFFER is bound to normal buffer
   GL.glEnableClientState(GL.GL_NORMAL_ARRAY);
   GL.glNormalPointer(GL.GL_FLOAT, 0, None);
   
   GL.glMaterialfv(GL.GL_FRONT_AND_BACK,  GL.GL_AMBIENT,  [.8, .8, .8, 1])
   if useVBO:
       GL.glDrawElements(GL.GL_TRIANGLES, numtris*3, GL.GL_UNSIGNED_INT, None)
   else:
       for tri in tris:
           GL.glColor3f(1, 1, 1)
           GL.glBegin(GL.GL_TRIANGLES)
           for vert in tri:
               norm = norms[vert]
               vert = verts[vert]
               GL.glNormal3f(norm[0], norm[1], norm[2])
               
               
               GL.glVertex3f(vert[0], vert[1], vert[2])
           GL.glEnd()
   
   
   GL.glFlush()
   
   
def mousemoved(x, y):
    
    global orientation
    if x != 500 or y != 500:
       xrot = (x-500.) / 1000
       yrot = (y - 500.) / 1000
      
       rotator = np.matrix([[np.cos(xrot), np.sin(xrot), 0],
                                 [-np.sin(xrot),np.cos(xrot), 0],
                                 [0,           0,             1]])
       rotator *= np.matrix([[1, 0, 0],
                            [0,np.cos(yrot), np.sin(yrot)],
                            [0,-np.sin(yrot),np.cos(yrot)]])
       
       orientation = rotator * orientation
       
       GLUT.glutWarpPointer(500, 500)

                            
GLUT.glutPassiveMotionFunc(mousemoved)
GLUT.glutDisplayFunc(draw)
print len(norms)

def update():
    global position, orientation
    t0 = time.time()
    orientation = (rotator**-1) * orientation
    go = np.array((orientation** -1) * np.matrix([[0], [0], [.2]])).flatten()
    
    position[0] += go[0] 
    position[1] += go[1]
    position[2] += go[2]
    draw()
    t1 = time.time()
    if heightfunc(-position[2], -position[0]) > -position[1]:
        print("dead")
        time.sleep(5)
        position = [0.1, -27, -1.5]
        makeTea()
 
        orientation = np.matrix([[1, 0, 0], [0, 1, 0], [0, 0, 1]], dtype = np.float)
    for i, pot in enumerate(teapots):
        if ((pot[0] + position[0])**2 + (pot[1] + position[1])**2 
            + (pot[2] + position[2])**2 < 3):
            teapots[i][3] = True
        
        
        
    #print position
    GLUT.glutSetWindowTitle(str(1 / (t1 - t0)))
    
    
def stdinControl():
    global orientation
    global rotator
    while True:
        try:
            inp = raw_input()
            #print "recieved " + inp
            if all(map(float, inp.split())):
                array = np.array(map(float, inp.split()))
                array = array.reshape(3, 3).transpose()
                matr = np.matrix(array)
                matr *= np.matrix([[1, 0, 0],
                                   [0,  1, 0],
                                   [0,  0, 1]])
                                  
                #print np.linalg.det(matr)
                if abs(np.linalg.det(matr) - 1)  <.09 or abs(np.linalg.det(matr) + 1)  <.09:
                    rotator =  matr
                #print matr
                
        except Exception as e:
            print "error reading: ", e
            print inp
        
t = Thread(target = stdinControl)
t.start()
GLUT.glutIdleFunc(update)
GLUT.glutMainLoop()

