#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec2 a_uv;
layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;
layout (location = 1) out vec3 v_normal;
layout(std140, set = 1, binding = 0) uniform ViewUniform
{
  mat4 viewMat;
};

void main()
{
  gl_Position = viewMat * vec4(a_position, 1.0f);
  v_color = a_color;
  v_uv = a_uv;
  v_normal = a_normal;
}
