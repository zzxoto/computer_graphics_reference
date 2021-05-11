#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb/stb_truetype.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <zzxoto/helper.h>
#include <zzxoto/gl_helper.h>
#include <iostream>

using std::cout;
using std::endl;

typedef unsigned char uchar;

typedef struct FontMetrics
{
  int ascent;   //max distance from the baseline to top of bbox, across all the glyphs of the font. +ve.
  int descent;  //max distance from the baseline to bottom of bbox, across all the glyphs of the font. -ve.
  int lineHeight;//Line Height. This value little more than ascent - descent. +ve. When breaking a new line, do baeline += lineHeight;
} FontMetrics;

typedef struct CharacterFontInfo
{
  //bbox is the smallest box i.e. the box with the least area 
  //that bounds the glyph.
  
  //If the font were being drawn on the paper, the penPosition would be the 
  //horizontal position of the pen on standby about to draw.
  
  uchar character;
  int advanceWidth; //move penPosition by this distance, to draw next character. +ve
  int topBearing;   //distance from baseline to top of bbox. +ve
  int leftBearing;  //distance from origin to left of bbox.  +ve
  GLuint textureId; //OpenGL texture Id from `glGenTextures` call.
  int w;            //width of the texture/bbox/glyph. +ve
  int h;            //height of the texture/bbox/glyph. +ve
} CharacterInfo;

typedef struct Font
{
  FontMetrics *fontMetrics;
  stbtt_fontinfo *stbFont;
  int fontHeightPx;
  int characterFontInfoCount;
  CharacterFontInfo *characterFontInfo;
} Font;
Font *g_font;

static const int FPS = 60;
static const int DELAYMS = 1000 / FPS;
static int g_frames = 0;
static int g_windowH = 600, g_windowW = 600;
static int g_displayTextLeft = 100, g_displayTextTop = 100;

//static char g_displayTextBuffer[1 << 10];
static const char *g_displayTextBuffer = R"FOO(The Quick 
    Brown 
 F_O_X

Jumps          Over 
      T h   e 

Lazy  DOg
)FOO";

static glm::mat4 g_worldToClipMatrix(1);
static glm::mat4 g_modelMatrix(1);

static float g_zRotation  = 0;
static bool g_shouldSpin = true;

uchar characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567891 !@#$%^&*()-_+=.";

uchar ttfBuffer[1 << 22];

Font *initFont(char *ttfFilename, int fontHeightPx)
{
  Font *myFont;
  //1. load ttf content to buffer
  FILE *fp = fopen(ttfFilename, "r");
  if (fp == NULL)
  {
    cout << "Failed to load font" << endl;
  }
  else
  {
    fread(ttfBuffer, 1, 1 << 22, fp);
    fclose(fp);
    
    //2. init stb font
    stbtt_fontinfo *stbtt_font = (stbtt_fontinfo *) malloc(sizeof(stbtt_fontinfo));
    stbtt_InitFont(stbtt_font, ttfBuffer, stbtt_GetFontOffsetForIndex(ttfBuffer, 0));
    
    //3. scale factor given the desired font height in pixel
    float scale = stbtt_ScaleForPixelHeight(stbtt_font, fontHeightPx);
    
    int charactersN = sizeof(characters);
    
    CharacterFontInfo *characterFontInfo = (CharacterFontInfo *) malloc(charactersN * sizeof(CharacterFontInfo));
    
    GLint oldAlign = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlign);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    uchar *bitmapTempBuffer = (uchar *) malloc(1 << 16);
    
    for (int i = 0; i < charactersN; i++)
    {
      //4. character font bitmap and metrics
      uchar c = characters[i];
      CharacterFontInfo *cFontInfo = characterFontInfo + i;
      
      {
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(stbtt_font, c, scale, scale, &x0, &y0, &x1, &y1);
        cFontInfo->w = x1 - x0;
        cFontInfo->h = y1 - y0;
        cFontInfo->topBearing = -y0;
      }
      
      stbtt_MakeCodepointBitmap(stbtt_font, 
                                bitmapTempBuffer, 
                                cFontInfo->w, 
                                cFontInfo->h, 
                                cFontInfo->w,
                                scale, 
                                scale, 
                                c);
      cFontInfo->character = c;
      glGenTextures(1, &cFontInfo->textureId);
      glBindTexture(GL_TEXTURE_2D, cFontInfo->textureId);
      glTexImage2D(GL_TEXTURE_2D,
                  0,
                  GL_R8,
                  cFontInfo->w,
                  cFontInfo->h,
                  0,
                  GL_RED,
                  GL_UNSIGNED_BYTE,
                  bitmapTempBuffer);  
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      
      int advanceWidth, leftBearing;
      stbtt_GetCodepointHMetrics(stbtt_font, c, &advanceWidth, &leftBearing);
      cFontInfo->advanceWidth = (int) advanceWidth * scale;
      cFontInfo->leftBearing = (int) leftBearing * scale;
    }
    free(bitmapTempBuffer);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, oldAlign);
    
    //5. Font metrics
    int ascent, descent, lineGap;
    FontMetrics *fontMetrics = (FontMetrics *) malloc(sizeof(FontMetrics));
    stbtt_GetFontVMetrics(stbtt_font, &ascent, &descent, &lineGap);
    fontMetrics->ascent = (int) ascent * scale;
    fontMetrics->descent = (int) descent * scale;
    fontMetrics->lineHeight = (int) (ascent - descent + lineGap) * scale;
    
    //6. pack and return pointer;
    int characterMetricsCount = charactersN;
    int characterBitmapCount = charactersN;
    myFont = (Font *) malloc(sizeof(Font));
    myFont->stbFont = stbtt_font;
    myFont->fontMetrics = fontMetrics;
    myFont->fontHeightPx = fontHeightPx;
    myFont->characterFontInfoCount = charactersN;
    myFont->characterFontInfo = characterFontInfo;
  }
  
  return myFont;
}

