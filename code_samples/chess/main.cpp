#define STBTT_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stb/stb_image.h>
#include <stdio.h>
#include <zzxoto/helper.h>
#include <zzxoto/gl_helper.h>

typedef unsigned char uchar;

typedef struct ProgramData
{
  GLuint program;
  GLuint worldToClipMatrix;
  GLuint sampler;
  GLuint transparentPixel;
  GLuint color;
} ProgramData;
ProgramData g_chessPieceProgramData, g_chessBoardProgramData;

typedef struct Texture
{
  int w;
  int h;
  GLuint textureId;
} Texture;

static const int VERTICES_COUNT = 16;
typedef struct RenderData
{
  GLfloat vertices[VERTICES_COUNT];
  GLuint textureId;
  glm::vec3 color;
  ProgramData *programData;
  bool shouldRender;
} RenderData;

static const int RENDERDATA_COUNT = 33;
RenderData g_renderData[RENDERDATA_COUNT];

typedef enum ChessUnit
{
  cu_w_knight=1,
  cu_w_bishop,
  cu_w_rook,
  cu_w_queen,
  cu_w_king,
  cu_w_pawn,
  cu_b_knight,
  cu_b_bishop,
  cu_b_rook,
  cu_b_queen,
  cu_b_king,
  cu_b_pawn
} ChessUnit;

typedef struct ChessPiece {
  int x;  //[0, 7]
  int y;  //[0, 7]
  ChessUnit unit;
  bool active;
} ChessPiece;

static const int CHESSPIECE_COUNT = 32;

struct 
{
  ChessPiece chessPieces[CHESSPIECE_COUNT];
} g_chessState;

Texture tx_chessBoard;
Texture tx_knight;
Texture tx_bishop;
Texture tx_rook;
Texture tx_queen;
Texture tx_king;
Texture tx_pawn;

static bool isBlack(const ChessPiece &chessPiece)
{
  bool result = false;
  switch(chessPiece.unit)
  {
    case cu_b_knight:
    case cu_b_bishop:
    case cu_b_rook:
    case cu_b_queen:
    case cu_b_king:
    case cu_b_pawn:
    {
      result = true;
    }
  }
  
  return result;
}

static GLuint getChessPieceTextureId(const ChessPiece &chessPiece)
{
  GLuint textureId = 0;
  
  switch(chessPiece.unit)
  {
    case cu_w_knight:
    case cu_b_knight:
    {
      textureId = tx_knight.textureId;
    }
    break;
    case cu_w_bishop:
    case cu_b_bishop:
    {
      textureId = tx_bishop.textureId;
    }
    break;
    case cu_w_rook:
    case cu_b_rook:
    {
      textureId = tx_rook.textureId;
    }
    break;
    case cu_w_queen:
    case cu_b_queen:
    {
      textureId = tx_queen.textureId;
    }
    break;
    case cu_b_king:
    case cu_w_king:
    {
      textureId = tx_king.textureId;
    }
    break;
    case cu_b_pawn:
    case cu_w_pawn:
    {
      textureId = tx_pawn.textureId;
    }
  }
  
  return textureId;
}

static void init();
static void keyboard(uchar key, int x, int y);
static void runGameLoop(int val);
static void exitGameLoop();
static void update();
static void display();
static void reshape(int w, int h);

static int g_windowH = 600, g_windowW = 600;
static const int FPS = 60;
static const int DELAYMS = 1000 / FPS;
static bool g_gameLoopContinues = true;
static const GLuint g_textureUnit = 3;
static GLuint g_VBO, g_VAO, g_EBO;
static glm::mat4 g_worldToClipMatrix(1);

static glm::vec3 g_transparentPixel(0.f, 187.0f/255.0f, 0.f); 
static glm::vec3 g_colorChessBlack(.1f, .1f, .1f);  
static glm::vec3 g_colorChessWhite(.7f,.7f, .7f);

const char *vertexShader = R"FOO(
#version 330 core
in vec2 aPos;
in vec2 textureCoord;

uniform mat4 worldToClipMatrix;

out vec2 uvCoord;

