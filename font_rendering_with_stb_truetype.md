# Font Rendering with STB Truetype

## Introduction
Here Font concepts are discussed in general sense, and practical implementation wise, `stb_truetype` single header file library is referenced. Code samples are in C/C++. Also, horizontal left-to-right layout of true type fonts is assumed. 

Topics to cover as TODO:
- Packing every character bitmap into a large texture as an optimization
- Kerning
- Bezier Curve

## Concepts

### Glyph or Character Glyph
Visual shape of the character. Below is a character glyph for character `A`:
```
  XXX  
 XX XX 
XX   XX
XX   XX
XXXXXXX
XX   XX
XX   XX
```

### Codepoint
If a glyph is a visual shape of the character, then codepoint is the unique number that identifies that character. This is usually derived from character encoding standard. E.g. If a font file uses a ASCII encoding standard then character `A` would be identified by codepoint 65.

### Bounding Box or Bbox
Smallest box i.e. the box with the least area that bounds the glyph. The borders around glyph below is the bbox for that glyph
```
---------
|  XXX  |
| XX XX |
|XX   XX|
|XX   XX|
|XXXXXXX|
|XX   XX|
|XX   XX|
---------
```

### Baseline
Glyph shapes are defined relative to a baseline, which is the bottom of uppercase characters. Characters extend both above and below the baseline. E.g. the character glyph of lowercase `g`, relative to baseline would look as such:
```
   xxxxx
 xx    xx  
 x     xx  
 xx    xx  
  xxxxxxx  
 ----- xx----baseline
  xx   xx  
   xxxxxx   
```

### Origin
When strings of characters are drawn, characters are spaced apropriately. Space before a character is defined by the distance from the 'origin' of the character glyph to the glyph's bbox. `O` marks the origin below:
```
    |    ---------
    |    |  XXX  |
    |    | XX XX |
    |    |XX   XX|
    |    |XX   XX|
    |    |XXXXXXX|
    |    |XX   XX|
   O|    |XX   XX|
----|-----------------baseline
    |
```

### Pen Position
As strings of characters are drawn, pen position is always at the origin of the character being drawn.

### Font Metrics
Measurements of the font, that could either be defined per font like `ascent`, `line height`, etc. or per character glyph like `advance width`, `left bearing`, etc.

#### Ascent
Max distance from the baseline to top of bbox, among all the glyphs of the font. This value is defined per font.

#### Descent
Max distance from the baseline to bottom of bbox, among all the glyphs of the font. This value is defined per font.

#### Line Height
Height of the layout where characters are rendered horizontally. When breaking a new line, advance the baseline vertically by this value. This value is defined per font.

#### Advance Width
As strings of characters are being drawn horizontally, horizontal position(pen position) is tracked. After drawing a character, advance that horizontal position by this value. This value is defined per glyph. 

#### Left Bearing
Distance from the origin to the left of bbox of the glyph. This value is defined per glyph.

#### Top Bearing
Distance from the baseline to the top of bbox of the glyph. This value is defined per glyph.

#### My Font and Font Metrics Definition
`stb_truetype` font metrics at times have -ve value. For instance, `descent` is defined relative to the baseline as 0, and therefore the library returns that value as -ve. In structs defined below every metric values are +ve. Also, the textures and metrics are defined for a specific font height; for different font height, different set of textures and metrics can be constructed from the same `stbtt_fontinfo`:
```
typedef struct CharacterFontInfo
{
  unsigned char character;
  int advanceWidth; 
  int topBearing;   
  int leftBearing;  
  GLuint textureId; //OpenGL texture Id from `glGenTextures` call.
  int w;            //width of the texture/bbox/glyph.
  int h;            //height of the texture/bbox/glyph.
} CharacterInfo;

typedef struct FontMetrics
{
  int ascent;   
  int descent;  
  int lineHeight;
} FontMetrics;

typedef struct Font
{
  FontMetrics *fontMetrics;
  stbtt_fontinfo *stbFont;
  int fontHeightPx;
  CharacterFontInfo *characterFontInfo;//tightly packed array
  int characterFontInfoCount;          //no. of elements in array above
} Font;

Font *g_font;
```

## Including stb_truetype library
`stb_truetype.h` header file defines both the interface and the implementation.
When including we can put in appropriate `#define`s to include either of them. For instance to have the `#include` expand to implementation, have `#define STB_TRUETYPE_IMPLEMENTATION` before the `#include`. Also, to include implementations as statics, have `#define STBTT_STATIC` before the `#include`.
Example of include:
```
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

//..
//..
```

