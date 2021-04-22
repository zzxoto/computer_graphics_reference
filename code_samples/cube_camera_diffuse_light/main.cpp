#include <iostream>
#include <stdio.h>
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
  in vec3 aNormal;

  uniform mat4 modelToWorldMatrix;
  uniform mat3 normalTransformMatrix;

  layout (std140) uniform Matrices
  {
    mat4 worldToCameraMatrix;
    mat4 cameraToClipMatrix;
  };

  out vec3 position_cameraSpace;
  out vec3 normal_cameraSpace;

  void main()
  {
    normal_cameraSpace = normalTransformMatrix * aNormal;
    vec4 position_cameraSpace_ =  worldToCameraMatrix * modelToWorldMatrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    gl_Position = cameraToClipMatrix * position_cameraSpace_;
    position_cameraSpace = vec3(position_cameraSpace_);
  }
)FOO";

global const char *fragmentShaderSource = R"FOO(
  #version 330 core
  uniform vec3 diffuseColor;
  uniform vec3 lightIntensity;
  uniform vec3 ambientIntensity;
  uniform vec3 lightPosition_cameraSpace;

  in vec3 position_cameraSpace;
  in vec3 normal_cameraSpace;

  out vec4 outColor;
  void main()
  {
    vec3 lightDirection_cameraSpace = normalize(lightPosition_cameraSpace - position_cameraSpace);

    float cosAngIncedence = dot(normalize(normal_cameraSpace), lightDirection_cameraSpace);
    cosAngIncedence = clamp(cosAngIncedence, 0, 1);

    //outColor = vec4(vec3(gl_FragCoord.z), 1.0);
    outColor = vec4((diffuseColor * lightIntensity * cosAngIncedence)
               + (diffuseColor * ambientIntensity), 1.0);
  }
)FOO";

global const char *vertexShaderSource2 = R"FOO(
  #version 330 core
  in vec3 aPos;

  uniform mat4 modelToWorldMatrix;

  layout (std140) uniform Matrices
  {
    mat4 worldToCameraMatrix;
    mat4 cameraToClipMatrix;
  };

  void main()
  {
    gl_Position = cameraToClipMatrix * worldToCameraMatrix * modelToWorldMatrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);
  }
)FOO";

global const char *fragmentShaderSource2 = R"FOO(
  #version 330 core
  uniform vec3 surfaceColor;

  out vec4 outColor;
  void main()
  {
    //outColor = vec4(vec3(gl_FragCoord.z), 1.0);
    outColor = vec4(surfaceColor, 1.0);
  }
)FOO";

#define PT_A -.5f, .5f, .5f
#define PT_B .5f, .5f, .5f
#define PT_C -.5f, -.5f, .5f
#define PT_D .5f, -.5f, .5f
#define PT_E -.5f, .5f, -.5f
#define PT_F .5f, .5f, -.5f
#define PT_G -.5f, -.5f, -.5f
#define PT_H .5f, -.5f, -.5f

#define NORM_LEFT  -1.0f, .0f, .0f
#define NORM_RIGHT 1.0f,  .0f, .0f
#define NORM_UP    .0f,   1.0f, .0f
#define NORM_DOWN  .0f,  -1.0f, .0f
#define NORM_FRONT .0f,  .0f,  1.0f
#define NORM_BACK  .0f,  .0f,  -1.0f

global float cubePositions[] = 
{
  PT_A, PT_C, PT_D, PT_B,
  PT_B, PT_D, PT_H, PT_F,
  PT_E, PT_A, PT_B, PT_F,
  
  PT_A, PT_E, PT_G, PT_C,
  PT_E, PT_F, PT_H, PT_G,
  PT_G, PT_H, PT_D, PT_C
};

global float cubeNormals[] =
{
  NORM_FRONT, NORM_FRONT, NORM_FRONT, NORM_FRONT,
  NORM_RIGHT, NORM_RIGHT, NORM_RIGHT, NORM_RIGHT,
  NORM_UP, NORM_UP, NORM_UP, NORM_UP,
  
  NORM_LEFT, NORM_LEFT, NORM_LEFT, NORM_LEFT,
  NORM_BACK, NORM_BACK, NORM_BACK, NORM_BACK,
  NORM_DOWN, NORM_DOWN, NORM_DOWN, NORM_DOWN
};

global GLuint cubeIndices[] = {
  1, 3, 0, 
  1, 2, 3,
  
  5, 7, 4, 
  5, 6, 7,
  
  9, 11, 8,
  9, 10, 11,
  
  15, 12, 13,
  15, 13, 14,
  
  19, 16, 17,
  19, 17, 18,
  
  23, 20, 21,
  23, 21, 22
};

glm::vec3 cubeSurfaceColor(.3f, .3f, .3f);

global float floorPositions[] = {
  .5f, .0f, -.5f,
  .5f, .0f, .5f, 
  -.5f, .0f, .5f,
  -.5f, .0f, -.5f,
};

global GLuint floorIndices[] = {
  0, 2, 1,
  0, 3, 2
};