void main()
{
  gl_Position = worldToClipMatrix * vec4(aPos.x, aPos.y, 1.0, 1.0);
  uvCoord = textureCoord;
}
)FOO";

const char *chessPieceFragmentShader = R"FOO(
#version 330 core
uniform sampler2D myTexture;
uniform vec3 transparentPixel;
uniform vec3 pieceColor;

in vec2 uvCoord;

out vec4 outColor;

bool isTransparent(vec4 color)
{
  bool result = abs(color.x - transparentPixel.x) <= .01
           && abs(color.y - transparentPixel.y) <= .2
           && abs(color.z - transparentPixel.z) <= .01;
  return result;
}

void main()
{
  vec4 sampled = texture(myTexture, uvCoord);
  if (isTransparent(sampled))
  {
    outColor.w = 0;
  }
  else
  {
    outColor = vec4(pieceColor, 1.f);
  }
}
)FOO";

const char *chessBoardFragmentShader = R"FOO(
#version 330 core
uniform sampler2D myTexture;
in vec2 uvCoord;

out vec4 outColor;

void main()
{
  vec4 sampled = texture(myTexture, uvCoord);
  outColor = sampled;
}
)FOO";

#if 0
const char *fragmentShader = R"FOO(
#version 330 core
out vec4 outColor;

void main()
{
  outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
)FOO";
#endif

static void update()
{
  //1. board
  {
    GLfloat left = 0; 
    GLfloat top = 0;
    GLfloat right = g_windowW;
    GLfloat bottom = g_windowH;
    
    //a   b
    // [ ]
    //c   d  
    GLfloat vertices[VERTICES_COUNT] = {
      left, top,     0.0f, 0.0f,  //a
      right, top,    4.0f, 0.0f,  //b
      left,  bottom, 0.0f, 4.0f,  //c
      right, bottom, 4.0f, 4.0f   //d
    };
    
    RenderData &boardRenderData = g_renderData[0];
    for (int i = 0; i < VERTICES_COUNT; i++)
    {
      boardRenderData.vertices[i] = vertices[i];
    }
    boardRenderData.textureId = tx_chessBoard.textureId;
    boardRenderData.programData = &g_chessBoardProgramData;
    boardRenderData.shouldRender = true;
  }
  
  //2. pieces
  {
    float pieceOffset = 4.f;
    float widthOfTile  = (g_windowW / 8.f);
    float heightOfTile = (g_windowH / 8.f);
    float widthOfPiece = widthOfTile - (pieceOffset * 2);
    float heightOfPiece= heightOfTile - (pieceOffset * 2);
    
    for (int i = 0; i < CHESSPIECE_COUNT; i++)
    {
      ChessPiece &chessPiece = g_chessState.chessPieces[i];
      RenderData &chessPieceRenderData = g_renderData[i + 1];
      
      if (chessPiece.active)
      {
        GLfloat left = chessPiece.x * widthOfTile + pieceOffset;
        GLfloat top = chessPiece.y * heightOfTile + pieceOffset;
        GLfloat right = left + widthOfPiece;
        GLfloat bottom = top + heightOfPiece;
        
        //a   b
        // [ ]
        //c   d  
        GLfloat vertices[VERTICES_COUNT] = {
          left, top,     0.0f, 0.0f,  //a
          right, top,    1.0f, 0.0f,  //b
          left,  bottom, 0.0f, 1.0f,  //c
          right, bottom, 1.0f, 1.0f   //d
        };
        RenderData &chessPieceRenderData = g_renderData[i + 1];
        for (int i = 0; i < VERTICES_COUNT; i++)
        {
          chessPieceRenderData.vertices[i] = vertices[i]; 
        }
        chessPieceRenderData.textureId = getChessPieceTextureId(chessPiece);
        chessPieceRenderData.color = isBlack(chessPiece)
          ? g_colorChessBlack
          : g_colorChessWhite;
        chessPieceRenderData.shouldRender = true;
        chessPieceRenderData.programData = &g_chessPieceProgramData;
      }
      else
      {
        chessPieceRenderData.shouldRender = false;
      }
    }    
  }
}

