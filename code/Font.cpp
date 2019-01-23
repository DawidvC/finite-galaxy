// Font.cpp

#include "Font.h"

#include "Colour.h"
#include "ImageBuffer.h"
#include "Point.h"
#include "Screen.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace {
  bool showUnderlines = false;
  
  const char *vertexCode =
    // "scale" maps pixel coordinates to GL coordinates (-1 to 1).
    "uniform vec2 scale;\n"
    // The (x, y) coordinates of the top left corner of the glyph.
    "uniform vec2 position;\n"
    // The glyph to draw. (ASCII value - 32).
    "uniform int glyph;\n"
    // Aspect ratio of rendered glyph (unity by default).
    "uniform float aspect = 1.f;\n"
    
    // Inputs from the VBO.
    "in vec2 vert;\n"
    "in vec2 corner;\n"
    
    // Output to the fragment shader.
    "out vec2 texCoord;\n"
    
    // Pick the proper glyph out of the texture.
    "void main() {\n"
    "  texCoord = vec2((glyph + corner.x) / 110.f, corner.y);\n"
    "  gl_Position = vec4((aspect * vert.x + position.x) * scale.x, (vert.y + position.y) * scale.y, 0, 1);\n"
    "}\n";
  
  const char *fragmentCode =
    // The user must supply a texture and a colour (white by default).
    "uniform sampler2D tex;\n"
    "uniform vec4 colour = vec4(1, 1, 1, 1);\n"
    
    // This comes from the vertex shader.
    "in vec2 texCoord;\n"
    
    // Output colour.
    "out vec4 finalColour;\n"
    
    // Multiply the texture by the user-specified colour (including alpha).
    "void main() {\n"
    "  finalColour = texture(tex, texCoord).a * colour;\n"
    "}\n";
  
  const int KERN = 2;
}



Font::Font()
  : texture(0), vao(0), vbo(0), colourI(0), scaleI(0), glyphI(0), aspectI(0),
    positionI(0), height(0), space(0), screenWidth(0), screenHeight(0)
{
}



Font::Font(const string &imagePath)
  : Font()
{
  Load(imagePath);
}



void Font::Load(const string &imagePath)
{
  // Load the texture.
  ImageBuffer image;
  if(!image.Read(imagePath))
    return;
  
  LoadTexture(image);
  CalculateAdvances(image);
  SetUpShader(image.Width() / GLYPHS, image.Height());
}



void Font::Draw(const string &str, const Point &point, const Colour &colour) const
{
  DrawAliased(str, round(point.X()), round(point.Y()), colour);
}



void Font::DrawAliased(const string &str, double x, double y, const Colour &colour) const
{
  glUseProgram(shader.Object());
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindVertexArray(vao);
  
  glUniform4fv(colourI, 1, colour.Get());
  
  // Update the scale, only if the screen size has changed.
  if(Screen::Width() != screenWidth || Screen::Height() != screenHeight)
  {
    screenWidth = Screen::Width();
    screenHeight = Screen::Height();
    GLfloat scale[2] = {2.f / screenWidth, -2.f / screenHeight};
    glUniform2fv(scaleI, 1, scale);
  }
  
  GLfloat textPos[2] = {
    static_cast<float>(x - 1.),
    static_cast<float>(y)};
  int previous = 0;
  bool isAfterSpace = true;
  bool underlineChar = false;
  const int underscoreGlyph = max(0, min(GLYPHS - 1, '_' - 32));
  
  for(char c : str)
  {
    if(c == '_')
    {
      underlineChar = showUnderlines;
      continue;
    }
    
    int glyph = Glyph(c, isAfterSpace);
    if(c != '"' && c != '\'')
      isAfterSpace = !glyph;
    if(!glyph)
    {
      textPos[0] += space;
      continue;
    }
    
    glUniform1i(glyphI, glyph);
    glUniform1f(aspectI, 1.f);
    
    textPos[0] += advance[previous * GLYPHS + glyph] + KERN;
    glUniform2fv(positionI, 1, textPos);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    if(underlineChar)
    {
      glUniform1i(glyphI, underscoreGlyph);
      glUniform1f(aspectI, static_cast<float>(advance[glyph * GLYPHS] + KERN)
        / (advance[underscoreGlyph * GLYPHS] + KERN));
      
      glUniform2fv(positionI, 1, textPos);
      
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      underlineChar = false;
    }
    
    previous = glyph;
  }
  
  glBindVertexArray(0);
  glUseProgram(0);
}



