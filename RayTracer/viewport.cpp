
//-------------------------------------------------------------------------------
///
/// \file       viewport.cpp 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    5.0
/// \date       September 13, 2025
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
#include "lights.h"
#include "materials.h"
#include <stdlib.h>
#include <time.h>

#include <GL/freeglut.h>
#include <objects.h>

//-------------------------------------------------------------------------------
// How to use:

// Call this function to open a window that shows the scene to be renderered.
// The window also handles mouse and keyboard input:
// 1 - Shows OpenGL view
// 2 - Shows the rendered image
// 3 - Shows the z (depth) image
// Space - Starts/stops rendering. BeginRender and StopRender functions must be implemented.
// Esc - Terminates software
// Mouse left click - Writes the pixel information to the console
void ShowViewport(RenderScene* scene);

//-------------------------------------------------------------------------------
// To be implemented:

void BeginRender(RenderScene* scene); // Called to start rendering (renderer must run in a separate thread)
void StopRender(); // Called to end rendering (if it is not already finished)

//-------------------------------------------------------------------------------

RenderScene* theScene = nullptr;
MtlBlinn defaultMaterial;

//-------------------------------------------------------------------------------

#define WINDOW_TITLE        "Ray Tracer - CS 6620"
#define WINDOW_TITLE_OPENGL WINDOW_TITLE " - OpenGL"
#define WINDOW_TITLE_IMAGE  WINDOW_TITLE " - Rendered Image"
#define WINDOW_TITLE_Z      WINDOW_TITLE " - Z (Depth) Image"

enum Mode {
    MODE_READY,         // Ready to render
    MODE_RENDERING,     // Rendering the image
    MODE_RENDER_DONE    // Rendering is finished
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
};

static Mode      mode = MODE_READY;        // Rendering mode
static ViewMode  viewMode = VIEWMODE_OPENGL;   // Display mode
static MouseMode mouseMode = MOUSEMODE_NONE;    // Mouse mode
static int       startTime;                     // Start time of rendering
static GLuint    viewTexture;

//-------------------------------------------------------------------------------

void GlutDisplay();
void GlutReshape(int w, int h);
void GlutIdle();
void GlutKeyboard(unsigned char key, int x, int y);
void GlutMouse(int button, int state, int x, int y);
void GlutMotion(int x, int y);

//-------------------------------------------------------------------------------

