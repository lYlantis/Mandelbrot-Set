// Calculate and display the Mandelbrot set
// ECE4893/8893 final project, Fall 2011

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <GL/glut.h>
#include <GL/glext.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "complex.h"

using namespace std;

const int width=512;
const int height=512;


int MBArr[512][512];			//Array to store mandelbrot set pixel information
int IterArr[512][512];			//Array to store the number of iterations it took to find if pixel was not in set
Complex CompVal[512][512];		//Array to store the c values for the zoom function
int numThreads = 16;			//How many pthreads we will create
double *r, *g, *b;				
double W, H;
float dx,dy,diff;

// Min and max complex plane values
Complex  minC(-2.0, -1.2);
Complex  maxC( 1.0, 1.8);
int      maxIt = 2000;     // Max iterations for the set computations

//Mouse
bool mouseClicked;
struct Position
{
	Position() : x(0), y(0) {}
	float x, y;
};
Position start, finish;

void* mandelbrotCalculate(void* v) {
	W=512, H=512;
	//Values to interpolate pixels to Open GL coordinates
	double Real_Fact = (maxC.real - minC.real) / (W-1);
	double Imag_Fact = (maxC.imag - minC.imag) / (H-1);
	Complex factors = Complex(Real_Fact,Imag_Fact);
	
	int N;
	
	//Information needed to make function threadable
	unsigned long myId = (unsigned long)v;
	int startRow = myId * H/numThreads;
	int endRow = startRow + H/numThreads;
	
	for(int y=startRow; y < endRow; y++) {
		double c_imag = maxC.imag - y*factors.imag;
		for(int x = 0; x < W; x++){
			double c_real = minC.real + x*factors.real;
			Complex c = Complex(c_real, c_imag);
			Complex z = Complex(c_real, c_imag);
			bool inside = true;
			CompVal[x][y].real = c.real;
			CompVal[x][y].imag = c.imag;
			for(int n=0; n < maxIt; n++) {
				double z_real_sq = z.real*z.real;
				double z_imag_sq = z.imag*z.imag;
				Complex z_sq = Complex(z_real_sq, z_imag_sq);
				
				if(z_sq.real + z_sq.imag >4){
					inside = false;
					N=n;
					break;
				}
				z.imag = 2*z.real*z.imag + c.imag;
				z.real = z_sq.real - z_sq.imag + c.real;		
			}
			
			if(inside==true){
				//Dont need to save color as inside = black
				MBArr[x][y]=1;
			} else {
				//Save pixel as not in set and how many iterations were needed to find this
				MBArr[x][y]=0;
				IterArr[x][y]=N;
			}
		}
	}
	return 0;
}


void mandelbrotDisplay(const int W, const int H) {		
	
	glPointSize(1);
	glBegin(GL_POINTS);
	
	for(int x=0; x < W; x++) {
		for(int  y=0; y < H; y++){
			if(MBArr[x][y]==1) {
				glColor3f(0.0, 0.0, 0.0);
				glVertex2i(x,y);
			} else {
				glColor3f(r[IterArr[x][y]], g[IterArr[x][y]], b[IterArr[x][y]]);
				glVertex2i(x,y);
			} 
		}
	}
	glEnd();
}

void* genColorArr(void* v){
	//Generate a random array of 2000 rgb values so the color of each pixel is based on the iteration
	srand48(time(0));
	
	unsigned long myId = (unsigned long)v;
	int startPos = myId * maxIt/numThreads;
	int endPos = startPos + maxIt/numThreads;
	
	for(int i=startPos; i<endPos; i++){
		r[i] = drand48();
		g[i] = drand48();
		b[i] = drand48();
	}	
	return 0;
}

