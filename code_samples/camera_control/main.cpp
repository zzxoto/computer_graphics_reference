#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "zzxoto/helper.h"
#include "math.h"

#define internal static
#define global static

using std::cout;
using std::endl;

typedef unsigned int uint;

global const int windowH = 640;
global const int windowW = 640;
global const float ZNEAR = .1f;
global const float ZFAR = 100.0f;

global const char *vertexShaderSource = R"FOO(
  #version 330 core
  in vec3 aPos;

  uniform mat4 modelToWorldMatrix;
  uniform mat4 worldToCameraMatrix;
  uniform mat4 cameraToClipMatrix;

  void main()
  {
    gl_Position = cameraToClipMatrix * worldToCameraMatrix * modelToWorldMatrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);
  }
)FOO";

global const char *fragmentShaderSource = R"FOO(
  #version 330 core
  uniform vec3 surfaceColor;

  out vec4 outColor;
  void main()
  {
    outColor = vec4(surfaceColor, 1.0f);
  }
)FOO";

global float floorVertices[] = {
  .5f, .0f, -.5f,
  .5f, .0f, .5f, 
  -.5f, .0f, .5f,
  -.5f, .0f, -.5f,
};

global GLuint floorIndices[] = {
  0, 1, 2,
  0, 2, 1,
  2 ,3, 0,
  2, 0, 3
};

glm::vec3 floorSurfaceColor(.9f, .8f, .7f);

global GLuint floorVAO;

typedef struct ProgramData
{
  GLuint program;
  GLuint modelToWorldMatrixUnif;
  GLuint worldToCameraMatrixUnif;
  GLuint cameraToClipMatrixUnif;
  GLuint surfaceColor;
} ProgramData;

typedef struct Camera
{
  glm::vec3 cameraSphericalRelPos;
  glm::vec3 cameraTargetPos;
} Camera;

ProgramData programData;
Camera camera;

internal glm::vec3 resolveCameraPosition_sphericalToEuclidean(const Camera &camera)
{
  float theta = glm::radians(camera.cameraSphericalRelPos.x);
  float phi = glm::radians(camera.cameraSphericalRelPos.y);
  float r = camera.cameraSphericalRelPos.z;
  
  float cosTheta = cosf(theta);
  float sinTheta = sinf(theta);
  float sinPhi   = sinf(phi);
  float cosPhi   = cosf(phi);
  
  glm::vec3 cameraRelPos_euclidean = glm::vec3(r * sinPhi * cosTheta, r * cosPhi, r * sinPhi * sinTheta);
  return cameraRelPos_euclidean + camera.cameraTargetPos;
}

internal glm::mat4 calcLookAtMatrix(const Camera &camera)
{
  const glm::vec3 upPt = glm::vec3(.0f, 1.0f, .0f);
  const glm::vec3 &lookPt = camera.cameraTargetPos;
  const glm::vec3 &cameraPt = resolveCameraPosition_sphericalToEuclidean(camera);
  
  glm::vec3 lookDir = glm::normalize(lookPt - cameraPt);
  glm::vec3 upDir = glm::normalize(upPt);
  
  glm::vec3 rightDir = glm::normalize(glm::cross(lookDir, upDir));
  glm::vec3 perpUpDir = glm::cross(rightDir, lookDir);
  
  glm::mat4 rotMat(1.0f);
  rotMat[0] = glm::vec4(rightDir, 0.0f);
  rotMat[1] = glm::vec4(perpUpDir, 0.0f);
  rotMat[2] = glm::vec4(-lookDir, 0.0f);
  
  rotMat = glm::transpose(rotMat);
  
  glm::mat4 transMat(1.0f);
  transMat[3] = glm::vec4(-cameraPt, 1.0f);
  
  return rotMat * transMat;
}

internal GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
{
  uint vertexShader, fragmentShader, program;
  
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  
  {
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
      glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
      cout << infoLog << endl;
    }
  }
  
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  
  {
    int success;
    char infoLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
      glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      cout << infoLog << endl;
    }
  }
  
  program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);
  {
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
      glGetProgramInfoLog(program, 512, NULL, infoLog);
      cout << infoLog << endl;
    }
  }
  
  return program;
}

