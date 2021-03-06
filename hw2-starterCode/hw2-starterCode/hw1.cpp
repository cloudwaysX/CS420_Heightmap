/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code
  Student username: <type your USC username here>
*/

#include <iostream>
#include <cstring>
#include <vector>
#include "openGLHeader.h"
#include "glutHeader.h"
#include <cmath>
#include <cstdio>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"
#include "secondPipelineProgram.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

#define BUFFER_OFFSET(i) ((void*)(i)) //define the buffer offset

using namespace std;

void GenNewRailway();

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not


//three rendering types
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;


// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

//global variables
OpenGLMatrix *openGLMatrix;
GLuint saveCount=0;

//spline
GLuint vbo;
GLuint ebo;
GLuint vao;
GLuint *indices_lines;
vector<GLfloat> positions;
vector<GLfloat> color;
BasicPipelineProgram *pipelineProgram;
GLuint program;
void bindProgram();
glm::mat4 basisMatrix;
glm::mat4 controlMatrix;
glm::vec3 offset(0,0,0);//when we have multiple splines, we want the next spline be continous to the previous one, so we do affine change

//cross vertices
std::vector<GLfloat> crossVerticesPos_left;
std::vector<GLfloat> crossVerticesPos_right;
std::vector<GLuint> crossIndices;
GLuint vbo_cross_left;
GLuint vao_cross_left;
GLuint ebo_cross_left;
GLuint vbo_cross_right;
GLuint vao_cross_right;
GLuint ebo_cross_right;
vector<GLfloat> color_cross;
const GLuint LEFTRAIL=0;
const GLuint RIGHTRAIL=1;

//Tshape
std::vector<GLfloat> TshapePos;
std::vector<GLuint> TshapeIndices;
GLuint vbo_Tshape;
GLuint vao_Tshape;
GLuint ebo_Tshape;
vector<GLfloat> color_Tshape;
GLfloat interval=0.4f;//the interval between two T shape should >=0.4f


//moving camera
GLuint curPo_index;
glm::vec4 tangParameterVec;
vector<glm::vec3> tangVec;
const GLuint POSITION=0;
const GLuint TANGENT=1;
const glm::vec3 V_DOWN(0,-1,0);//people upside down
const glm::vec3 V_UP(0,1,0);//people not upside down
const GLfloat INITIAL_V=0.0001f;
GLuint countStep;
GLfloat timeStep=0.0001f;
GLfloat HMAX=0.0f;
const GLfloat g=9.8f;
typedef enum { REAL, SUBDIVIDE } MOVE_MODE;
MOVE_MODE moveMode = REAL;

//ground
GLuint vao_groundTex;
GLuint vbo_groundTex;
vector<GLfloat> groundPos;
vector<GLfloat> groundUVs;
GLuint groundTexHandle;
void BindGroundTexProgram();

//sky
GLuint vao_skyTex;
GLuint vbo_skyTex;
GLuint ebo_skyTex;
vector<GLfloat> skyPos;
vector<GLuint> skyIndices;
GLuint skyTexHandle;
void BindSkyTexProgram();

//for both sky and ground
SecondPipelineProgram *tex2DPipeline;
GLuint tex2DProgram;
SecondPipelineProgram *tex3DPipeline;
GLuint tex3DProgram;

//Set some constant parameter for the window view
const float fovy = 60;//the view angle is 45 degrees
const float aspect=(float)windowWidth/(float)windowHeight;//calculate the perpective 


// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};

// the spline array 
Spline * splines;
GLuint curSplineIndex=0;

// total number of splines 
int numSplines;

int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;
  cout<<fopen(argv, "r")<<endl;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file\n");
    exit(1);
  }
  
  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
     &splines[j].points[i].x, 
     &splines[j].points[i].y, 
     &splines[j].points[i].z) != EOF) 
    {
      i++;
    }
  }

  free(cName);

  return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  cout<<imageFilename<<endl;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 575; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;

  return 0;
}