void drawSquare(){
	glColor3d(1.0, 0.0, 0.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBegin(GL_POLYGON);
	  glVertex3f(start.x, start.y, 0);
	  glVertex3f(finish.x, start.y,0);
	  glVertex3f(finish.x, finish.y,0);
	  glVertex3f(start.x, finish.y,0);
	glEnd();
}

void display(void)
{ // Your OpenGL display code here
	glClearColor(1.0,1.0,1.0,1.0); 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LINE_SMOOTH | GL_DEPTH_TEST);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double W = glutGet(GLUT_WINDOW_WIDTH);
	double H = glutGet(GLUT_WINDOW_HEIGHT);
	glOrtho(0, W, H, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	mandelbrotDisplay(W, H);
	
	//Draws Red Box around selection
	if(mouseClicked) {
		drawSquare();
	}
	
	glutSwapBuffers();		//for double buffering 
}

void init()
{ // Your OpenGL initialization code here
  gluLookAt(0.0, 0.0, 1.0, 0.0, 0.5, 0.0, 0.0, 1.0, 0.0);
}

void mouse(int button, int state, int x, int y)
{ // Your mouse click processing here
  // state == 0 means pressed, state != 0 means released
  // Note that the x and y coordinates passed in are in
  // PIXELS, with y = 0 at the top.
	
	//When button is pressed for the first time
	if(button==GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		mouseClicked=true;
		start.x = finish.x = x;
		start.y = finish.y = y;
	}
	
	//When button is released
	if(button==GLUT_LEFT_BUTTON && state == GLUT_UP) {
		mouseClicked=false;
		finish.x = x;
		finish.y = y;

		dx = abs(x - start.x);
		dy = abs(y - start.y );
		
		if (dx>=dy) {
			diff = dy;
		} else if (dy>dx) {
			diff = dx;
		}
		
		if(x>start.x && y>start.y){
		 finish.x = start.x + diff;
	     finish.y = start.y + diff;
		
	     minC = CompVal[(int)start.x][(int)finish.y];
	     maxC = CompVal[(int)finish.x][(int)start.y]; 
		}  
	    
		else if(x<start.x && y>start.y) {
			finish.x = start.x - diff;
			finish.y = start.y + diff;
			
	        maxC = CompVal[(int)start.x][(int)start.y];
	        minC = CompVal[(int)finish.x][(int)finish.y];
	      }
	     
		else if(x>start.x&& y<start.y) {
		 finish.x = start.x + diff;
		 finish.y = start.y - diff;
		 
		 minC = CompVal[(int)start.x][(int)start.y];
		 maxC = CompVal[(int)finish.x][(int)finish.y];
		}
	     
		else if(x<start.x && y<start.y) {
		 finish.x = start.x - diff;
		 finish.y = start.y - diff;
		 
		 minC = CompVal[(int)finish.x][(int)start.y];
		 maxC = CompVal[(int)start.x][(int)finish.y];
		}
		
		//Recalculate the mandelbrot set with the new min and max values
		for(int i=0; i<numThreads;++i) {
		  pthread_t pt;
		  pthread_create(&pt, 0, mandelbrotCalculate, (void *)i );
		}
		//Redraw the zoomed in region
		mandelbrotDisplay(W,H);
	}
	glutPostRedisplay();
}

void motion(int x, int y)
{ // Your mouse motion here, x and y coordinates are as above
	finish.x = x;
	finish.y = y;
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
  r = new double[maxIt];
  g = new double[maxIt];
  b = new double[maxIt];
  
  //Have threads calculate random colors
  for(int i=0; i<numThreads; i++) {
	  pthread_t pt;
	  pthread_create(&pt, 0, genColorArr, (void *)i );
  }

  //Have threads calculate the manelbrot set
  for(int i=0; i<numThreads;++i) {
  	  pthread_t pt;
  	  pthread_create(&pt, 0, mandelbrotCalculate, (void *)i );
    }
  
  // Initialize OpenGL, but only on the "master" thread or process.
  // See the assignment writeup to determine which is "master" 
  // and which is slave.
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE); //| GLUT_RGB
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glutInitWindowSize(512,512);
  glutInitWindowPosition(900,100);
  glutCreateWindow("Mandlebrot Set");
  
  //initialize values
  init();

  // Set your glut callbacks here
  glutMouseFunc(mouse);
  glutMotionFunc(motion);  
  glutDisplayFunc(display);
  glutIdleFunc(display);
  
  // Enter the glut main loop here
  glutMainLoop();	
  return 0;
}