void ShowViewport(RenderScene* scene)
{
    theScene = scene;

#ifdef _WIN32
    SetProcessDPIAware();
#endif

    int argc = 1;
    char argstr[] = "raytrace";
    char* argv = argstr;
    glutInit(&argc, &argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    if (glutGet(GLUT_SCREEN_WIDTH) > 0 && glutGet(GLUT_SCREEN_HEIGHT) > 0) {
        glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - theScene->camera.imgWidth) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - theScene->camera.imgHeight) / 2);
    }
    else glutInitWindowPosition(50, 50);
    glutInitWindowSize(theScene->camera.imgWidth, theScene->camera.imgHeight);

    glutCreateWindow(WINDOW_TITLE_OPENGL);
    glutDisplayFunc(GlutDisplay);
    glutReshapeFunc(GlutReshape);
    glutIdleFunc(GlutIdle);
    glutKeyboardFunc(GlutKeyboard);
    glutMouseFunc(GlutMouse);
    glutMotionFunc(GlutMotion);

    glClearColor(0, 0, 0, 0);

    glPointSize(3.0);
    glEnable(GL_CULL_FACE);

    float zero[] = { 0,0,0,0 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

    glEnable(GL_NORMALIZE);

    glLineWidth(2);

    glGenTextures(1, &viewTexture);
    glBindTexture(GL_TEXTURE_2D, viewTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glutMainLoop();
}

//-------------------------------------------------------------------------------

void GlutReshape(int w, int h)
{
    if (w != theScene->camera.imgWidth || h != theScene->camera.imgHeight) {
        glutReshapeWindow(theScene->camera.imgWidth, theScene->camera.imgHeight);
    }
    else {
        glViewport(0, 0, w, h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float r = (float)w / float(h);
        Box const& box = theScene->rootNode.GetChildBoundBox();
        float len = (box.pmax - box.pmin).Length();
        gluPerspective(theScene->camera.fov, r, len / 100000, len);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
}

//-------------------------------------------------------------------------------

void DrawNode(Node const* node)
{
    glPushMatrix();

    const Material* mtl = node->GetMaterial();
    if (!mtl) mtl = &defaultMaterial;
    mtl->SetViewportMaterial();

    Matrix3f tm = node->GetTransform();
    Vec3f p = node->GetPosition();
    float m[16] = { tm[0],tm[1],tm[2],0, tm[3],tm[4],tm[5],0, tm[6],tm[7],tm[8],0, p.x,p.y,p.z,1 };
    glMultMatrixf(m);

    const Object* obj = node->GetNodeObj();
    if (obj) obj->ViewportDisplay(mtl);

    for (int i = 0; i < node->GetNumChild(); i++) {
        DrawNode(node->GetChild(i));
    }

    glPopMatrix();
}

//-------------------------------------------------------------------------------

void DrawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glPushMatrix();
    Vec3f p = theScene->camera.pos;
    Vec3f t = theScene->camera.pos + theScene->camera.dir;
    Vec3f u = theScene->camera.up;
    gluLookAt(p.x, p.y, p.z, t.x, t.y, t.z, u.x, u.y, u.z);

    if (theScene->lights.size() > 0) {
        for (unsigned int i = 0; i < theScene->lights.size(); i++) {
            theScene->lights[i]->SetViewportLight(i);
        }
    }
    else {
        // Default lighting for scenes without a light
        float white[] = { 1,1,1,1 };
        float black[] = { 0,0,0,0 };
        Vec4f p(theScene->camera.pos, 1);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_AMBIENT, black);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
        glLightfv(GL_LIGHT0, GL_SPECULAR, white);
        glLightfv(GL_LIGHT0, GL_POSITION, &p.x);
    }

    DrawNode(&theScene->rootNode);

    glPopMatrix();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}

//-------------------------------------------------------------------------------

void DrawImage(void const* data, GLenum type, GLenum format)
{
    glBindTexture(GL_TEXTURE_2D, viewTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, theScene->renderImage.GetWidth(), theScene->renderImage.GetHeight(), 0, format, type, data);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(-1, -1);
    glTexCoord2f(1, 1);
    glVertex2f(1, -1);
    glTexCoord2f(1, 0);
    glVertex2f(1, 1);
    glTexCoord2f(0, 0);
    glVertex2f(-1, 1);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_TEXTURE_2D);
}

//-------------------------------------------------------------------------------

void DrawProgressBar(float done)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_LINES);
    glColor3f(1, 1, 1);
    glVertex2f(-1, -1);
    glVertex2f(done * 2 - 1, -1);
    glColor3f(0, 0, 0);
    glVertex2f(done * 2 - 1, -1);
    glVertex2f(1, -1);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

//-------------------------------------------------------------------------------

void DrawRenderProgressBar()
{
    int rp = theScene->renderImage.GetNumRenderedPixels();
    int np = theScene->renderImage.GetWidth() * theScene->renderImage.GetHeight();
    if (rp >= np) return;
    float done = (float)rp / (float)np;
    DrawProgressBar(done);
}

//-------------------------------------------------------------------------------

void GlutDisplay()
{
    switch (viewMode) {
    case VIEWMODE_OPENGL:
        DrawScene();
        break;
    case VIEWMODE_IMAGE:
        DrawImage(theScene->renderImage.GetPixels(), GL_UNSIGNED_BYTE, GL_RGB);
        break;
    case VIEWMODE_Z:
        theScene->renderImage.ComputeZBufferImage();
        DrawImage(theScene->renderImage.GetZBufferImage(), GL_UNSIGNED_BYTE, GL_LUMINANCE);
        break;
    }
    DrawRenderProgressBar();

    glutSwapBuffers();
}