internal ProgramData loadProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
{
  ProgramData p;
  p.program = createShaderProgram(vertexShaderSource, fragmentShaderSource);
  p.modelToWorldMatrixUnif = glGetUniformLocation(p.program, "modelToWorldMatrix");
  p.worldToCameraMatrixUnif = glGetUniformLocation(p.program, "worldToCameraMatrix");
  p.cameraToClipMatrixUnif = glGetUniformLocation(p.program, "cameraToClipMatrix");
  p.surfaceColor = glGetUniformLocation(p.program, "surfaceColor");
  
  return p;
}

internal void initFloor(void)
{
  GLuint VBO, EBO, VAO;
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
  
  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  
  GLint positionAttribLocation = glGetAttribLocation(programData.program, "aPos");
  glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(positionAttribLocation);
  
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  floorVAO = VAO;
}

internal void init(void)
{
  camera.cameraSphericalRelPos = glm::vec3(90.0f, 45.0f, 50.0f);
  camera.cameraTargetPos = glm::vec3(.0f, .0f, .0f);
  programData = loadProgram(vertexShaderSource, fragmentShaderSource);
  initFloor();
}

internal void display(void)
{
  glClearColor(.1f, .2f, .2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  //set camera transformation
  glm::mat4 cameraMatrix = calcLookAtMatrix(camera);
  glUseProgram(programData.program);
  glUniformMatrix4fv(programData.worldToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(cameraMatrix));
  glUseProgram(0);
  
  //draw floor
  {
    MatrixStack modelMatrix;
    modelMatrix.Scale(glm::vec3(50.0f, 0.0f, 50.0f));
    
    glUseProgram(programData.program);
    glUniformMatrix4fv(programData.modelToWorldMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
    glUniform3f(programData.surfaceColor, floorSurfaceColor.x, floorSurfaceColor.y, floorSurfaceColor.z);
    
    glBindVertexArray(floorVAO);
    glDrawElements(GL_TRIANGLES, sizeof(floorIndices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);
    
    glUseProgram(0);
    glBindVertexArray(0);  
  }
  
  glutSwapBuffers();
  glutPostRedisplay();
}

internal void keyboard(unsigned char key, int x, int y)
{
  switch(key)
  {
    case 27: 
    {
      glutLeaveMainLoop();
      break;
    }
    case 'w': 
    {
      camera.cameraTargetPos.z -= 4.0f; 
      break;
    }
    case 's': 
    {
      camera.cameraTargetPos.z += 4.0f; 
      break;
    }
    case 'd': 
    {
      camera.cameraTargetPos.x += 4.0f; 
      break;
    }
    case 'a': 
    {
      camera.cameraTargetPos.x -= 4.0f; 
      break;
    }
    case 'e': 
    {
      camera.cameraTargetPos.y -= 4.0f; 
      break;
    }
    case 'q': 
    {
      camera.cameraTargetPos.y += 4.0f; 
      break;
    }
    case 'j':
    {
      camera.cameraSphericalRelPos.x -= 3.0f;
      break;
    }
    case 'l':
    {
      camera.cameraSphericalRelPos.x += 3.0f;
      break;
    }
    case 'i':
    {
      camera.cameraSphericalRelPos.y -= 1.0f;
      break;
    }
    case 'k':
    {
      camera.cameraSphericalRelPos.y += 1.0f;
      break;
    }
  }
  
  camera.cameraSphericalRelPos.y = glm::clamp(camera.cameraSphericalRelPos.y, 1.0f, 80.0f);
}

internal void reshape(int w, int h)
{
  MatrixStack mat;
  mat.Perspective(45.0f, ZNEAR, ZFAR);
  
  glUseProgram(programData.program);
  glUniformMatrix4fv(programData.cameraToClipMatrixUnif, 1, GL_FALSE, glm::value_ptr(mat.Top()));
  glUseProgram(0);
  
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  glutPostRedisplay();
}

int main(int argc, char **argv)
{
  //glut init
  glutInit(&argc, argv);
  
  //init context
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  
  //init window
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
  glutInitWindowSize(windowW, windowH);
  
  glutCreateWindow("OpenGL");
  
  GLenum glewError = glewInit();
  if (glewError != GLEW_OK)
  {
    cout << "Error initializing Glew: " << glewGetErrorString(glewError) << endl;
  }
  
  if (!GLEW_VERSION_3_3)
  {
    cout << "OpenGL 3.3 not supported" << endl;
  }
  
  init();
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutReshapeFunc(reshape);
  glutMainLoop();
  
  return 0;
}