int Font::Width(const string &str, char after) const
{
  return Width(str.c_str(), after);
}



int Font::Width(const char *str, char after) const
{
  int width = 0;
  int previous = 0;
  bool isAfterSpace = true;
  
  for( ; *str; ++str)
  {
    if(*str == '_')
      continue;
    
    int glyph = Glyph(*str, isAfterSpace);
    if(*str != '"' && *str != '\'')
      isAfterSpace = !glyph;
    if(!glyph)
      width += space;
    else
    {
      width += advance[previous * GLYPHS + glyph] + KERN;
      previous = glyph;
    }
  }
  width += advance[previous * GLYPHS + max(0, min(GLYPHS - 1, after - 32))];
  
  return width;
}



string Font::Truncate(const string &str, int width) const
{
  int prevChars = str.size();
  int prevWidth = Width(str);
  if(prevWidth <= width)
    return str;
  
  width -= Width("...");
  // As a safety against infinite loops (even though they won't be possible if
  // this implementation is correct) limit the number of loops to the number
  // of characters in the string.
  for(size_t i = 0; i < str.length(); ++i)
  {
    // Loop until the previous width we tried was too long and this one is
    // too short, or vice versa. Each time, the next string length we try is
    // interpolated from the previous width.
    int nextChars = (prevChars * width) / prevWidth;
    bool isSame = (nextChars == prevChars);
    bool prevWorks = (prevWidth <= width);
    nextChars += (prevWorks ? isSame : -isSame);
    
    int nextWidth = Width(str.substr(0, nextChars), '.');
    bool nextWorks = (nextWidth <= width);
    if(prevWorks != nextWorks && abs(nextChars - prevChars) == 1)
      return str.substr(0, min(prevChars, nextChars)) + "...";
    
    prevChars = nextChars;
    prevWidth = nextWidth;
  }
  return str;
}



string Font::TruncateFront(const string &str, int width) const
{
  int prevChars = str.size();
  int prevWidth = Width(str);
  if(prevWidth <= width)
    return str;
  
  width -= Width("...");
  // As a safety against infinite loops (even though they won't be possible if
  // this implementation is correct) limit the number of loops to the number
  // of characters in the string.
  for(size_t i = 0; i < str.length(); ++i)
  {
    // Loop until the previous width we tried was too long and this one is
    // too short, or vice versa. Each time, the next string length we try is
    // interpolated from the previous width.
    int nextChars = (prevChars * width) / prevWidth;
    bool isSame = (nextChars == prevChars);
    bool prevWorks = (prevWidth <= width);
    nextChars += (prevWorks ? isSame : -isSame);
    
    int nextWidth = Width(str.substr(str.size() - nextChars));
    bool nextWorks = (nextWidth <= width);
    if(prevWorks != nextWorks && abs(nextChars - prevChars) == 1)
      return "..." + str.substr(str.size() - min(prevChars, nextChars));
    
    prevChars = nextChars;
    prevWidth = nextWidth;
  }
  return str;
}



string Font::TruncateMiddle(const string &str, int width) const
{
  int prevChars = str.size();
  int prevWidth = Width(str);
  if(prevWidth <= width)
    return str;
  
  width -= Width("...");
  // As a safety against infinite loops (even though they won't be possible if
  // this implementation is correct), limit the number of loops to the number
  // of characters in the string.
  for(size_t i = 0; i < str.length(); ++i)
  {
    // Loop until the previous width we tried was too long and this one is
    // too short, or vice versa. Each time, the next string length we try is
    // interpolated from the previous width.
    int nextChars = (prevChars * width) / prevWidth;
    bool isSame = (nextChars == prevChars);
    bool prevWorks = (prevWidth <= width);
    nextChars += (prevWorks ? isSame : -isSame);
    
    int leftChars = nextChars / 2;
    int rightChars = nextChars - leftChars;
    int nextWidth = Width(str.substr(0, leftChars) + str.substr(str.size() - rightChars));
    bool nextWorks = (nextWidth <= width);
    if(prevWorks != nextWorks && abs(nextChars - prevChars) == 1)
    {
      leftChars = min(prevChars,nextChars) / 2;
      rightChars = min(prevChars, nextChars) - leftChars;
      return str.substr(0, leftChars) + "..." + str.substr(str.size() - rightChars);
    }
    
    prevChars = nextChars;
    prevWidth = nextWidth;
  }
  return str;
}



int Font::Height() const
{
  return height;
}