//-------------------------------------------------------------------------------

void GlutIdle()
{
    static int lastRenderedPixels = 0;
    if (mode == MODE_RENDERING) {
        int nrp = theScene->renderImage.GetNumRenderedPixels();
        if (lastRenderedPixels != nrp) {
            lastRenderedPixels = nrp;
            if (theScene->renderImage.IsRenderDone()) {
                mode = MODE_RENDER_DONE;
                int endTime = (int)time(nullptr);
                int t = endTime - startTime;
                int h = t / 3600;
                int m = (t % 3600) / 60;
                int s = t % 60;
                printf("\nRender time is %d:%02d:%02d.\n", h, m, s);
            }
            glutPostRedisplay();
        }
    }
}

//-------------------------------------------------------------------------------

void GlutKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:    // ESC
        exit(0);
        break;
    case ' ':
        switch (mode) {
        case MODE_READY:
            mode = MODE_RENDERING;
            viewMode = VIEWMODE_IMAGE;
            glutSetWindowTitle(WINDOW_TITLE_IMAGE);
            DrawScene();
            glReadPixels(0, 0, theScene->renderImage.GetWidth(), theScene->renderImage.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, theScene->renderImage.GetPixels());
            {
                Color24* c = theScene->renderImage.GetPixels();
                for (int y0 = 0, y1 = theScene->renderImage.GetHeight() - 1; y0 < y1; y0++, y1--) {
                    int i0 = y0 * theScene->renderImage.GetWidth();
                    int i1 = y1 * theScene->renderImage.GetWidth();
                    for (int x = 0; x < theScene->renderImage.GetWidth(); x++, i0++, i1++) {
                        Color24 t = c[i0]; c[i0] = c[i1]; c[i1] = t;
                    }
                }
            }
            startTime = (int)time(nullptr);
            BeginRender(theScene);
            break;
        case MODE_RENDERING:
            mode = MODE_READY;
            StopRender();
            glutPostRedisplay();
            break;
        case MODE_RENDER_DONE:
            mode = MODE_READY;
            viewMode = VIEWMODE_OPENGL;
            glutSetWindowTitle(WINDOW_TITLE_OPENGL);
            glutPostRedisplay();
            break;
        }
        break;
    case '1':
        viewMode = VIEWMODE_OPENGL;
        glutSetWindowTitle(WINDOW_TITLE_OPENGL);
        glutPostRedisplay();
        break;
    case '2':
        viewMode = VIEWMODE_IMAGE;
        glutSetWindowTitle(WINDOW_TITLE_IMAGE);
        glutPostRedisplay();
        break;
    case '3':
        viewMode = VIEWMODE_Z;
        glutSetWindowTitle(WINDOW_TITLE_Z);
        glutPostRedisplay();
        break;
    }
}

//-------------------------------------------------------------------------------

void PrintPixelData(int x, int y)
{
    if (x >= 0 && y >= 0 && x < theScene->renderImage.GetWidth() && y < theScene->renderImage.GetHeight()) {
        Color24* colors = theScene->renderImage.GetPixels();
        float* zbuffer = theScene->renderImage.GetZBuffer();
        int i = y * theScene->renderImage.GetWidth() + x;
        printf("Pixel [ %d, %d ] Color24: %d, %d, %d   Z: %f\n", x, y, colors[i].r, colors[i].g, colors[i].b, zbuffer[i]);
    }
}

//-------------------------------------------------------------------------------

void GlutMouse(int button, int state, int x, int y)
{
    if (state == GLUT_UP) {
        mouseMode = MOUSEMODE_NONE;
    }
    else {
        switch (button) {
        case GLUT_LEFT_BUTTON:
            mouseMode = MOUSEMODE_DEBUG;
            PrintPixelData(x, y);
            break;
        }
    }
}

