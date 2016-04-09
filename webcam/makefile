CC=g++
CFLAGS=-Wall -O3
LINKER=-lGL -lglut -lGLU -lGLEW -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_stitching 
OUTFILE=webcam

all:
	$(CC) $(CFLAGS) webcam.cpp -o  $(OUTFILE) $(LINKER)

clean:
	rm $(OUTFILE)