global float floorNormals[] =
{
  NORM_UP,
  NORM_UP,
  NORM_UP,
  NORM_UP
};

glm::vec3 floorSurfaceColor(.9f, .8f, .7f);

global GLuint floorVAO, cubeVAO, matricesUBO, bindingPointUBO;

typedef struct PointLight
{
  glm::vec3 intensity;
  glm::vec3 position;
} PointLight;

typedef struct FragmentLightingProgramData
{
  GLuint program;
  GLuint modelToWorldMatrix;
  GLuint matricesUniformBlock;
  GLuint normalTransformMatrix;
  GLuint diffuseColor;
  GLuint lightIntensity;
  GLuint ambientIntensity;
  GLuint lightPosition_cameraSpace;
  
} FragmentLightingProgramData;

typedef struct SimpleShaderProgramData
{
  GLuint program;
  GLuint modelToWorldMatrix;
  GLuint matricesUniformBlock;
  GLuint surfaceColor;
} SimpleShaderProgramData;

typedef struct Camera
{
  glm::vec3 cameraSphericalRelPos;
  glm::vec3 cameraTargetPos;
} Camera;

FragmentLightingProgramData programData_fragmentLighting;
SimpleShaderProgramData programData_simpleShader;
Camera camera;
PointLight pointLight;
glm::vec3 ambientIntensity;

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

internal FragmentLightingProgramData loadProgram_fragmentLighting(const char *vertexShaderSource, const char *fragmentShaderSource)
{
  FragmentLightingProgramData p;
  
  p.program = createShaderProgram(vertexShaderSource, fragmentShaderSource);
  p.modelToWorldMatrix  = glGetUniformLocation(p.program, "modelToWorldMatrix");
  p.normalTransformMatrix = glGetUniformLocation(p.program, "normalTransformMatrix");
  p.matricesUniformBlock = glGetUniformBlockIndex(p.program, "Matrices");
  
  p.diffuseColor   = glGetUniformLocation(p.program, "diffuseColor");
  p.lightIntensity = glGetUniformLocation(p.program, "lightIntensity");
  p.ambientIntensity = glGetUniformLocation(p.program, "ambientIntensity");
  
  p.lightPosition_cameraSpace = glGetUniformLocation(p.program, "lightPosition_cameraSpace");
  
  return p;
}

internal SimpleShaderProgramData loadProgram_simpleShader(const char *vertexShaderSource, const char *fragmentShaderSource)
{
  SimpleShaderProgramData p;
  
  p.program = createShaderProgram(vertexShaderSource, fragmentShaderSource);
  
  p.modelToWorldMatrix  = glGetUniformLocation(p.program, "modelToWorldMatrix");
  p.matricesUniformBlock = glGetUniformBlockIndex(p.program, "Matrices");
  
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
  glBufferData(GL_ARRAY_BUFFER, sizeof(floorNormals) + sizeof(floorPositions), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floorNormals), floorNormals);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(floorNormals), sizeof(floorPositions), floorPositions);
  
  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  
  GLint normalAttribLocation = glGetAttribLocation(programData_fragmentLighting.program, "aNormal");
  glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(normalAttribLocation);
  
  GLint positionAttribLocation = glGetAttribLocation(programData_fragmentLighting.program, "aPos");
  glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(floorNormals));
  glEnableVertexAttribArray(positionAttribLocation);
  
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  floorVAO = VAO;
}

internal void initCube(void)
{
  GLuint VBO, EBO, VAO;
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubePositions) + sizeof(cubeNormals), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cubeNormals), cubeNormals);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(cubeNormals), sizeof(cubePositions), cubePositions);
  
  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  
  {
    GLint normalAttribLocation = glGetAttribLocation(programData_fragmentLighting.program, "aNormal");
    glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(normalAttribLocation);
    
    GLint positionAttribLocation = glGetAttribLocation(programData_fragmentLighting.program, "aPos");
    glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(cubeNormals));
    glEnableVertexAttribArray(positionAttribLocation);
  }
  
  {
    GLint positionAttribLocation = glGetAttribLocation(programData_simpleShader.program, "aPos");
    glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(cubeNormals));
    glEnableVertexAttribArray(positionAttribLocation);
  }
  
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  cubeVAO = VAO;
}

internal void initUBO()
{
  //initialize buffer object
  glGenBuffers(1, &matricesUBO);
  glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
  glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  
  //bind the shader's uniform reference to a binding point
  glUniformBlockBinding(programData_fragmentLighting.program, 
                        programData_fragmentLighting.matricesUniformBlock, bindingPointUBO);
  glUniformBlockBinding(programData_simpleShader.program, 
                        programData_simpleShader.matricesUniformBlock, bindingPointUBO);
  
  //bind buffer object to a binding point
  glBindBufferRange(GL_UNIFORM_BUFFER, bindingPointUBO, matricesUBO, 0, 2 * sizeof(glm::mat4));
}