int Font::Space() const
{
  return space;
}



void Font::ShowUnderlines(bool show)
{
  showUnderlines = show;
}



int Font::Glyph(char c, bool isAfterSpace)
{
/*
  // Interpunct
  if(c == '·')
    return 96;

  // Degrees, minutes, seconds
  if(c == '°')
    return 97;
  if(c == '′')
    return 98;
  if(c == '″')
    return 99;
*/

  // Curly quotes
  if(c == '"' && isAfterSpace)
    return 102;
  if(c == '"' && !isAfterSpace)
    return 104;
  if(c == '\'' && isAfterSpace)
    return 103;
  if(c == '\'' && !isAfterSpace)
    return 105;
  
  return max(0, min(GLYPHS - 3, c - 32));
}



void Font::LoadTexture(ImageBuffer &image)
{
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.Width(), image.Height(), 0,
    GL_BGRA, GL_UNSIGNED_BYTE, image.Pixels());
}



void Font::CalculateAdvances(ImageBuffer &image)
{
  // Get the format and size of the surface.
  int width = image.Width() / GLYPHS;
  height = image.Height();
  unsigned mask = 0xFF00'0000;
  unsigned half = 0xC000'0000;
  int pitch = image.Width();
  
  // advance[previous * GLYPHS + next] is the x advance for each glyph pair.
  // There is no advance if the previous value is 0, i.e. we are at the very
  // start of a string.
  memset(advance, 0, GLYPHS * sizeof(advance[0]));
  for(int previous = 1; previous < GLYPHS; ++previous)
    for(int next = 0; next < GLYPHS; ++next)
    {
      int maxD = 0;
      int glyphWidth = 0;
      uint32_t *begin = image.Pixels();
      for(int y = 0; y < height; ++y)
      {
        // Find the last non-empty pixel in the previous glyph.
        uint32_t *pend = begin + previous * width;
        uint32_t *pit = pend + width;
        while(pit != pend && (*--pit & mask) < half) {}
        int distance = (pit - pend) + 1;
        glyphWidth = max(distance, glyphWidth);
        
        // Special case: if "next" is zero (i.e. end of line of text),
        // calculate the full width of this character. Otherwise:
        if(next)
        {
          // Find the first non-empty pixel in this glyph.
          uint32_t *nit = begin + next * width;
          uint32_t *nend = nit + width;
          while(nit != nend && (*nit++ & mask) < half) {}
          
          // How far apart do you want these glyphs drawn? If drawn at
          // an advance of "width", there would be:
          // pend + width - pit   <- pixels after the previous glyph.
          // nit - (nend - width) <- pixels before the next glyph.
          // So for zero kerning distance, you would want:
          distance += 1 - (nit - (nend - width));
        }
        maxD = max(maxD, distance);
        
        // Update the pointer to point to the beginning of the next row.
        begin += pitch;
      }
      // This is a fudge factor to avoid over-kerning, especially for the
      // underscore and for glyph combinations like AV.
      advance[previous * GLYPHS + next] = max(maxD, glyphWidth - 4) / 2;
    }
  
  // Set the space size based on the character width.
  width /= 2;
  height /= 2;
  space = (width + 3) / 6 + 1;
}



void Font::SetUpShader(float glyphW, float glyphH)
{
  glyphW *= .5f;
  glyphH *= .5f;
  
  shader = Shader(vertexCode, fragmentCode);
  glUseProgram(shader.Object());
  glUniform1i(shader.Uniform("tex"), 0);
  glUseProgram(0);
  
  // Create the VAO and VBO.
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  
  GLfloat vertices[] = {
       0.f,    0.f, 0.f, 0.f,
       0.f, glyphH, 0.f, 1.f,
    glyphW,    0.f, 1.f, 0.f,
    glyphW, glyphH, 1.f, 1.f
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
  // connect the xy to the "vert" attribute of the vertex shader
  glEnableVertexAttribArray(shader.Attrib("vert"));
  glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
  
  glEnableVertexAttribArray(shader.Attrib("corner"));
  glVertexAttribPointer(shader.Attrib("corner"), 2, GL_FLOAT, GL_FALSE,
    4 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  
  // We must update the screen size next time we draw.
  screenWidth = 0;
  screenHeight = 0;
  
  colourI = shader.Uniform("colour");
  scaleI = shader.Uniform("scale");
  glyphI = shader.Uniform("glyph");
  aspectI = shader.Uniform("aspect");
  positionI = shader.Uniform("position");
}
