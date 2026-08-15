#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_normal;

layout (location = 0) out vec4 FragColor;
layout(set = 2, binding = 0) uniform sampler2D tex;
layout(std140, set = 3, binding = 0) uniform FragmentUniforms
{
  vec2 uResolution;
};

void main()
{
  // vec3 N = normalize(v_normal);
  // FragColor = vec4(-N * 0.5 + 0.5, 1.0);

  vec2 normalizedCoord = gl_FragCoord.xy / uResolution;
  FragColor = vec4(normalizedCoord, 1.0, 1.0);
}