//Initiate the GL_TEXTURE_CUBE instead of GL_TEXTURE_2D
int initTextureCubeMap(string imageFoldername, GLuint textureHandle){
  // bind the texture
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureHandle);

  // read the texture image
  string filename[6]={"/right.jpg","/left.jpg","/top.jpg","/bot.jpg","/back.jpg","/front.jpg"};
  for(int i=0;i<6;i++){
  	ImageIO img;
  	ImageIO::fileFormatType imgFormat;
  	string name=imageFoldername+filename[i];
  	const char* theName =name.c_str();
  	cout<<theName<<endl;
  	ImageIO::errorType err = img.load(theName, &imgFormat);
  	if (err != ImageIO::OK) 
  	{
    	printf("Loading texture from %s failed.\n", name);
    	return -1;
  	}

  	// check that the number of bytes is a multiple of 4
  	if (img.getWidth() * img.getBytesPerPixel() % 4) 
  	{
    	printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", name);
    	return -1;
  	}
  	// allocate space for an array of pixels
  	int width = img.getWidth();
  	int height = img.getHeight();
  	unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 3 bytes per pixel, i.e., RGBA

  	// fill the pixelsRGBA array with the image pixels
  	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  	for (int h = 0; h < height; h++)
    	for (int w = 0; w < width; w++) 
    	{
      		// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      		pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      		pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      		pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      		pixelsRGBA[4 * (h * width + w) + 3] = 575; // alpha channel; fully opaque

      		// set the RGBA channels, based on the loaded image
      		int numChannels = img.getBytesPerPixel();
      		for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        		pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    	}
    // initialize the texture
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

    // de-allocate the pixel array -- it is no longer needed
  	delete [] pixelsRGBA;

  }
    // set the texture parameters
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);

  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  

  return 0;

}


void setTextureUnit(GLint unit, GLuint texProgram){
  glActiveTexture(unit);

  GLint h_textureImage=glGetUniformLocation(texProgram,"textureImage");
  glUniform1i(h_textureImage,unit-GL_TEXTURE0);
}


// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void renderSpline(){
  pipelineProgram->Bind();
  glBindVertexArray(vao_cross_left);
  glDrawElements(GL_TRIANGLE_STRIP,crossIndices.size(),GL_UNSIGNED_INT,BUFFER_OFFSET(0));
  glBindVertexArray(0);
  glBindVertexArray(vao_cross_right);
  glDrawElements(GL_TRIANGLE_STRIP,crossIndices.size(),GL_UNSIGNED_INT,BUFFER_OFFSET(0));
  glBindVertexArray(0);
  glBindVertexArray(vao_Tshape);
  glDrawElements(GL_TRIANGLES,TshapeIndices.size(),GL_UNSIGNED_INT,BUFFER_OFFSET(0));
  glBindVertexArray(0);
}

void renderGround(){
  tex2DPipeline->Bind();
  setTextureUnit(GL_TEXTURE0,tex2DProgram);
  glBindTexture(GL_TEXTURE_2D,groundTexHandle);

  glBindVertexArray(vao_groundTex);//bind vao
  glDrawArrays(GL_TRIANGLES,0,6);
  glBindVertexArray(0);
}

void renderSky(){
  tex3DPipeline->Bind();
  setTextureUnit(GL_TEXTURE0,tex3DProgram);
  glBindTexture(GL_TEXTURE_CUBE_MAP,skyTexHandle);

  glBindVertexArray(vao_skyTex);
  glDrawArrays(GL_TRIANGLES,0,36);
  //glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,BUFFER_OFFSET(0));
  glBindVertexArray(0);
}


///////////create moving Camera///////////////////////////////
void SetLookAt(){
  GLfloat eyeX=positions.at(curPo_index*3);
  GLfloat eyeY=positions.at(curPo_index*3+1);
  GLfloat eyeZ=positions.at(curPo_index*3+2);

  glm::vec3 V;
  V=V_DOWN;
  glm::vec3 center=tangVec.at(curPo_index);
  if(center==V||center==-V){
  	curPo_index+=countStep; 
  	return;
  }
  glm::vec3 up=glm::cross(center,V);
  up=glm::abs(up/glm::length(up));
  up=glm::cross(up,center);
  openGLMatrix->LookAt(eyeX+up[0]*0.1f, eyeY+up[1]*0.1f, eyeZ+up[2]*0.1f, center[0]+eyeX, center[1]+eyeY, center[2]+eyeZ, up[0], up[1], up[2]);  
  //plus up*0.1 because in reality peopel in th cart is a little higher than rail

  offset=glm::vec3(eyeX,eyeY,eyeZ);
  //cout<<offset.x<<" "<<offset.y<<" "<<offset.z<<endl;
}

