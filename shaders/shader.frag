#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 1) in vec3 v_normal;

layout (location = 0) out vec4 FragColor;
layout(set = 2, binding = 0) uniform sampler2D tex;

void main()
{
  vec3 L = normalize(lightPos - v_position);
  FragColor = texture(tex, v_uv);
}
