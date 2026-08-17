#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in vec2 a_uv;
layout (location = 4) in vec3 a_translate;
layout (location = 5) in vec3 a_scale;
layout (location = 6) in vec3 a_rotation;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out vec3 v_normal;

layout(std140, set = 1, binding = 0) uniform ViewUniform
{
  mat4 viewMat;
};

void main()
{

  mat4 translation = mat4(1.0);
  translation[3] = vec4(a_translate, 1.0);

  mat4 scale = mat4(1.0);
  scale[0][0] = a_scale.x;
  scale[1][1] = a_scale.y;
  scale[2][2] = a_scale.z;

  mat4 rotationX = mat4(
			1.0, 0.0, 0.0, 0.0,
			0.0, cos(a_rotation.x), -sin(a_rotation.x), 0.0,
			0.0, sin(a_rotation.x),  cos(a_rotation.x), 0.0,
			0.0, 0.0, 0.0, 1.0
			);

  mat4 rotationY = mat4(
			cos(a_rotation.y), 0.0, sin(a_rotation.y), 0.0,
			0.0, 1.0, 0.0, 0.0,
			-sin(a_rotation.y), 0.0, cos(a_rotation.y), 0.0,
			0.0, 0.0, 0.0, 1.0
			);

  mat4 rotationZ = mat4(
			cos(a_rotation.z), -sin(a_rotation.z), 0.0, 0.0,
			sin(a_rotation.z),  cos(a_rotation.z), 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0
			);

  mat4 model =
    translation *
    rotationZ *
    rotationY *
    rotationX *
    scale;
  
  gl_Position = viewMat * model * vec4(a_position, 1.0f);
  v_color = a_color;
  v_uv = a_uv;
  v_normal = a_normal;
}