void doTransform()
{
  //Calculate the transform matrix and store it into m_view
  openGLMatrix->SetMatrixMode(OpenGLMatrix::ModelView);
  openGLMatrix->LoadIdentity();
  SetLookAt(); 
  //openGLMatrix->LookAt(0.0f, 0.0f, 2.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
  openGLMatrix->Translate(landTranslate[0],landTranslate[1],landTranslate[2]);
  openGLMatrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
  openGLMatrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
  openGLMatrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);
  openGLMatrix->Scale(landScale[0],landScale[1],landScale[2]);

  //store the ModelView matrix into m_view
  float m_view[16];
  openGLMatrix->SetMatrixMode(OpenGLMatrix::ModelView);
  openGLMatrix->GetMatrix(m_view);

  //store the perspective matrix into m_perspective
  float m_perspective[16];
  openGLMatrix->SetMatrixMode(OpenGLMatrix::Projection);
  openGLMatrix->GetMatrix(m_perspective);

  //upload the transformation to shader
  pipelineProgram->Bind();
  pipelineProgram->SetModelViewMatrix(m_view);
  pipelineProgram->SetProjectionMatrix(m_perspective);
  tex2DPipeline->Bind();
  tex2DPipeline->SetModelViewMatrix(m_view);
  tex2DPipeline->SetProjectionMatrix(m_perspective);
  tex3DPipeline->Bind();
  tex3DPipeline->SetModelViewMatrix(m_view);
  tex3DPipeline->SetProjectionMatrix(m_perspective);
}

void displayFunc()
{
  //clear the Window
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  //clear buffer
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  glDepthMask(GL_TRUE);
  glBindVertexArray(0);

  doTransform();
  renderSky();
  renderSpline();
  renderGround();


  glutSwapBuffers();

  //update the camera postion
  curPo_index+=countStep; 
  if(curPo_index>=positions.size()/3){
    curPo_index=0;
    curSplineIndex=rand()%numSplines;
    if(curSplineIndex>numSplines-1){
    	curSplineIndex=0;
    }
    if(numSplines>1){
    	GenNewRailway();
    }
    cout<<curSplineIndex<<endl;

  }

}
void idleFunc()
{
	
	/*saveCount++;

	char screenName[10];
	sprintf(screenName, "%03d", saveCount/4);
	saveScreenshot(("animation/"+std::string(screenName) + ".jpg").c_str());*/
	
    glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // setup perspective matrix...
  glViewport(0, 0, w, h);
  // setup perspective matrix...
  openGLMatrix->SetMatrixMode(OpenGLMatrix::Projection);
  openGLMatrix->LoadIdentity();
  openGLMatrix->Perspective(fovy,aspect,0.01f,1000.0f);
  openGLMatrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      delete []indices_lines;
        exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar. Switch Move Mode" << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
  }
}

///////////////////////////////////////////////
/////create spline
void LoadBasisMatrix(float s) {
  float m[16] = { -1 * s, 2.0f - s, s - 2.0f, s,
                  2 * s, s - 3.0f, 3 - 2.0f * s, -1 * s,
                  -1 * s, 0, s, 0,
                  0, 1, 0, 0 };
  basisMatrix = glm::make_mat4(m);
  
}

void LoadControlMatrix(Point &p1,Point &p2,Point &p3,Point &p4 ){
  float m[16] = { p1.x, p1.y, p1.z,0,
                  p2.x, p2.y, p2.z,0,
                  p3.x, p3.y, p3.z,0,
                  p4.x, p4.y, p4.z,0 };
  controlMatrix = glm::make_mat4(m);
}