internal void init(void)
{
  camera.cameraSphericalRelPos = glm::vec3(90.0f, 45.0f, 50.0f);
  camera.cameraTargetPos = glm::vec3(.0f, .0f, .0f);
  
  pointLight.intensity = glm::vec3(.8f, .8f, .8f);
  pointLight.position  = glm::vec3(5.0f, 10.0f, 4.0f);
  
  ambientIntensity = glm::vec3(.20f, .20f, .20f);
  
  bindingPointUBO = 2;
  
  programData_fragmentLighting = loadProgram_fragmentLighting(vertexShaderSource, fragmentShaderSource);
  programData_simpleShader = loadProgram_simpleShader(vertexShaderSource2, fragmentShaderSource2);
  
  initUBO();
  initFloor();
  initCube();
}

internal void drawFloor(const glm::mat4 &cameraMatrix, MatrixStack &modelMatrix)
{
  PushStack push(modelMatrix);
  modelMatrix.Scale(glm::vec3(50.0f, 1.0f, 50.0f));
  
  glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(cameraMatrix * modelMatrix.Top())));
  
  glUseProgram(programData_fragmentLighting.program);
  glUniformMatrix4fv(programData_fragmentLighting.modelToWorldMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
  glUniformMatrix3fv(programData_fragmentLighting.normalTransformMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
  glUniform3f(programData_fragmentLighting.diffuseColor, floorSurfaceColor.x, floorSurfaceColor.y, floorSurfaceColor.z);
  
  glBindVertexArray(floorVAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  
  glUseProgram(0);
  glBindVertexArray(0);
}

internal void drawCube(const glm::mat4 &cameraMatrix, MatrixStack &modelMatrix)
{
  PushStack push(modelMatrix);
  modelMatrix.Scale(8.0f);
  
  glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(cameraMatrix * modelMatrix.Top())));
  
  glUseProgram(programData_fragmentLighting.program);
  glUniformMatrix4fv(programData_fragmentLighting.modelToWorldMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
  glUniformMatrix3fv(programData_fragmentLighting.normalTransformMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
  glUniform3f(programData_fragmentLighting.diffuseColor, cubeSurfaceColor.x, cubeSurfaceColor.y, cubeSurfaceColor.z);
  
  glBindVertexArray(cubeVAO);
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
  
  glUseProgram(0);
  glBindVertexArray(0);
}

internal void drawLightSource(const glm::mat4 &cameraMatrix, MatrixStack &modelMatrix)
{
  PushStack push(modelMatrix);
  modelMatrix.Translate(pointLight.position.x, pointLight.position.y, pointLight.position.z);
  //modelMatrix.Scale(.8f);
  
  glUseProgram(programData_simpleShader.program);
  glUniformMatrix4fv(programData_simpleShader.modelToWorldMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
  glUniform3f(programData_simpleShader.surfaceColor, pointLight.intensity.x, pointLight.intensity.y, pointLight.intensity.z);
  
  glBindVertexArray(cubeVAO);
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
  
  glUseProgram(0);
  glBindVertexArray(0);
}

internal void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glm::mat4 cameraMatrix = calcLookAtMatrix(camera);
  glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(cameraMatrix));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  
  glUseProgram(programData_fragmentLighting.program);
  glm::vec3 &intensity = pointLight.intensity;
  glm::vec3 lightPosition_cameraSpace = glm::vec3(cameraMatrix * glm::vec4(pointLight.position, 1.0f));
  
  glUniform3f(programData_fragmentLighting.lightIntensity, intensity.x, intensity.y, intensity.z);
  glUniform3f(programData_fragmentLighting.ambientIntensity, ambientIntensity.x, ambientIntensity.y, ambientIntensity.z);
  glUniform3f(programData_fragmentLighting.lightPosition_cameraSpace, lightPosition_cameraSpace.x, lightPosition_cameraSpace.y, lightPosition_cameraSpace.z);
  glUseProgram(0);
  
  MatrixStack modelMatrix;
  {
    glDepthMask(GL_FALSE);
    drawFloor(cameraMatrix, modelMatrix);
    glDepthMask(GL_TRUE);
  }
  
  {
    PushStack push(modelMatrix);
    modelMatrix.Translate(0, 1.0f, 0.0f);
    drawCube(cameraMatrix, modelMatrix);
  }
  
  {
    drawLightSource(cameraMatrix, modelMatrix);
  }
  
  glutSwapBuffers();
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
  glutPostRedisplay();
}

internal void reshape(int w, int h)
{
  MatrixStack mat;
  mat.Perspective(45.0f, ZNEAR, ZFAR);
  
  glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
  glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(mat.Top()));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  
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
    cout << "OpenGL 3.3 not supported\n";
  }
  
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  //[near, far] in NDC is [1, -1] as opposed to the default [-1, 1]
  //this is due the (F - N) in denominator in perspective matrix
  glDepthRange(1.0f, 0.0f);
  
  glClearColor(.1f, .2f, .2f, 1.0f);
  glClearDepth(1.0f);
  
  init();
  
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutReshapeFunc(reshape);
  glutMainLoop();
  
  return 0;
}