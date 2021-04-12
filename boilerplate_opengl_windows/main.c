#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

#define internal static

static const int windowH = 480;
static const int windowW = 640;

internal void display(void)
{
  glutPostRedisplay();
}

int main(int argc, char **argv)
{
  printf("Hello world\n");
  //init glut
  glutInit(&argc, argv);
  
  //init context
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  
  //init window
  //NOTE(Abhaya): glewInit before gluInitDisplayMode call, reports an error
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
  glutInitWindowSize(windowH, windowH);
  glutCreateWindow("OpenGL");
  
  //glew extension wrangling
  GLenum glewError = glewInit();
  if (glewError != GLEW_OK)
  {
    printf("Error initializing GlEW: %s\n", glewGetErrorString(glewError));
    return 1;
  }
  
  if (!GLEW_VERSION_3_3)
  {
    printf("OpenGL 3.1 not supported\n");
    return 1;
  }
  
  glutDisplayFunc(display);
  glutMainLoop();
  
  return 0;
}