glm::vec4 LoadParamter(float u){
  float v[4]={pow(u,3),pow(u,2),u,1.0f};
  return glm::make_vec4(v);
}

glm::vec4 LoadTangParamter(float u){
  float v[4]={3*pow(u,2),2*u,1.0f,0.0f};
  return glm::make_vec4(v);
}

//use subdivided recursion to generate the roller coaster
void SubDivide(GLfloat u0, GLfloat u1, GLfloat maxLength){
  glm::vec4 v0=LoadParamter(u0);
  glm::vec4 v1=LoadParamter(u1);
  GLfloat uMid=(u0+u1)/2;
  if(glm::distance(v0,v1)>maxLength){
    SubDivide(u0,uMid,maxLength);
    SubDivide(uMid,u1,maxLength);
  }
  else{
      glm::vec3 temp(controlMatrix*basisMatrix*v1);

      //calculate the vertices position
      positions.push_back(temp[0]);
      positions.push_back(temp[1]);
      positions.push_back(temp[2]);

      //calculate the tangent line
      glm::vec4 tangParameterVec=LoadTangParamter(u1);
      glm::vec3 temp2(controlMatrix*basisMatrix*tangParameterVec);
      tangVec.push_back(temp2/glm::length(temp2));
  }
   
}

//simulate the real gravity
GLfloat RealGravity(GLfloat u0, GLfloat u1,bool offsetIsCalculated){
	GLfloat new_u=u0;
	while(new_u<=u1){
		glm::vec4 tangParameterVec=LoadTangParamter(new_u);
  		glm::vec3 temp(controlMatrix*basisMatrix*tangParameterVec);
  		tangVec.push_back(temp/glm::length(temp));

		glm::vec4 v1=LoadParamter(new_u);
		glm::vec3 temp2(controlMatrix*basisMatrix*v1);
		if(!offsetIsCalculated){
  			offset=offset-temp2;
  			offsetIsCalculated=true;
  		}
  		temp2=temp2+offset;
      	positions.push_back(temp2[0]);
      	positions.push_back(temp2[1]);
      	positions.push_back(temp2[2]);		
		new_u=new_u+timeStep*pow(2*g*(HMAX-temp2[1]),0.5f)/glm::length(temp)+INITIAL_V;
	}
	return new_u-u1;
}

//calculate the vertices between two control point in either subdivide or real gravity mode
GLfloat CalculateVertice(GLfloat u0, GLfloat u1, GLfloat maxLength, GLuint mode,bool offsetIsCalculated){

  if(mode==REAL){
  	return RealGravity(u0,u1,offsetIsCalculated);
  }

  //first load the u0
  glm::vec4 tangParameterVec=LoadTangParamter(u0);
  glm::vec3 temp(controlMatrix*basisMatrix*tangParameterVec);
  tangVec.push_back(temp/glm::length(temp));

  glm::vec4 parameterVec=LoadParamter(u0);
  glm::vec3 temp2(controlMatrix*basisMatrix*parameterVec);
  if(!offsetIsCalculated){
  	offset=offset-temp2;
  	offsetIsCalculated=true;
  }
  temp2=temp2+offset;
  positions.push_back(temp2[0]);
  positions.push_back(temp2[1]);
  positions.push_back(temp2[2]);

  //then do the subdivide recursion
  SubDivide(u0,u1,maxLength);

  return 0.0f;
}

