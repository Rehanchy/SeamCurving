//   Copyright © 2021, Renjie Chen @ USTC
//   Hanchi Li @ USTC

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#define FREEGLUT_STATIC
#include "gl_core_3_3.h"
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#define TW_STATIC
#include <AntTweakBar.h>

#include <vector>
#include <string>

#include "glprogram.h"
#include "MyImage.h"
#include "VAOImage.h"
#include "VAOMesh.h"


GLProgram MyMesh::prog;

MyMesh M;
int viewport[4] = { 0, 0, 1280, 960 };

bool showATB = true;
bool use_energy_from_file = true;
bool Context_Aware = false;


std::string imagefile = "boy.png";
std::string imagefile2 = "boy_Saliency.png";
std::string imagefile3 = "Context_Aware.png";

MyImage img,imgi;
int resize_width, resize_height;

int mousePressButton;
int mouseButtonDown;
int mousePos[2];

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, viewport[2], viewport[3]);
    M.draw(viewport);

    if (showATB) TwDraw();
    glutSwapBuffers();
}

void onKeyboard(unsigned char code, int x, int y)
{
    if (!TwEventKeyboardGLUT(code, x, y)) {
        switch (code) {
        case 17:
            exit(0);
        case 'f':
            glutFullScreenToggle();
            break;
        case ' ':
            showATB = !showATB;
            break;
        }
    }

    glutPostRedisplay();
}

void onMouseButton(int button, int updown, int x, int y)
{
    if (!showATB || !TwEventMouseButtonGLUT(button, updown, x, y)) {
        mousePressButton = button;
        mouseButtonDown = updown;

        mousePos[0] = x;
        mousePos[1] = y;
    }

    glutPostRedisplay();
}


void onMouseMove(int x, int y)
{
    if (!showATB || !TwEventMouseMotionGLUT(x, y)) {
        if (mouseButtonDown == GLUT_DOWN) {
            if (mousePressButton == GLUT_MIDDLE_BUTTON) {
                M.moveInScreen(mousePos[0], mousePos[1], x, y, viewport);
            }
        }
    }

    mousePos[0] = x; mousePos[1] = y;

    glutPostRedisplay();
}


void onMouseWheel(int wheel_number, int direction, int x, int y)
{
    if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
    }
    else
        M.mMeshScale *= direction > 0 ? 1.1f : 0.9f;

    glutPostRedisplay();
}

int initGL(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(960, 960);
    glutInitWindowPosition(200, 50);
    glutCreateWindow(argv[0]);

    // !Load the OpenGL functions. after the opengl context has been created
    if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
        return -1;

    glClearColor(1.f, 1.f, 1.f, 0.f);

    glutReshapeFunc([](int w, int h) { viewport[2] = w; viewport[3] = h; TwWindowSize(w, h); });
    glutDisplayFunc(display);
    glutKeyboardFunc(onKeyboard);
    glutMouseFunc(onMouseButton);
    glutMotionFunc(onMouseMove);
    glutMouseWheelFunc(onMouseWheel);
    glutCloseFunc([]() {exit(0); });
    return 0;
}

void uploadImage(const MyImage& img)
{
    int w = img.width();
    int h = img.height();
    float x[] = { 0, 0, 0, w, 0, 0, w, h, 0, 0, h, 0 };
    M.upload(x, 4, nullptr, 0, nullptr);

    M.tex.setImage(img);
    M.tex.setClamping(GL_CLAMP_TO_EDGE);
}


void readImage(const std::string& file)
{
    int w0 = img.width(), h0 = img.height();
    img = MyImage(file);
    uploadImage(img);
    resize_width = img.width();
    resize_height = img.height();

    if (w0 != img.width() || h0 != img.height()) M.updateBBox();
}

void saveImage(const std::string& outfile, int  Width, int  Height, int Channels, unsigned char* Output, bool open = true)
{
    stbi_write_png(outfile.data(), Width, Height, Channels, Output, open);
}

void readEnergy(const std::string& file)
{
    int w0 = imgi.width(), h0 = imgi.height();
    imgi = MyImage(file);
    if (Context_Aware)
    {
        imgi = imgi.Context_Aware_Saliency(imgi);
    }
    imgi = imgi.Get_Grey();      // turn grey
    if(Context_Aware)
        imgi.write("Context_Aware.png");
    uploadImage(imgi);
    resize_width = imgi.width();
    resize_height = imgi.height();

    if (w0 != imgi.width() || h0 != imgi.height()) M.updateBBox();
}

MyImage SeamCarving_Row(MyImage& img)
{
    vector<int> index(0);

    if (use_energy_from_file == true) 
    {
        index = img.Seamindex_Row(imgi);
        imgi = imgi.Row_Delete(index);
    }
    uploadImage(img);
    display();

    img = img.Row_Delete(index);
    return img;
}

