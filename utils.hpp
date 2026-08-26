#pragma once
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "types.hpp"

#define sdlAssert(cond)					\
  do {							\
    if (!(cond)) {					\
      std::fprintf(stderr, "%s:%d: SDL error: %s\n",	\
		   __FILE__, __LINE__, SDL_GetError());	\
      std::abort();					\
    }							\
  } while (0)

#define ASSERT(cond, msg)                                              \
  do {                                                                 \
    if (!(cond)) {                                                     \
      std::fprintf(stderr, "%s:%d: ASSERT FAILED: %s - %s\n",          \
                   __FILE__, __LINE__, #cond, msg);                    \
      std::abort();                                                    \
    }                                                                  \
  } while (0)

std::vector<u8> readFile(const char* path);

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0)
            os << ",\n";
        os << vec[i];
    }
    os << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vertex& v);
std::ostream& operator<<(std::ostream& os, const glm::vec2& v);
std::ostream& operator<<(std::ostream& os, const glm::vec3& v);
std::ostream& operator<<(std::ostream& os, const glm::vec4& v);
std::ostream& operator<<(std::ostream& os, const MeshRegion& r);

float degToRad(float deg);

template <typename F>
class ScopeGuard {
  F f;
  bool active = true;

public:
  explicit ScopeGuard(F&& f) : f(std::forward<F>(f)) {}

  ScopeGuard(ScopeGuard&& other)
    : f(std::move(other.f)), active(other.active) {
    other.active = false;
  }

  ~ScopeGuard() {
    if (active)
      f();
  }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
};

template <typename F>
ScopeGuard(F&&) -> ScopeGuard<F>;

#define DEFER_IMPL(x, y) x##y
#define DEFER(x, y) DEFER_IMPL(x, y)

#define defer(code)						\
  auto DEFER(_defer_, __COUNTER__) = ScopeGuard([&] { code; })

glm::mat4 transformToMat4(const Transform& t);

Mat4 translateMatrix(const f32 v[4]);
Mat4 translateMatrix(const glm::vec3& v);