## stb_truetype constructs used in subsequent code samples
```
//Library internally loads font file, parses information and stores it into
//this structure. Most of the library's functions take this as first argument. 
//This structure, although visible when including, should be treated as opaque.
//The structure is defined publicly so one can declare and allocate.
struct stbtt_fontinfo { ... }

//Given an offset into the file that defines a font, this function builds
//the necessary cached info for the rest of the system. You must allocate
//the stbtt_fontinfo yourself, and stbtt_InitFont will fill it out. You don't
//need to do anything special to free it, because the contents are pure
//value data with no additional data structures. Returns 0 on failure.
int stbtt_InitFont(stbtt_fontinfo *info, const unsigned char *data, int offset);

//Each .ttf/.ttc file may have more than one font. Each font has a sequential
//index number starting from 0. Call this function to get the font offset for
//a given index; it returns -1 if the index is out of range. A regular .ttf
//file will only define one font and it always be at offset 0, so it will
//return '0' for index 0, and -1 for all other indices.
int stbtt_GetFontOffsetForIndex(const unsigned char *data, int index);

//computes a scale factor to produce a font whose "height" is 'pixels' tall.
//Height is measured as the distance from the highest ascender to the lowest
//descender; in other words, it's equivalent to calling stbtt_GetFontVMetrics
//and computing: scale = pixels / (ascent - descent)
//so if you prefer to measure height by the ascent only, 
//use a similar calculation.
float stbtt_ScaleForPixelHeight(const stbtt_fontinfo *info, float pixels);


//Renders specified character/glyph at the specified scale as 8bpp bitmap, with
//antialiasing. 0 is no coverage (transparent), 255 is fully covered (opaque).
//Bitmap is stored left-to-right, top-to-bottom. You pass in storage for the
//bitmap in the form of 'output'. Call stbtt_GetCodepointBitmapBox to get the
//width and height and positioning info for the character glyph. Then pass the 
//width as 3rd and 5th argument, while height as the second argument.
void stbtt_MakeCodepointBitmap(const stbtt_fontinfo *info, 
                               unsigned char *output, 
                               int out_w, 
                               int out_h, 
                               int out_stride, 
                               float scale_x, 
                               float scale_y, 
                               int codepoint);

//Get horizontal metrics for the given code point. 'advanceWidth' and
//'leftSideBearing' metrics are discussed previously. The scale factor
//hasn't been accounted.
void stbtt_GetCodepointHMetrics(const stbtt_fontinfo *info, 
                                int codepoint, 
                                int *advanceWidth, 
                                int *leftSideBearing);

//Get Vertical metrics, which are font specific as opposed to codepoint
//specific parameters. 'ascent' and 'descent' metrics are discussed previously,
//however descent here is -ve. The lineGap is the extra vertical spacing. The
//line height therefore is calculated as: 
//lineHeight = ascent - descent + lineGap
void stbtt_GetFontVMetrics(const stbtt_fontinfo *info, 
                           int *ascent, 
                           int *descent, 
                           int *lineGap);
```

## Finding the area of the text to render
```
//Given left and top as argument, returns right and bottom, that is just big
//enough to render text. (Not totally accurate, but is adequate for demo)
//Assumes top-left origin, 2d pixel coordinate.
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
      //next line, therefore increase height
      h += font->fontMetrics->lineHeight;

      //if this line is longer than the current longest line, register that
      if (penPosition - left > w)
      {
        w = penPosition - left;
      }
      
      //set pen position to initial left position
      penPosition = left;
    }
    
    text++;
    c = *text;
  }
  
  //not important, just handles special case of 0 new line characters
  if (w == 0 && penPosition - left > 0)
  {
    w = penPosition - left;
  }
  
  //not important, just handles special case of 0 new line characters
  if (w > 0 && h == 0)
  {
    h = font->fontMetrics->lineHeight;
  }
  
  *right = left + w;
  *bottom = top + h;
}

```

Code samples below here assume some familiarity with OpenGL api. Any variable
names prefixed with `g_` is global variable, that is visible in the scope. Also, `uchar` is typedefed `unsigned char`. Shader is assumed to be bound.
Also assumes top-left origin and 2d pixel coordinate.
## Rendering text
```
void displayText(Font *font, const char *text, int left, int top)
{
char c = *text;
  
  //If say top were 0, and the line height were 30, then one can imagine
  //baseline to start at somewhere around 20. Note that the y increases as
  //as we go downowards.
  int baseline = top + font->fontMetrics->ascent;

  int penPosition = left;
  
  //Every character glyph is rendered into a rectangle.
  //That rectangle consists of 4 vertices, that is, 2 * 4 unsigned
  //short positions and 2 * 4 unsigned short texture coordinates.
  glBindBuffer(GL_ARRAY_BUFFER, g_VBO);

  glBindVertexArray(g_VAO);
  while (c != 0)
  {
    CharacterFontInfo *characterFontInfo = NULL;

    //Find the metrics and texture info of the character to be
    //drawn from the font->characterFontInfo array.
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
      //'Left Bearing' and 'Top Bearing' concepts were defined previously.
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
      
      //bind GL texture to texture unit
      glActiveTexture(GL_TEXTURE0 + g_textureUnit);
      glBindTexture(GL_TEXTURE_2D, characterFontInfo->textureId);
      
      //bind GLSL sampler to texture unit
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
```

## Loading font
```
uchar g_characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567891 !@#$%^&*()-_+=.";

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
    
    int charactersN = sizeof(g_characters);
    
    CharacterFontInfo *characterFontInfo = (CharacterFontInfo *) malloc(charactersN * sizeof(CharacterFontInfo));
    
    //Since the bitmap is 8bpp while OpenGL expects texture data
    //to be 32bpp by default. This changes that default to 8bpp.
    GLint oldAlign = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlign);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    uchar *bitmapTempBuffer = (uchar *) malloc(1 << 16);
    
    for (int i = 0; i < charactersN; i++)
    {
      //4. character font bitmap and metrics
      uchar c = g_characters[i];
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

      //Here we are generating texture for every character
      //Ideally, there would be only one texture for every character,
      //and that would reduce texture swapping when rendering text.
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

      //Mipmaps are bad idea for for Font bitmap textures
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

      //Linear filter result in smoother looking fonts.
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
    
    //5. Font metrics, that are defined per font.
    int ascent, descent, lineGap;
    FontMetrics *fontMetrics = (FontMetrics *) malloc(sizeof(FontMetrics));
    stbtt_GetFontVMetrics(stbtt_font, &ascent, &descent, &lineGap);
    fontMetrics->ascent = (int) ascent * scale;

    //Since descent is -ve in stb  font metrics
    fontMetrics->descent = (int) -descent * scale;
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
```