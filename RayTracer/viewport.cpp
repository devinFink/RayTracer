//-------------------------------------------------------------------------------
///
/// \file       viewport.cpp 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    1.0
/// \date       August 20, 2025
///
/// \brief Example source for CS 6620 - University of Utah.
///
/// Copyright (c) 2019 Cem Yuksel. All Rights Reserved.
///
/// This code is provided for educational use only. Redistribution, sharing, or 
/// sublicensing of this code or its derivatives is strictly prohibited.
///
//-------------------------------------------------------------------------------

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "scene.h"
#include "objects.h"
#include <stdlib.h>
#include <time.h>

#include <GL/freeglut.h>

//-------------------------------------------------------------------------------

void BeginRender( RenderScene *scene );	// Called to start rendering (renderer must run in a separate thread)
void StopRender ();	// Called to end rendering (if it is not already finished)

RenderScene *theScene = nullptr;

//-------------------------------------------------------------------------------

enum Mode {
	MODE_READY,			// Ready to render
	MODE_RENDERING,		// Rendering the image
	MODE_RENDER_DONE	// Rendering is finished
};

enum ViewMode
{
	VIEWMODE_OPENGL,
	VIEWMODE_IMAGE,
	VIEWMODE_Z,
};

enum MouseMode {
	MOUSEMODE_NONE,
	MOUSEMODE_DEBUG,
	MOUSEMODE_ROTATE,
};

static Mode		mode		= MODE_READY;		// Rendering mode
static ViewMode	viewMode	= VIEWMODE_OPENGL;	// Display mode
static MouseMode mouseMode	= MOUSEMODE_NONE;	// Mouse mode
static int		startTime;						// Start time of rendering
static int mouseX=0, mouseY=0;
static float viewAngle1=0, viewAngle2=0;
static GLuint viewTexture;

//-------------------------------------------------------------------------------

void GlutDisplay();
void GlutReshape(int w, int h);
void GlutIdle();
void GlutKeyboard(unsigned char key, int x, int y);
void GlutMouse(int button, int state, int x, int y);
void GlutMotion(int x, int y);

//-------------------------------------------------------------------------------

void ShowViewport( RenderScene *scene )
{
	theScene = scene;

#ifdef _WIN32
	SetProcessDPIAware();
#endif

	int argc = 1;
	char argstr[] = "raytrace";
	char *argv = argstr;
	glutInit(&argc,&argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
	if (glutGet(GLUT_SCREEN_WIDTH) > 0 && glutGet(GLUT_SCREEN_HEIGHT) > 0){
		glutInitWindowPosition( (glutGet(GLUT_SCREEN_WIDTH) - theScene->camera.imgWidth)/2, (glutGet(GLUT_SCREEN_HEIGHT) - theScene->camera.imgHeight)/2 );
	}
	else glutInitWindowPosition( 50, 50 );
	glutInitWindowSize(theScene->camera.imgWidth, theScene->camera.imgHeight);

	glutCreateWindow("Ray Tracer - CS 6620");
	glutDisplayFunc(GlutDisplay);
	glutReshapeFunc(GlutReshape);
	glutIdleFunc(GlutIdle);
	glutKeyboardFunc(GlutKeyboard);
	glutMouseFunc(GlutMouse);
	glutMotionFunc(GlutMotion);

	glClearColor(0,0,0,0);

	glPointSize(3.0);
	glEnable( GL_CULL_FACE );

	#define LIGHTAMBIENT 0.1f
	glEnable( GL_LIGHT0 );
	float lightamb[4] =  { LIGHTAMBIENT, LIGHTAMBIENT, LIGHTAMBIENT, 1.0f };
	glLightfv( GL_LIGHT0, GL_AMBIENT, lightamb );

	#define LIGHTDIF0 1.0f
	float lightdif0[4] = { LIGHTDIF0, LIGHTDIF0, LIGHTDIF0, 1.0f };
	glLightfv( GL_LIGHT0, GL_DIFFUSE, lightdif0 );
	glLightfv( GL_LIGHT0, GL_SPECULAR, lightdif0 );

	glEnable(GL_NORMALIZE);

	glLineWidth(2);

	glGenTextures(1,&viewTexture);
	glBindTexture(GL_TEXTURE_2D, viewTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glutMainLoop();
}

//-------------------------------------------------------------------------------

void GlutReshape( int w, int h )
{
	if( w != theScene->camera.imgWidth || h != theScene->camera.imgHeight ) {
		glutReshapeWindow( theScene->camera.imgWidth, theScene->camera.imgHeight );
	} else {
		glViewport( 0, 0, w, h );

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		float r = (float) w / float (h);
		gluPerspective( theScene->camera.fov, r, 0.02, 1000.0 );

		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
	}
}

//-------------------------------------------------------------------------------

void DrawNode( const Node *node )
{
	glPushMatrix();

	Matrix3f tm = node->GetTransform();
	Vec3f p = node->GetPosition();
	float m[16] = { tm[0],tm[1],tm[2],0, tm[3],tm[4],tm[5],0, tm[6],tm[7],tm[8],0, p.x,p.y,p.z,1 };
	glMultMatrixf( m );

	const Object *obj = node->GetNodeObj();
	if ( obj ) obj->ViewportDisplay();

	for ( int i=0; i<node->GetNumChild(); i++ ) {
		DrawNode( node->GetChild(i) );
	}

	glPopMatrix();
}

//-------------------------------------------------------------------------------

void DrawScene()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	glEnable( GL_LIGHTING );
	glEnable( GL_DEPTH_TEST );

	glPushMatrix();
	Vec3f p = theScene->camera.pos;
	Vec3f t = theScene->camera.pos + theScene->camera.dir;
	Vec3f u = theScene->camera.up;
	gluLookAt( p.x, p.y, p.z,  t.x, t.y, t.z,  u.x, u.y, u.z );

	glRotatef( viewAngle1, 1, 0, 0 );
	glRotatef( viewAngle2, 0, 0, 1 );

	DrawNode( &theScene->rootNode );

	glPopMatrix();

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );
}

