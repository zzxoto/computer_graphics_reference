#ifndef H_ZZXOTO_HELPER
#define H_ZZXOTO_HELPER

#include <stdio.h>
#include <stack>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>

#define PI 3.14159

void printMatrix(const glm::mat4 &matrix)
{
  char *s = R"FOO( 
  %.3f, %.3f, %.3f, %.3f,
  %.3f, %.3f, %.3f, %.3f,
  %.3f, %.3f, %.3f, %.3f,
  %.3f, %.3f, %.3f, %.3f
  )FOO";
  
  const float *mat = glm::value_ptr(matrix);
  
  printf(s,
         mat[0], mat[4], mat[8], mat[12],
         mat[1], mat[5], mat[9], mat[13], 
         mat[2], mat[6], mat[10],mat[14], 
         mat[3], mat[7], mat[11],mat[15]); 
}

void printVector(const glm::vec3 &vec)
{
  char *s = "<%.3f, %.3f, %.3f>\n";
  
  printf(s, vec.x, vec.y, vec.z);
}

void printMatrix(const glm::mat3 &matrix)
{
  char *s = R"FOO( 
  %.8f, %.8f, %.8f,
  %.8f, %.8f, %.8f,
  %.8f, %.8f, %.8f
  )FOO";
  
  const float *mat = glm::value_ptr(matrix);
  
  printf(s,
         mat[0], mat[3], mat[6],
         mat[1], mat[4], mat[7],
         mat[2], mat[5], mat[8]);
}

float toRadians(float degrees)
{
  return degrees * (PI / 180.0f);
}

glm::vec3 normalize(const glm::vec3 &vec)
{
  float length = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
  
  glm::vec3 result;
  
  if (length > 0)
  {
    result = glm::vec3(vec.x / length, vec.y / length, vec.z / length);
  }
  else
  {
    result = vec;
  }
  
  return result;
}

class MatrixStack
{
  public:
  MatrixStack()
    :m_currMatrix(1)
  {
  }
  
  MatrixStack(const glm::mat4 &initialMatrix)
    :m_currMatrix(initialMatrix)
  {
  }
  
  void Push()
  {
    m_stack.push(m_currMatrix);
  }
  
  void Pop()
  {
    m_currMatrix = m_stack.top();
    m_stack.pop();
  }
  
  const glm::mat4 &Top() const
  {
    return m_currMatrix;
  }
  
  void Scale(const glm::vec3 &scaleVec)
  {
    glm::mat4 scale(1);
    float *columnMajor = glm::value_ptr(scale);
    
    columnMajor[0]  = scaleVec.x;
    columnMajor[5]  = scaleVec.y;
    columnMajor[10] = scaleVec.z;
    
    this->m_currMatrix = this->m_currMatrix * scale;
  }
  
  void Scale(float uniformScale) 
  {
    Scale(glm::vec3(uniformScale));
  }
  
  void Translate(float tx, float ty, float tz)
  {
    glm::mat4 translate(1);
    float *columnMajor = glm::value_ptr(translate);
    
    
    columnMajor[12] = tx;
    columnMajor[13] = ty;
    columnMajor[14] = tz;
    
    this->m_currMatrix = this->m_currMatrix * translate;
  }
  
  void Translate(const glm::vec3 &offsetVec)
  {
    Translate(offsetVec[0], offsetVec[1], offsetVec[2]);
  }
  
  void Perspective(float fov, float N, float F)
  {
    float *columnMajor = glm::value_ptr(m_currMatrix);
    float tanThetaOver2 = tan(toRadians(fov));
    
    columnMajor[0] = 1 / tanThetaOver2;
    columnMajor[1] = 0;
    columnMajor[2] = 0;
    columnMajor[3] = 0;
    
    columnMajor[4] = 0;
    columnMajor[5] = 1 / tanThetaOver2;
    columnMajor[6] = 0;
    columnMajor[7] = 0;
    
    columnMajor[8] = 0;
    columnMajor[9] = 0;
    columnMajor[10] = (F + N) / (F - N);
    columnMajor[11] = -1;
    
    columnMajor[12] = 0;
    columnMajor[13] = 0;
    columnMajor[14] = (2 * N * F) / (F - N);
    columnMajor[15] = 0;
  }
  
  void Rotate(float degrees, const glm::vec3 &axis_)
  {
    glm::vec3 axis = normalize(axis_);
    
    glm::mat4 rotation(1);
    float *columnMajor = glm::value_ptr(rotation);
    
    float s = sin(toRadians(degrees));
    float c = cos(toRadians(degrees));
    float c_ = 1.0f - c;
    
    columnMajor[0] = (axis.x * axis.x * c_) + c;
    columnMajor[1] = (axis.x * axis.y * c_) + (axis.z * s);
    columnMajor[2] = (axis.x * axis.z * c_) - (axis.y * s);
    
    columnMajor[4] = (axis.y * axis.x * c_) - (axis.z * s);
    columnMajor[5] = (axis.y * axis.y * c_) + c;
    columnMajor[6] = (axis.y * axis.z * c_) + (axis.x * s);
    
    columnMajor[8] = (axis.z * axis.x * c_) + (axis.y * s);
    columnMajor[9] = (axis.z * axis.y * c_) - (axis.x * s);
    columnMajor[10] = (axis.z * axis.z * c_) + c;
    
    this->m_currMatrix = this->m_currMatrix * rotation;
  }
  
  
  void RotateZ(float degrees)
  {
    glm::mat4 rotation(1);
    float *columnMajor = glm::value_ptr(rotation);
    
    float s = sin(toRadians(degrees));
    float c = cos(toRadians(degrees));
    
    //[
    //  cos, -sin, 0, 0
    //  sin, cos, 0, 0
    //  0,   0,   1, 0
    //  0,   0,   0, 1
    //]
    columnMajor[0] = c;
    columnMajor[1] = s;
    columnMajor[4] = -s;
    columnMajor[5] = c;
    
    this->m_currMatrix = this->m_currMatrix * rotation;
  }
  
  private:
  std::stack<glm::mat4, std::vector<glm::mat4>> m_stack;
  glm::mat4 m_currMatrix;
};

class PushStack
{
  public:
  PushStack(MatrixStack &stack)
    : m_stack(stack)
  {
    m_stack.Push();
  }
  
  ~PushStack()
  {
    m_stack.Pop();
  }
  
  private:
  MatrixStack &m_stack;
};

#endif