//calcualte vertices of crossroad section
void CalculateCrossVertices(GLuint splines_Index, vector<GLfloat>& crossVerticesPos, GLuint mode){
  GLfloat curPoX=positions.at(splines_Index*3);
  GLfloat curPoY=positions.at(splines_Index*3+1);
  GLfloat curPoZ=positions.at(splines_Index*3+2);
  glm::vec3 p0(curPoX,curPoY,curPoZ);
  glm::vec3 V;
  V=V_DOWN;
  glm::vec3 t0=tangVec.at(splines_Index);
  glm::vec3 b0=glm::cross(t0,V);
  b0=b0/glm::length(b0);
  glm::vec3 n0=glm::cross(b0,t0);
  GLfloat railW=0.1f;

  b0=glm::abs(b0);
  if(mode==LEFTRAIL){
    p0=p0-railW*b0;
  }
  else{
    p0=p0+railW*b0;
  }

  float length_regulizar=0.02f;
  glm::vec3 v0=p0+length_regulizar*(b0-n0);
  glm::vec3 v1=p0+length_regulizar*(b0+n0);
  glm::vec3 v2=p0+length_regulizar*(-b0+n0);
  glm::vec3 v3=p0+length_regulizar*(-b0-n0);

  crossVerticesPos.push_back(v0[0]);
  crossVerticesPos.push_back(v0[1]);
  crossVerticesPos.push_back(v0[2]);
  crossVerticesPos.push_back(v1[0]);
  crossVerticesPos.push_back(v1[1]);
  crossVerticesPos.push_back(v1[2]);
  crossVerticesPos.push_back(v2[0]);
  crossVerticesPos.push_back(v2[1]);
  crossVerticesPos.push_back(v2[2]);
  crossVerticesPos.push_back(v3[0]);
  crossVerticesPos.push_back(v3[1]);
  crossVerticesPos.push_back(v3[2]);


  if(mode==LEFTRAIL){
  	TshapePos.push_back(v0[0]);
  	TshapePos.push_back(v0[1]);
  	TshapePos.push_back(v0[2]);
   	TshapePos.push_back(v1[0]);
  	TshapePos.push_back(v1[1]);
  	TshapePos.push_back(v1[2]);
  }
  else{
  	TshapePos.push_back(v2[0]);
  	TshapePos.push_back(v2[1]);
  	TshapePos.push_back(v2[2]);
   	TshapePos.push_back(v3[0]);
  	TshapePos.push_back(v3[1]);
  	TshapePos.push_back(v3[2]);  	
  }

}

//calculate color for each vertices, since left rail and right rail is exactly the same so we only calcualte once
void CalculateColor(){
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.5f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.5f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.5f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.4f);
  color_cross.push_back(0.5f);

  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.5f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.5f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.5f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.4f);
  color_Tshape.push_back(0.5f);

}

//generate whole spline vertices
void GenVerices(GLuint mode){
  bool offsetIsCalculated=false;

  LoadBasisMatrix(0.5f);
  int numOfInterval=splines[curSplineIndex].numControlPoints-3;
  GLfloat prevU_more=0.0f;

  //calculate the maxium height first
  for(int i=0;i<splines[curSplineIndex].numControlPoints;i++){
    if(splines[curSplineIndex].points[i].y>HMAX){
    	HMAX=splines[curSplineIndex].points[i].y;
    }
  }

  cout<<HMAX<<endl;
  //calculate each vertices
  for(int i=0;i<numOfInterval;i++){
    LoadControlMatrix(splines[curSplineIndex].points[i],splines[curSplineIndex].points[i+1],splines[curSplineIndex].points[i+2],splines[curSplineIndex].points[i+3]);
    prevU_more=CalculateVertice(prevU_more, 1.0f, 0.001f,mode,offsetIsCalculated);
    offsetIsCalculated=true;
  }
}

//gnerate whole cross section vertices
void GenCrossVertices(){
  for(GLuint i=0;i<tangVec.size();i++){
    CalculateCrossVertices(i,crossVerticesPos_left,LEFTRAIL);
    CalculateCrossVertices(i,crossVerticesPos_right,RIGHTRAIL);
    CalculateColor();
  }
}

