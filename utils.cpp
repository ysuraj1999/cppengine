#include <fstream>
#include <random>
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

static std::vector<u8> readFile(const char* path) {
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

// RANDOM shit
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);


// DEFER
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
