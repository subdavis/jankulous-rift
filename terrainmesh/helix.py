# Pygame/PyopenGL example by Bastiaan Zapf, Apr 2009
#
# Draw an helix, wiggle it pleasantly
#
# Keywords: Alpha Blending, Textures, Animation, Double Buffer

from OpenGL.GL import *
from OpenGL.GLU import *

from math import * # trigonometry

import pygame # just to get a display

from PIL import Image

# get an OpenGL surface

pygame.init() 
pygame.display.set_mode((800,600), pygame.OPENGL|pygame.DOUBLEBUF)

# How to catch errors here?

done = False

t=0
texture=glGenTextures( 1 )

glBindTexture( GL_TEXTURE_2D, texture );
glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

# set sane defaults for a plethora of potentially uninitialized
# variables
"""
glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                 GL_REPEAT);
glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                 GL_REPEAT );
"""
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)


pulse = 0
"""
texdata=[[[0.0,0,1,1],
          [0.0,0,0,0],
          [0.0,1,0,1],
          [0.0,0,0,0]],
         [[0.0,0,0,0],
          [pulse,pulse,pulse,1],
          [pulse,pulse,pulse,1],
          [0.0,0,0,0]],
         [[0.0,1,0,1],
          [1,pulse,pulse,1],
          [pulse,pulse,0,1],
          [0.0,0,0,0]],
         [[0.0,0,0,0],
          [0.0,0,0,0],
          [0.0,0,0,0],
          [0.0,0,0,0]]];

glTexImage2Df(GL_TEXTURE_2D, 0,4,0,GL_RGBA,
              texdata)
"""
im = Image.open("../skyboxsun5deg.png")
print im
try: 
    ix, iy, image = im.size[0], im.size[1], im.tostring("raw", "RGBA", 0, -1) 
except SystemError: 
    print "whoops"
    ix, iy, image = im.size[0], im.size[1], im.tobytes("raw", "RGBX", 0, -1)

glPixelStorei(GL_UNPACK_ALIGNMENT,1)

glTexImage2D(GL_TEXTURE_2D, 0, 3, ix, iy, 0, GL_RGBA, GL_UNSIGNED_BYTE, image)

while not done:

    t=t+1
    
    # for fun comment out these two lines

    glClearColor(0.0, 0.0, 0.0, 1.0)
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)

    # Get a perspective at the helix

    glMatrixMode(GL_PROJECTION);    

    glLoadIdentity()
    gluPerspective(90,1,0.01,1000)
    gluLookAt(sin(t/200.0)*3,sin(t/500.0)*3,cos(t/200.0)*3,0,0,0,0,1,0)

    # Draw the helix (this ought to be a display list call)

    glMatrixMode(GL_MODELVIEW)

    # get a texture (this ought not to be inside the inner loop)

    

    # a texture

    #pulse = sin(t/30)*0.5+0.5 # try this one
    

    #glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE); # XXX Why GL_ONE?
# alternatively:
#    glEnable(GL_DEPTH_TEST);

    glEnable( GL_TEXTURE_2D );
    # use the texture
    glBindTexture( GL_TEXTURE_2D, texture );

    # vertices & texture data

    glBegin(GL_TRIANGLE_STRIP);

    for i in range(0,100):

        r=5.0 # try other values - integers as well
        d=1   # try other values

        if (i%2==0):
            glTexCoord2f(0,i);
            glVertex3f( cos(i/r), -2.5+i*0.05, sin(i/r));            
#            glVertex3f( cos(i/r)*pulse2, -2.5+i*0.05, sin(i/r)*pulse2);
        else:
            glTexCoord2f(1,i);
            glVertex3f( cos(i/r+3.14), -2.5+i*0.05+d, sin(i/r+3.14));
#            glVertex3f( cos(i/r+3.14)*pulse2, -2.5+i*0.05+d+pulse2*1, sin(i/r+3.14)*pulse2);
        

    glEnd();

    glFlush()

    pygame.display.flip()