//generate indices for cross section vertices and T-shape vertices
void GenCrossIndices(){;
  //strips 1
  for(int i=0;i<tangVec.size();i++){
    crossIndices.push_back(i*4);
    crossIndices.push_back(i*4+1);
  }
  //strip 2
  for(int i=tangVec.size()-1;i>=0;i--){
    crossIndices.push_back(i*4+1);
    crossIndices.push_back(i*4+2);
  }
  //strip 3
  for(int i=0;i<tangVec.size();i++){
    crossIndices.push_back(i*4+2);
    crossIndices.push_back(i*4+3);
  }
  //strip 4
  for(int i=tangVec.size()-1;i>=0;i--){
    crossIndices.push_back(i*4+3);
    crossIndices.push_back(i*4);
  }

  glm::vec3 prevPo(0,0,0);//record the previous position where the interval is big enough
  for(int i=0;i<tangVec.size();i++){
  	glm::vec3 temp1(positions.at(i*3),positions.at(i*3+1),positions.at(i*3+2));
  	if(glm::distance(temp1,prevPo)<=interval){
  		continue;
  	}
  	prevPo=temp1;

    TshapeIndices.push_back(i*4+1);
    TshapeIndices.push_back(i*4+2);
    TshapeIndices.push_back(i*4+2+60);
    TshapeIndices.push_back(i*4+1);
    TshapeIndices.push_back(i*4+1+60);
    TshapeIndices.push_back(i*4+2+60);

    TshapeIndices.push_back(i*4);
    TshapeIndices.push_back(i*4+3);
    TshapeIndices.push_back(i*4+3+60);
    TshapeIndices.push_back(i*4+0);
    TshapeIndices.push_back(i*4+0+60);
    TshapeIndices.push_back(i*4+3+60);

    TshapeIndices.push_back(i*4+1);
    TshapeIndices.push_back(i*4+0);
    TshapeIndices.push_back(i*4+3);
    TshapeIndices.push_back(i*4+1);
    TshapeIndices.push_back(i*4+2);
    TshapeIndices.push_back(i*4+3);

    TshapeIndices.push_back(i*4+1+60);
    TshapeIndices.push_back(i*4+0+60);
    TshapeIndices.push_back(i*4+3+60);
    TshapeIndices.push_back(i*4+1+60);
    TshapeIndices.push_back(i*4+2+60);
    TshapeIndices.push_back(i*4+3+60);
  }

}

void initProgram(){
  //initiate the shader program
  pipelineProgram = new BasicPipelineProgram();
  if(pipelineProgram->Init(shaderBasePath)!=0){
    cout << "Error finding the shaderBasePath " << shaderBasePath << "." << endl;
    exit(EXIT_FAILURE);
  }
  pipelineProgram->Bind();
  //get pipeline program handle
  program=pipelineProgram->GetProgramHandle();
}

void bindProgram(vector<GLfloat > crossVerticesPos){
    GLuint loc = glGetAttribLocation(program, "position"); 
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    GLuint loc2 = glGetAttribLocation(program, "color"); 
    glEnableVertexAttribArray(loc2);
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0,BUFFER_OFFSET(crossVerticesPos.size()*sizeof(GLfloat)));

}

void GenCrossBuffer(GLuint& vao_cross,GLuint& vbo_cross,GLuint& ebo_cross, vector<GLfloat > crossVerticesPos){
  //create VAO
  glGenVertexArrays(1,&vao_cross);
  glBindVertexArray(vao_cross);
  // Generate 1 vbo buffer to store all vertices information
  glGenBuffers(1, &vbo_cross);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_cross);
  //glBufferData(GL_ARRAY_BUFFER,crossVerticesPos.size()*sizeof(GLfloat),crossVerticesPos.data(),GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER,(crossVerticesPos.size()+color_cross.size() )*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER,0,crossVerticesPos.size()*sizeof(GLfloat),crossVerticesPos.data());//upload position data
  glBufferSubData(GL_ARRAY_BUFFER,crossVerticesPos.size()*sizeof(GLfloat),color_cross.size()*sizeof(GLfloat),color_cross.data());

  glGenBuffers(1,&ebo_cross);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo_cross);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,crossIndices.size()*sizeof(GLuint),crossIndices.data(),GL_STATIC_DRAW);
  bindProgram(crossVerticesPos);//bind vao
  //unbind vao
  glBindVertexArray(0);

  //create VAO
  glGenVertexArrays(1,&vao_Tshape);
  glBindVertexArray(vao_Tshape);
  // Generate 1 vbo buffer to store all vertices information
  glGenBuffers(1, &vbo_Tshape);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_Tshape);
  //glBufferData(GL_ARRAY_BUFFER,crossVerticesPos.size()*sizeof(GLfloat),crossVerticesPos.data(),GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER,(TshapePos.size()+color_Tshape.size() )*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER,0,TshapePos.size()*sizeof(GLfloat),TshapePos.data());//upload position data
  glBufferSubData(GL_ARRAY_BUFFER,TshapePos.size()*sizeof(GLfloat),color_Tshape.size()*sizeof(GLfloat),color_Tshape.data());

  glGenBuffers(1,&ebo_Tshape);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo_Tshape);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,TshapeIndices.size()*sizeof(GLuint),TshapeIndices.data(),GL_STATIC_DRAW);
  bindProgram(TshapePos);//bind vao
  //unbind vao
  glBindVertexArray(0);
}