static void reshape(int w, int h)
{
  //maintain square aspect ratio
  int s = w < h? w: h;
  g_windowH = s;
  g_windowW = s;
  
  MatrixStack mat;
  mat.Translate(-1.0f, 1.0f, 0.0f);
  mat.Scale(glm::vec3(2.0f / g_windowW, -2.0f / g_windowH, 1.0f));  
  
  g_worldToClipMatrix = mat.Top();
  
  glUseProgram(g_chessPieceProgramData.program);
  glUniformMatrix4fv(g_chessPieceProgramData.worldToClipMatrix, 1, GL_FALSE, glm::value_ptr(g_worldToClipMatrix));
  glUseProgram(0);
  
  glUseProgram(g_chessBoardProgramData.program);
  glUniformMatrix4fv(g_chessBoardProgramData.worldToClipMatrix, 1, GL_FALSE, glm::value_ptr(g_worldToClipMatrix));
  glUseProgram(0);
  
  int x0 = (w - s) / 2;
  int y0 = (h - s) / 2;
  
  glViewport(x0, y0, (GLsizei) g_windowW, (GLsizei) g_windowH);
}

void display()
{
  glClearColor(.1f, .2f, .2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glBindVertexArray(g_VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
  
  //render chess board
  //for (int i = 0; i < 1; i++)
  for (int i = 0; i < RENDERDATA_COUNT; i++)
  {
    RenderData &renderData = g_renderData[i];
    if (renderData.shouldRender)
    {
      glUseProgram(renderData.programData->program);
      
      //bind texture
      glActiveTexture(GL_TEXTURE0 + g_textureUnit);
      glUniform1i(renderData.programData->sampler, g_textureUnit); 
      glBindTexture(GL_TEXTURE_2D, renderData.textureId);
      
      //copy vertices
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 16, renderData.vertices);
      
      //copy color
      glUniform3f(renderData.programData->color, 
                  renderData.color.x, 
                  renderData.color.y,
                  renderData.color.z);
      
      //render
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);    
    }
  }
  
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
  
  glutSwapBuffers();
}

void runGameLoop(int val)
{
  update();
  display();
  
  if (g_gameLoopContinues)
  {
    glutTimerFunc(DELAYMS, runGameLoop, val);
  }
}

static void exitGameLoop()
{
  glutLeaveMainLoop();
  g_gameLoopContinues = false;
}

static bool loadTexture(const char *filepath, Texture *tx)
{
  bool result = false;
  
  uchar *pixelData = stbi_load(filepath, &tx->w, &tx->h, 0, 3);
  if (pixelData == NULL)
  {
    printf("Failed to load image\n");
  }
  else
  {
    printf("Image loaded; width: %d, height: %d, channels: %d\n", tx->w, tx->h, 3);
    
    //load image into texture
    glGenTextures(1, &tx->textureId);
    glBindTexture(GL_TEXTURE_2D, tx->textureId);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB8,
                 tx->w,
                 tx->h,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 pixelData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(pixelData);
    result = true;
  }
  
  return result;
}

static void initProgram(const char *vShader, const char *fShader, ProgramData *p)
{
  p->program = createShaderProgram(vShader, fShader);
  p->worldToClipMatrix = glGetUniformLocation(p->program, "worldToClipMatrix");
  p->sampler = glGetUniformLocation(p->program, "myTexture");
  p->transparentPixel = glGetUniformLocation(p->program, "transparentPixel");
  p->color = glGetUniformLocation(p->program, "pieceColor");
  
  glUseProgram(p->program);
  glUniform3f(p->transparentPixel, 
                g_transparentPixel.x, 
                g_transparentPixel.y,
                g_transparentPixel.z);
  glUseProgram(0);
}