//-------------------------------------------------------------------------------

void DrawProgressBar(float done)
{
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	glBegin(GL_LINES);
	glColor3f(1,1,1);
	glVertex2f(-1,-1);
	glVertex2f(done*2-1,-1);
	glColor3f(0,0,0);
	glVertex2f(done*2-1,-1);
	glVertex2f(1,-1);
	glEnd();

	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
}

//-------------------------------------------------------------------------------

void DrawImage( const void *data, GLenum type, GLenum format )
{
	glBindTexture(GL_TEXTURE_2D, viewTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, theScene->renderImage.GetWidth(), theScene->renderImage.GetHeight(), 0, format, type, data); 

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	glEnable(GL_TEXTURE_2D);

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	glColor3f(1,1,1);
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);
	glVertex2f(-1,-1);
	glTexCoord2f(1,1);
	glVertex2f(1,-1);
	glTexCoord2f(1,0);
	glVertex2f(1,1);
	glTexCoord2f(0,0);
	glVertex2f(-1,1);
	glEnd();

	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );

	glDisable(GL_TEXTURE_2D);
}

//-------------------------------------------------------------------------------

void DrawRenderProgressBar()
{
	int rp = theScene->renderImage.GetNumRenderedPixels();
	int np = theScene->renderImage.GetWidth() * theScene->renderImage.GetHeight();
	if ( rp >= np ) return;
	float done = (float) rp / (float) np;
	DrawProgressBar(done);
}

//-------------------------------------------------------------------------------

void GlutDisplay()
{
	switch ( viewMode ) {
	case VIEWMODE_OPENGL:
		DrawScene();
		break;
	case VIEWMODE_IMAGE:
		//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		//glDrawPixels( renderImage.GetWidth(), renderImage.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, renderImage.GetPixels() );
		DrawImage( theScene->renderImage.GetPixels(), GL_UNSIGNED_BYTE, GL_RGB );
		DrawRenderProgressBar();
		break;
	case VIEWMODE_Z:
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		if ( ! theScene->renderImage.GetZBufferImage() ) theScene->renderImage.ComputeZBufferImage();
		//glDrawPixels( renderImage.GetWidth(), renderImage.GetHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, renderImage.GetZBufferImage() );
		DrawImage( theScene->renderImage.GetZBufferImage(), GL_UNSIGNED_BYTE, GL_LUMINANCE );
		break;
	}

	glutSwapBuffers();
}

//-------------------------------------------------------------------------------