void GenNewRailway(){

  //clear the olde spline data
  positions.clear();
  crossVerticesPos_left.clear();
  crossVerticesPos_right.clear();
  crossIndices.clear();
  color_cross.clear();
  tangVec.clear();
  TshapeIndices.clear();
  TshapePos.clear();
  color_Tshape.clear();
  
  GenVerices(moveMode);
  GenCrossVertices();
  GenCrossIndices();
  initProgram();
  GenCrossBuffer(vao_cross_left,vbo_cross_left,ebo_cross_left,crossVerticesPos_left);
  GenCrossBuffer(vao_cross_right,vbo_cross_right,ebo_cross_right,crossVerticesPos_right);	
}
//////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////
//////Generate Texture Program and bind it//////////
void InitTexProgram(){
  // initialize shader pipeline program for textures
  tex2DPipeline = new SecondPipelineProgram();
  if(tex2DPipeline->Init(shaderBasePath,0)!=0){
    cout << "Error finding the shaderBasePath " << shaderBasePath << "." << endl;
    exit(EXIT_FAILURE);
  }
  tex3DPipeline = new SecondPipelineProgram();
  if(tex3DPipeline->Init(shaderBasePath,1)!=0){
    cout << "Error finding the shaderBasePath " << shaderBasePath << "." << endl;
    exit(EXIT_FAILURE);
  }
  tex2DPipeline->Bind();
  tex2DProgram = tex2DPipeline->GetProgramHandle();
  tex3DPipeline->Bind();
  tex3DProgram = tex3DPipeline->GetProgramHandle();
}

void Bind2DTexProgram(){
  GLuint loc = glGetAttribLocation(tex2DProgram, "position"); 
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  GLuint loc2=glGetAttribLocation(tex2DProgram,"texCoord");
  glEnableVertexAttribArray(loc2);
  glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(groundPos.size()*sizeof(GLfloat)));

}

void Bind3DTexProgram(){
  GLuint loc = glGetAttribLocation(tex3DProgram, "position"); 
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  GLuint loc2=glGetAttribLocation(tex3DProgram,"texCoord");
  glEnableVertexAttribArray(loc2);
  glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(skyPos.size()*sizeof(GLfloat)));

}


///////////////////////////////////////////
//////Create Ground///////////////////////

void GenGroundVetices(){
  //first triangle
  groundPos.push_back(-57.0f);groundPos.push_back(-48.0f);groundPos.push_back(57.0f);
  groundPos.push_back(57.0f);groundPos.push_back(-48.0f);groundPos.push_back(57.0f);
  groundPos.push_back(-57.0f);groundPos.push_back(-48.0f);groundPos.push_back(-57.0f);

  //secondTriangle
  groundPos.push_back(57.0f);groundPos.push_back(-48.0f);groundPos.push_back(57.0f);
  groundPos.push_back(-57.0f);groundPos.push_back(-48.0f);groundPos.push_back(-57.0f);
  groundPos.push_back(57.0f);groundPos.push_back(-48.0f);groundPos.push_back(-57.0f);
}

void GenGroundUV(){
  //(0,0)
  groundUVs.push_back(0.0f);groundUVs.push_back(0.0f);
  //(1,1)
  groundUVs.push_back(1.0f);groundUVs.push_back(0.0f);
  //(1,0)
  groundUVs.push_back(0.0f);groundUVs.push_back(1.0f);

  //(0,0)
  groundUVs.push_back(1.0f);groundUVs.push_back(0.0f);
  //(0,1)
  groundUVs.push_back(0.0f);groundUVs.push_back(1.0f);
  //(1,1)
  groundUVs.push_back(1.0f);groundUVs.push_back(1.0f);  

}