//-------------------------------------------------------------------------------

void GlutMotion(int x, int y)
{
    switch (mouseMode) {
    case MOUSEMODE_DEBUG:
        PrintPixelData(x, y);
        break;
    }
}

//-------------------------------------------------------------------------------
// Viewport Methods for various classes
//-------------------------------------------------------------------------------
void Sphere::ViewportDisplay(Material const* mtl) const
{
    static GLUquadric* q = nullptr;
    if (q == nullptr) {
        q = gluNewQuadric();
    }
    gluSphere(q, 1, 50, 50);
}
void Plane::ViewportDisplay(Material const* mtl) const
{
    const int resolution = 32;
    glPushMatrix();
    glScalef(2.0f / resolution, 2.0f / resolution, 2.0f / resolution);
    glNormal3f(0, 0, 1);
    glBegin(GL_QUADS);
    for (int y = 0; y < resolution; y++) {
        int yy = y - resolution / 2;
        for (int x = 0; x < resolution; x++) {
            int xx = x - resolution / 2;
            glVertex3i(yy, xx, 0);
            glVertex3i(yy + 1, xx, 0);
            glVertex3i(yy + 1, xx + 1, 0);
            glVertex3i(yy, xx + 1, 0);
        }
    }
    glEnd();
    glPopMatrix();
}
void TriObj::ViewportDisplay(Material const* mtl) const
{
    glBegin(GL_TRIANGLES);
    for (unsigned int i = 0; i < NF(); i++) {
        for (int j = 0; j < 3; j++) {
            if (HasTextureVertices()) glTexCoord3fv(&VT(FT(i).v[j]).x);
            if (HasNormals()) glNormal3fv(&VN(FN(i).v[j]).x);
            glVertex3fv(&V(F(i).v[j]).x);
        }
    }
    glEnd();
}
void GenLight::SetViewportParam(int lightID, ColorA ambient, ColorA intensity, Vec4f pos) const
{
    glEnable(GL_LIGHT0 + lightID);
    glLightfv(GL_LIGHT0 + lightID, GL_AMBIENT, &ambient.r);
    glLightfv(GL_LIGHT0 + lightID, GL_DIFFUSE, &intensity.r);
    glLightfv(GL_LIGHT0 + lightID, GL_SPECULAR, &intensity.r);
    glLightfv(GL_LIGHT0 + lightID, GL_POSITION, &pos.x);
}
void MtlPhong::SetViewportMaterial(int subMtlID) const
{
    ColorA d(diffuse);
    ColorA s(specular);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &d.r);
    glMaterialfv(GL_FRONT, GL_SPECULAR, &s.r);
    glMaterialf(GL_FRONT, GL_SHININESS, glossiness * 2);
}
void MtlBlinn::SetViewportMaterial(int subMtlID) const
{
    ColorA d(diffuse);
    ColorA s(specular);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &d.r);
    glMaterialfv(GL_FRONT, GL_SPECULAR, &s.r);
    glMaterialf(GL_FRONT, GL_SHININESS, glossiness);
}
void MtlMicrofacet::SetViewportMaterial(int subMtlID) const
{
    float ff = (ior - 1) / (ior + 1);
    float f0d = ff * ff;
    Color f0 = (1 - metallic) * f0d + metallic * baseColor;
    float a = roughness * roughness;
    float k = a / 2;
    float D = 1 / (Pi<float>() * a * a);
    float t = roughness * roughness * (3 - 2 * roughness);
    ColorA d(baseColor * ((1 - metallic) * (1 - f0) + metallic * t * 0.25f) / Pi<float>());
    ColorA s(f0 * D / 4);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &d.r);
    glMaterialfv(GL_FRONT, GL_SPECULAR, &s.r);
    glMaterialf(GL_FRONT, GL_SHININESS, (1 - roughness) * 128);
}
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
