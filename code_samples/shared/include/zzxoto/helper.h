#ifndef H_ZZXOTO_HELPER
#define H_ZZXOTO_HELPER

#include <stdio.h>
#include <stack>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>

#define PI 3.14159

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
    float *columnMajor = glm::value_ptr(m_currMatrix);
    columnMajor[0] *= scaleVec.x;
    columnMajor[5] *= scaleVec.y;
    columnMajor[10] *= scaleVec.z;
  }
  
  void Scale(float uniformScale) 
  {
    Scale(glm::vec3(uniformScale));
  }
  
  void Translate(float tx, float ty, float tz)
  {
    float *columnMajor = glm::value_ptr(m_currMatrix);
    
    for (int i = 0; i < 4; i++)
    {
      columnMajor[i + 12] = columnMajor[i] * tx + columnMajor[i + 4] * ty + 
        columnMajor[i + 8] * tz + columnMajor[i + 12];
    } 
  }
  
  void Translate(const glm::vec3 &offsetVec)
  {
    Translate(offsetVec[0], offsetVec[1], offsetVec[2]);
  }
  
  void Perspective(float fov, float N, float F)
  {
    float *columnMajor = glm::value_ptr(m_currMatrix);
    float tanThetaOver2 = tan(fov * (PI / 360.0f));
    
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
  
  PushStack(const PushStack &);
}; 


internal void printMatrix(const glm::mat4 &matrix)
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
#endif