void GenGroundBuffer(){
  //create VAO
  glGenVertexArrays(1,&vao_groundTex);
  glBindVertexArray(vao_groundTex);
  //create VBO
  // Generate 1 vbo buffer to store all vertices information
  glGenBuffers(1, &vbo_groundTex);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_groundTex);
  //glBufferData(GL_ARRAY_BUFFER,groundPos.size()*sizeof(GLfloat),groundPos.data(),GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER,(groundPos.size()+groundUVs.size())*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER,0,groundPos.size()*sizeof(GLfloat),groundPos.data());//upload position data
  glBufferSubData(GL_ARRAY_BUFFER,groundPos.size()*sizeof(GLfloat),groundUVs.size()*sizeof(GLfloat),groundUVs.data());//upload UV data*/
  Bind2DTexProgram();

  glBindVertexArray(0);
}

void GroundTexInit(){
  glGenTextures(1,&groundTexHandle);
  if(initTexture("../Texture/groundTex.jpg",groundTexHandle)!=0){
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

}
///////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////
//////////////Create Skybox///////////////////

void GenSkyVertices(){
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(-49); 
  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(-49);
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(-49);
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(-49);
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(-49);
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(-49);

  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(49);
  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(-49);
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(-49); 
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(-49);
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(49);
  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(49);

  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(-49);
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(49);
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(49); 
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(49);
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(-49);
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(-49);

  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(49);
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(49);
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(49); 
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(49);
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(49);
  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(49);

  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(-49);
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(-49);
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(49); 
  skyPos.push_back(49);skyPos.push_back(49);skyPos.push_back(49);
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(49);
  skyPos.push_back(-49);skyPos.push_back(49);skyPos.push_back(-49);

  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(-49);
  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(49);
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(-49); 
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(-49);
  skyPos.push_back(-49);skyPos.push_back(-49);skyPos.push_back(49);
  skyPos.push_back(49);skyPos.push_back(-49);skyPos.push_back(49);

}


void GenSkyBuffer(){
  //create VAO
  glGenVertexArrays(1,&vao_skyTex);
  glBindVertexArray(vao_skyTex);
  //create VBO
  // Generate 1 vbo buffer to store all vertices information
  glGenBuffers(1, &vbo_skyTex);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_skyTex);
  //glBufferData(GL_ARRAY_BUFFER,groundPos.size()*sizeof(GLfloat),groundPos.data(),GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER,skyPos.size()*2*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER,0,skyPos.size()*sizeof(GLfloat),skyPos.data());//upload position data
  glBufferSubData(GL_ARRAY_BUFFER,skyPos.size()*sizeof(GLfloat),skyPos.size()*sizeof(GLfloat),skyPos.data());//upload UV data*/


  Bind3DTexProgram();

  glBindVertexArray(0);
}

void SkyTexInit(){
  glGenTextures(1,&skyTexHandle);
  if(initTextureCubeMap("../Texture/blue",skyTexHandle)!=0){
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

}


///////////////////////////////////////////////////////////////////////////
void initScene(int argc, char *argv[])
{

  //initialize OpenGLMatrix 
  openGLMatrix = new OpenGLMatrix();

  // Enable depth testing, then prioritize fragments closest to the camera
  glEnable(GL_DEPTH_TEST);

  // Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //clear the window
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  GenNewRailway();
  ///////////finish creating spline/////////////


  InitTexProgram();
  ///////////create ground/////////////////////
  GenGroundVetices();
  GenGroundUV();
  GenGroundBuffer();
  GroundTexInit();

  //////////create Sky/////////////////////////
  GenSkyVertices();
  GenSkyBuffer();
  SkyTexInit();

  ////////moving camera/////////////////////
  curPo_index=0;//start from the ground
  countStep=15;

}

int main(int argc, char *argv[])
{
  if(argc<2){
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);    
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // load the splines from the provided filename
  loadSplines(argv[1]);

  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);


  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();

}
