#ifndef ZZXOTOGL_HELPER_H
#define ZZXOTO_HELPER_H
#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>

using std::cout;
using std::endl;

GLuint createShaderProgram(const char *vertexShaderSource, const char *fragmentShaderSource)
{
  GLuint vertexShader, fragmentShader, program;
  
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

#endif
