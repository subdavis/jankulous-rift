import numpy as np
import sys
import linecache
def PrintException():
    exc_type, exc_obj, tb = sys.exc_info()
    f = tb.tb_frame
    lineno = tb.tb_lineno
    filename = f.f_code.co_filename
    linecache.checkcache(filename)
    line = linecache.getline(filename, lineno, f.f_globals)
    print 'EXCEPTION IN ({}, LINE {} "{}"): {}'.format(filename, lineno, line.strip(), exc_obj)

while True:
    try:
        inp = raw_input()
        #print "in: ", inp
        if all(map(float, inp.split())):
            array = np.array(map(float, inp.split()))
            array = array.reshape(3, 3).transpose()
            matr = np.matrix(array)
            thetax = np.arctan2(matr[2, 1], matr[2, 2])
            thetay = np.arctan2(-matr[2, 0], np.sqrt(matr[2, 1] ** 2 
                                                 + matr[2, 2] ** 2))
            thetaz = -np.arctan2(matr[1, 0], matr[0, 0])
            
            X = np.matrix([[1, 0, 0],
                           [0, np.cos(thetax), -np.sin(thetax)],
                           [0, np.sin(thetax),  np.cos(thetax)]])
                           
            Y = np.matrix([[np.cos(thetay), 0 ,np.sin(thetay)],
                           [0, 1, 0],
                           [-np.sin(thetay),0 , np.cos(thetay)]])
                           
            Z = np.matrix([[np.cos(thetaz), -np.sin(thetaz), 0],
                           [np.sin(thetaz),  np.cos(thetaz), 0],
                           [0,               0,              1]])
                           
            out = X * Y * Z
            
            print np.linalg.det(out)
            out = np.array(out.transpose())
            out = out.flatten()
            #print "out: "
            for f in out:
                print f,
            print ""
        else:
            print "no"
    except ValueError as e:
        PrintException()
        
        
