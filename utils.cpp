#include "utils.hpp"
#include <cmath>
#include <fstream>
#include <random>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

std::vector<u8> readFile(const char* path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    return {};
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<u8> buffer(static_cast<size_t>(size));
  if (size > 0) {
    file.read(reinterpret_cast<char*>(buffer.data()), size);
  }
  return buffer;
}

std::ostream& operator<<(std::ostream& os, const Vertex& v) {
  return os << "Vertex{"
	    << "position=(" << v.position.x << ", " << v.position.y << ", " << v.position.z << "), "
	    << "color=(" << v.color.r << ", " << v.color.g << ", " << v.color.b << ", " << v.color.a << "), "
	    << "normal=(" << v.normal.x << ", " << v.normal.y << ", " << v.normal.z << "), "
	    << "uv=(" << v.uv.x << ", " << v.uv.y << ")"
	    << "}";
}

std::ostream& operator<<(std::ostream& os, const glm::vec2& v) {
    return os << "vec2(" << v.x << ", " << v.y << ")";
}

std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    return os << "vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
}

std::ostream& operator<<(std::ostream& os, const glm::vec4& v) {
    return os << "vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
}

std::ostream& operator<<(std::ostream& os, const MeshRegion& r) {
  return os << "MeshRegion("
	    << "vertexOffset=" << r.vertexOffset
	    << ", indexOffset=" << r.indexOffset
	    << ", indexCount=" << r.indexCount
	    << ")";
}

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dist(0.0f, 1.0f);


float degToRad(float deg) {
  return deg * (M_PI / 180.0f);
}

glm::mat4 transformToMat4(const Transform& t) {
  glm::mat4 translation = glm::translate(glm::mat4(1.0f), t.position);
  glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.x),
                                    glm::vec3(1.0f, 0.0f, 0.0f));
  glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.y),
                                    glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(t.rotation.z),
                                    glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 scale = glm::scale(glm::mat4(1.0f), t.scale);
  return translation * rotationZ * rotationY * rotationX * scale;
}

Mat4 translateMatrix(const f32 v[4]) {
    Mat4 result;
    result.m0 = 1.0f;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = -v[0];
    result.m4 = 0.0f;
    result.m5 = 1.0f;
    result.m6 = 0.0f;
    result.m7 = -v[1];
    result.m8 = 0.0f;
    result.m9 = 0.0f;
    result.m10 = 1.0f;
    result.m11 = -v[2];
    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;
    return result;
}

Mat4 translateMatrix(const glm::vec3& v) {
    Mat4 result;
    result.m0 = 1.0f;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = -v.x;
    result.m4 = 0.0f;
    result.m5 = 1.0f;
    result.m6 = 0.0f;
    result.m7 = -v.y;
    result.m8 = 0.0f;
    result.m9 = 0.0f;
    result.m10 = 1.0f;
    result.m11 = -v.z;
    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;
    return result;
}