const char *pixelCoordVertexShader = R"FOO(
#version 330 core
in vec2 aPos;
in vec2 textureCoord;

uniform mat4 worldToClipMatrix;
uniform mat4 modelMatrix;

out vec2 uvCoord;

void main()
{
  gl_Position = worldToClipMatrix * modelMatrix * vec4(aPos.x, aPos.y, 1.0, 1.0);
  uvCoord = textureCoord;
}
)FOO";

const char *fontFragShader = R"FOO(
#version 330 core
uniform sampler2D myTexture;
uniform vec3 fontColor;

in vec2 uvCoord;

out vec4 outColor;

void main()
{
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(myTexture, uvCoord).r * 2);
  outColor = vec4(fontColor, 1.0) * sampled;
}
)FOO";

typedef struct ProgramData
{
  GLuint program;
  GLuint worldToClipMatrix;
  GLuint modelMatrix;
  GLuint fontColor;
  GLuint sampler;
} ProgramData;
ProgramData g_programData;
GLuint g_VBO, g_VAO, g_textureObject, g_textureUnit = 3;

ProgramData initProgram(const char *vertexShader, const char *fragmentShader)
{
  ProgramData p;
  p.program = createShaderProgram(vertexShader, fragmentShader);
  p.worldToClipMatrix = glGetUniformLocation(p.program, "worldToClipMatrix");
  p.modelMatrix = glGetUniformLocation(p.program, "modelMatrix");
  p.fontColor = glGetUniformLocation(p.program, "fontColor");
  p.sampler = glGetUniformLocation(p.program, "myTexture");
  
  return p;  
}

void initShaderData(Font *font)
{
  GLuint EBO;
  glGenBuffers(1, &g_VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &g_VAO);
  glGenTextures(1, &g_textureObject);
  
  GLushort indices[] = {
    2, 0, 1,
    2, 1, 3
  };
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  
  glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLushort) * 16, NULL, GL_DYNAMIC_DRAW);
  
  glBindVertexArray(g_VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  GLuint positionAttribLocation = glGetAttribLocation(g_programData.program, "aPos");    
  glVertexAttribPointer(positionAttribLocation, 2, GL_UNSIGNED_SHORT, GL_FALSE, 8, (void *) 0);
  glEnableVertexAttribArray(positionAttribLocation);
  
  GLuint textureCoordAttribLocation = glGetAttribLocation(g_programData.program, "textureCoord");    
  glVertexAttribPointer(textureCoordAttribLocation, 2, GL_UNSIGNED_SHORT, GL_TRUE, 8, (void *) 4);
  glEnableVertexAttribArray(textureCoordAttribLocation);
  
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glUseProgram(g_programData.program);
  glUniformMatrix4fv(g_programData.worldToClipMatrix, 1, GL_FALSE, glm::value_ptr(g_worldToClipMatrix));
  glUniformMatrix4fv(g_programData.modelMatrix, 1, GL_FALSE, glm::value_ptr(g_modelMatrix));
  glUseProgram(0);
}

void init()
{
  g_programData = initProgram(pixelCoordVertexShader, fontFragShader);
  g_font = initFont("shared/data/arial.ttf", 30);
  initShaderData(g_font);
}

//assumes program is bound
void displayText(Font *font, const char *text, int left, int top)
{
  char c = *text;
  
  int baseline = top + font->fontMetrics->ascent;
  int penPosition = left;
  
  glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
  glBindVertexArray(g_VAO);
  while (c != 0)
  {
    CharacterFontInfo *characterFontInfo = NULL;
    for (int i = 0; i < font->characterFontInfoCount; i++)
    {
      CharacterFontInfo *cFontInfo = font->characterFontInfo + i;
      if (cFontInfo->character == c)
      {
        characterFontInfo = cFontInfo;
        break;
      }
    }
    
    if (characterFontInfo)
    {
      int xpos = penPosition + characterFontInfo->leftBearing;
      int ypos = baseline - characterFontInfo->topBearing;
      
      int w = characterFontInfo->w;
      int h = characterFontInfo->h;
      
      // a   b
      //  [ ]
      // c   d
      //a => xpos,     ypos
      //b => xpos + w, ypos
      //c => xpos,     ypos + h
      //d => xpos + w, ypos + h
      GLushort vertices[] = {
        xpos,     ypos,      0,     0,
        xpos + w, ypos,      65535, 0,
        xpos,     ypos + h,  0,     65535,
        xpos + w, ypos + h,  65535, 65535
      };
      
      glActiveTexture(GL_TEXTURE0 + g_textureUnit);
      glBindTexture(GL_TEXTURE_2D, characterFontInfo->textureId);
      
      glUniform1i(g_programData.sampler, g_textureUnit);      
      
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLushort) * 16, vertices);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
      
      penPosition = (int) penPosition + characterFontInfo->advanceWidth;
    }
    else if (c == '\n')
    {
      //go to next line and set penPosition to initial left position
      baseline = baseline + font->fontMetrics->lineHeight;
      penPosition = left;
    }
    
    text++;
    c = *text;
  }
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}