void GlutIdle()
{
	static int lastRenderedPixels = 0;
	if ( mode == MODE_RENDERING ) {
		int nrp = theScene->renderImage.GetNumRenderedPixels();
		if ( lastRenderedPixels != nrp ) {
			lastRenderedPixels = nrp;
			if ( theScene->renderImage.IsRenderDone() ) {
				mode = MODE_RENDER_DONE;
				int endTime = (int) time(nullptr);
				int t = endTime - startTime;
				int h = t / 3600;
				int m = (t % 3600) / 60;
				int s = t % 60;
				printf("\nRender time is %d:%02d:%02d.\n",h,m,s);
			}
			glutPostRedisplay();
		}
	}
}

//-------------------------------------------------------------------------------

void GlutKeyboard(unsigned char key, int x, int y)
{
	switch ( key ) {
	case 27:	// ESC
		exit(0);
		break;
	case ' ':
		switch ( mode ) {
		case MODE_READY: 
			mode = MODE_RENDERING;
			viewMode = VIEWMODE_IMAGE;
			DrawScene();
			glReadPixels( 0, 0, theScene->renderImage.GetWidth(), theScene->renderImage.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, theScene->renderImage.GetPixels() );
			{
				Color24 *c = theScene->renderImage.GetPixels();
				for ( int y0=0, y1=theScene->renderImage.GetHeight()-1; y0<y1; y0++, y1-- ) {
					int i0 = y0 * theScene->renderImage.GetWidth();
					int i1 = y1 * theScene->renderImage.GetWidth();
					for ( int x=0; x<theScene->renderImage.GetWidth(); x++, i0++, i1++ ) {
						Color24 t=c[i0]; c[i0]=c[i1]; c[i1]=t;
					}
				}
			}
			startTime = (int) time(nullptr);
			BeginRender( theScene );
			break;
		case MODE_RENDERING:
			mode = MODE_READY;
			StopRender();
			glutPostRedisplay();
			break;
		case MODE_RENDER_DONE: 
			mode = MODE_READY;
			viewMode = VIEWMODE_OPENGL;
			glutPostRedisplay();
			break;
		}
		break;
	case '1':
		viewAngle1 = viewAngle2 = 0;
		viewMode = VIEWMODE_OPENGL;
		glutPostRedisplay();
		break;
	case '2':
		viewMode = VIEWMODE_IMAGE;
		glutPostRedisplay();
		break;
	case '3':
		viewMode = VIEWMODE_Z;
		glutPostRedisplay();
		break;
	}
}

//-------------------------------------------------------------------------------

void PrintPixelData(int x, int y)
{
	if ( x < theScene->renderImage.GetWidth() && y < theScene->renderImage.GetHeight() ) {
		Color24 *colors = theScene->renderImage.GetPixels();
		float *zbuffer = theScene->renderImage.GetZBuffer();
		int i = (theScene->renderImage.GetHeight() - y - 1 ) *theScene->renderImage.GetWidth() + x;
		printf("Pixel [ %d, %d ] Color24: %d, %d, %d   Z: %f\n", x, y, colors[i].r, colors[i].g, colors[i].b, zbuffer[i] );
	} else {
		printf("-- Invalid pixel (%d,%d) --\n",x,y);
	}
}

//-------------------------------------------------------------------------------

void GlutMouse(int button, int state, int x, int y)
{
	if ( state == GLUT_UP ) {
		mouseMode = MOUSEMODE_NONE;
	} else {
		switch ( button ) {
			case GLUT_LEFT_BUTTON:
				mouseMode = MOUSEMODE_DEBUG;
				PrintPixelData(x,y);
				break;
			case GLUT_RIGHT_BUTTON:
				mouseMode = MOUSEMODE_ROTATE;
				mouseX = x;
				mouseY = y;
				break;
		}
	}
}

//-------------------------------------------------------------------------------

void GlutMotion(int x, int y)
{
	switch ( mouseMode ) {
		case MOUSEMODE_DEBUG:
			PrintPixelData(x,y);
			break;
		case GLUT_RIGHT_BUTTON:
			viewAngle1 -= 0.2f * ( mouseY - y );
			viewAngle2 -= 0.2f * ( mouseX - x );
			mouseX = x;
			mouseY = y;
			glutPostRedisplay();
			break;
	}
}

//-------------------------------------------------------------------------------
// Viewport Methods for various classes
//-------------------------------------------------------------------------------
void Sphere::ViewportDisplay() const
{
	static GLUquadric *q = nullptr;
	if ( q == nullptr ) {
		q = gluNewQuadric();
	}
	gluSphere(q,1,50,50);
}
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