MyImage SeamCarving_Col(MyImage& img)
{
    vector<int> index(0);
    if (use_energy_from_file == true) 
    { 
        index = img.Seamindex_Col(imgi);
        imgi = imgi.Col_Delete(index);
    }
    uploadImage(img);
    display();

    img = img.Col_Delete(index);
    return img;
}


void Expand_Col(MyImage& img)      
{
    vector<int> index(0);
    index = img.Seamindex_Col(imgi);
    uploadImage(img);
    display();
    imgi = imgi.Col_Add_for_Energy(index);
    img = img.Col_Add(index);
 }

void Expand_Row(MyImage& img)
{
    vector<int> index(0);
    index = img.Seamindex_Row(imgi);
    uploadImage(img);
    display();
    imgi = imgi.Row_Add_for_Energy(index);
    img = img.Row_Add(index);
}

MyImage seamCarving(MyImage& img, int w, int h)
{
    if (use_energy_from_file == true && (img.height() != imgi.height() || img.width() != imgi.width())) 
    {
        cout << "two files doesn't match";
        MyImage null;
        return null;
    }
    if (w <= img.width())
    {
        while (img.width() > w)
            SeamCarving_Col(img);
    }
    else 
        while (img.width() < w)
            Expand_Col(img);
    if (h <= img.height()) 
        while (img.height() > h)
            SeamCarving_Row(img);
    else
        while (img.height() < h)
            Expand_Row(img);
    return img;
}

void createTweakbar()
{
    //Create a tweak bar
    TwBar *bar = TwNewBar("Image Viewer");
    TwDefine(" 'Image Viewer' size='220 150' color='0 128 255' text=dark alpha=128 position='5 5'"); // change default tweak bar size and color

    TwAddVarRW(bar, "Scale", TW_TYPE_FLOAT, &M.mMeshScale, " min=0 step=0.1");

    TwAddVarRW(bar, "Image filename", TW_TYPE_STDSTRING, &imagefile, " ");
    TwAddVarRW(bar, "Energy filename", TW_TYPE_STDSTRING, &imagefile2, " ");
    TwAddButton(bar, "Read Image", [](void*) { readImage(imagefile); }, nullptr, "");

    TwAddVarRW(bar, "Resize Width", TW_TYPE_INT32, &resize_width, "group='Seam Carving' min=1 ");
    TwAddVarRW(bar, "Resize Height", TW_TYPE_INT32, &resize_height, "group='Seam Carving' min=1 ");
    TwAddButton(bar, "Run Seam Carving", [](void* img) {
        MyImage newimg = seamCarving(*(MyImage*)img, resize_width, resize_height);
        uploadImage(newimg);
        }, 
        &img, "");
    TwAddButton(bar, "Read Energy Image", [](void*) {
        readEnergy(imagefile2);
        use_energy_from_file = true; 
        }, nullptr, "");

    TwBar* bnsbar = TwNewBar("Bonus");
    TwDefine(" 'Bonus' size='280 150' color='156 38 50' text=white alpha=128 position='425 5'");
    TwAddVarRW(bnsbar, "Use Context-Aware Saliency, this means calculate CAS ", TW_TYPE_BOOLCPP, &Context_Aware, "");
    TwAddButton(bnsbar, "See Context-Aware Saliency", [](void* img) {
        if(Context_Aware)
            readEnergy(imagefile);
        else
        {
            use_energy_from_file = true;
            readEnergy(imagefile3);
        }
        },
        &img, "");
    TwAddButton(bnsbar, "Use Context-Aware to Calculate", [](void* img) {
        MyImage newimg = seamCarving(*(MyImage*)img, resize_width, resize_height);
        uploadImage(newimg);
        },
        &img, "");
 
}


int main(int argc, char *argv[])
{
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), { 100, 5000 });

    if (initGL(argc, argv)) {
        fprintf(stderr, "!Failed to initialize OpenGL!Exit...");
        exit(-1);
    }

    MyMesh::buildShaders();


    float x[] = { 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0 };
    float uv[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
    int t[] = { 0, 1, 2, 2, 3, 0 };

    M.upload(x, 4, t, 2, uv);

    //////////////////////////////////////////////////////////////////////////
    TwInit(TW_OPENGL_CORE, NULL);
    //Send 'glutGetModifers' function pointer to AntTweakBar;
    //required because the GLUT key event functions do not report key modifiers states.
    TwGLUTModifiersFunc(glutGetModifiers);
    glutSpecialFunc([](int key, int x, int y) { TwEventSpecialGLUT(key, x, y); glutPostRedisplay(); }); // important for special keys like UP/DOWN/LEFT/RIGHT ...
    TwCopyStdStringToClientFunc([](std::string& dst, const std::string& src) {dst = src; });


    createTweakbar();

    //////////////////////////////////////////////////////////////////////////
    atexit([] { TwDeleteAllBars();  TwTerminate(); });  // Called after glutMainLoop ends

    glutTimerFunc(1, [](int) { readImage(imagefile); },  0);


    //////////////////////////////////////////////////////////////////////////
    glutMainLoop();

    return 0;
}