//assumes program is bound
void layoutText(Font *font, const char *text, int left, int top, int *right, int *bottom)
{
  char c = *text;
  
  int penPosition = left;
  
  int w = 0;
  int h = 0;
  
  while (c != 0)
  {
    CharacterFontInfo *characterFontInfo = NULL;
    for (int i = 0; i < font->characterFontInfoCount; i++)
    {
      CharacterFontInfo *cFontInfo = font->characterFontInfo + i;
      if (cFontInfo->character == c)
      {
        characterFontInfo = cFontInfo;
        break;
      }
    }
    
    if (characterFontInfo)
    {
      penPosition += characterFontInfo->advanceWidth;
    }
    else if (c == '\n')
    {
      //go to next line and set penPosition to initial left position
      h += font->fontMetrics->lineHeight;
      if (penPosition - left > w)
      {
        w = penPosition - left;
      }
      penPosition = left;
    }
    
    text++;
    c = *text;
  }
  
  if (w == 0 && penPosition - left > 0)
  {
    w = penPosition - left;
  }
  
  if (w > 0 && h == 0)
  {
    h = font->fontMetrics->lineHeight;
  }
  
  *right = left + w;
  *bottom = top + h;
}

static void update()
{
  int textLayoutW = 0, textLayoutH = 0;
  layoutText(g_font, g_displayTextBuffer, 0, 0, &textLayoutW, &textLayoutH);
  
  if (g_shouldSpin)
  {
    g_zRotation += 1.0f;
    
    float translateX = g_displayTextLeft + textLayoutW / 2;
    float translateY = g_displayTextTop + textLayoutH / 2;
    
    MatrixStack mat;
    mat.Translate(translateX, translateY, 0);
    //For top left coordinate, rotation around -Z axis for CCW
    //cupping direction
    mat.Rotate(g_zRotation, glm::vec3(0.f, 0.f, -1.f));
    
    mat.Translate(-translateX, -translateY, 0);
    g_modelMatrix = mat.Top();
    
    glUseProgram(g_programData.program);
    glUniformMatrix4fv(g_programData.modelMatrix, 1, GL_FALSE, glm::value_ptr(g_modelMatrix));
    glUseProgram(0);
  }
}

static void display()
{
  glClearColor(.1f, .2f, .2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glUseProgram(g_programData.program);
  
  glUniform3f(g_programData.fontColor, .8f, .8f, .8f);
  
  displayText(g_font, g_displayTextBuffer, g_displayTextLeft, g_displayTextTop);
  
  glUseProgram(0);
  
  glutSwapBuffers();
}

static void reshape(int w, int h)
{
  g_windowH = h;
  g_windowW = w;
  
  
  MatrixStack mat;
  mat.Translate(-1.0f, 1.0f, 0.0f);
  mat.Scale(glm::vec3(2.0f / g_windowW, -2.0f / g_windowH, 1.0f));  
  
  g_worldToClipMatrix = mat.Top();
  
  glUseProgram(g_programData.program);
  glUniformMatrix4fv(g_programData.worldToClipMatrix, 1, GL_FALSE, glm::value_ptr(g_worldToClipMatrix));
  glUseProgram(0);
  
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}

static bool g_gameLoopContinues = true;
void runGameLoop(int val)
{
  g_frames++;
  
  update();
  display();
  
  if (g_gameLoopContinues)
  {
    glutTimerFunc(DELAYMS, runGameLoop, val);
  }
}

void exitGameLoop()
{
  g_gameLoopContinues = false;
}


static void keyboard(uchar key, int x, int y)
{
  switch(key)
  {
    case 27:
    {
      glutLeaveMainLoop();
      exitGameLoop();
      break;
    }
    case ' ':
    {
      g_shouldSpin = !g_shouldSpin;
      break;
    }
    
  }
}

int main(int argc, char **argv)
{
  //init glut
  glutInit(&argc, argv);
  
  //init context
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  
  //init window
  //NOTE(Abhaya): glewInit before gluInitDisplayMode call, reports an error
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
  glutInitWindowSize(g_windowW, g_windowH);
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
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
  
  init();
  
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  
  runGameLoop(0);
  
  glutMainLoop();
  
  return 0;
}