static void init()
{
  //1. init program
  initProgram(vertexShader, chessBoardFragmentShader, &g_chessBoardProgramData);
  initProgram(vertexShader, chessPieceFragmentShader, &g_chessPieceProgramData);
  
  //2. load chess board
  if (loadTexture("shared/data/chess.png", &tx_chessBoard))
  {
    glBindTexture(GL_TEXTURE_2D, tx_chessBoard.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  
  //3. load chess pieces
  loadTexture("shared/data/chess_pawn.png", &tx_pawn);
  loadTexture("shared/data/chess_knight.png", &tx_knight);
  loadTexture("shared/data/chess_bishop.png", &tx_bishop);
  loadTexture("shared/data/chess_rook.png", &tx_rook);
  loadTexture("shared/data/chess_queen.png", &tx_queen);
  loadTexture("shared/data/chess_king.png", &tx_king);
  
  //4. init chess state
  {
    for (int i = 0; i < CHESSPIECE_COUNT/2; i++)
    {
      g_chessState.chessPieces[i].x = i % 8;
      g_chessState.chessPieces[i].y = i / 8;
      g_chessState.chessPieces[i].active = true;
    }
    for (int i = CHESSPIECE_COUNT/2; i < CHESSPIECE_COUNT; i++)
    {
      int j = i - CHESSPIECE_COUNT / 2;
      g_chessState.chessPieces[i].x = j % 8;
      g_chessState.chessPieces[i].y = (j / 8) + 6;
      g_chessState.chessPieces[i].active = true;
    }
    int i = 0;
    g_chessState.chessPieces[i++].unit = cu_b_rook;
    g_chessState.chessPieces[i++].unit = cu_b_knight;
    g_chessState.chessPieces[i++].unit = cu_b_bishop;
    g_chessState.chessPieces[i++].unit = cu_b_queen;
    g_chessState.chessPieces[i++].unit = cu_b_king;
    g_chessState.chessPieces[i++].unit = cu_b_bishop;
    g_chessState.chessPieces[i++].unit = cu_b_knight;
    g_chessState.chessPieces[i++].unit = cu_b_rook;
    for (int j = 0; j < 8; j++)
    {
      g_chessState.chessPieces[i++].unit = cu_b_pawn;
    }
    for (int j = 0; j < 8; j++)
    {
      g_chessState.chessPieces[i++].unit = cu_w_pawn;
    }
    g_chessState.chessPieces[i++].unit = cu_w_rook;
    g_chessState.chessPieces[i++].unit = cu_w_knight;
    g_chessState.chessPieces[i++].unit = cu_w_bishop;
    g_chessState.chessPieces[i++].unit = cu_w_queen;
    g_chessState.chessPieces[i++].unit = cu_w_king;
    g_chessState.chessPieces[i++].unit = cu_w_bishop;
    g_chessState.chessPieces[i++].unit = cu_w_knight;
    g_chessState.chessPieces[i++].unit = cu_w_rook;
  }
  
  //4. vbo, ebo, vao
  {
    glGenBuffers(1, &g_VBO);
    glGenBuffers(1, &g_EBO);
    glGenVertexArrays(1, &g_VAO);    
    
    GLushort indices[] = {
      2, 0, 1,
      2, 1, 3
    };
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, NULL, GL_DYNAMIC_DRAW);
    
    glBindVertexArray(g_VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
    
    {
      ProgramData p = g_chessBoardProgramData;
      GLuint positionAttribLocation = glGetAttribLocation(p.program, "aPos");
      glVertexAttribPointer(positionAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)* 4, (void *) 0);
      glEnableVertexAttribArray(positionAttribLocation);
    
      GLuint textureCoordAttribLocation = glGetAttribLocation(p.program, "textureCoord");
      glVertexAttribPointer(textureCoordAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void *) (sizeof(GLfloat) * 2));
      glEnableVertexAttribArray(textureCoordAttribLocation);
    }
    
    {
      ProgramData p = g_chessPieceProgramData;
      GLuint positionAttribLocation = glGetAttribLocation(p.program, "aPos");
      glVertexAttribPointer(positionAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)* 4, (void *) 0);
      glEnableVertexAttribArray(positionAttribLocation);
      
      GLuint textureCoordAttribLocation = glGetAttribLocation(p.program, "textureCoord");
      glVertexAttribPointer(textureCoordAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void *) (sizeof(GLfloat) * 2));
      glEnableVertexAttribArray(textureCoordAttribLocation);
    }
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
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