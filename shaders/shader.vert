#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 0) out vec4 v_color;
layout(set = 0, binding = 0) uniform Camera { 
    mat4 viewProjection;
} camera;

void main()
{
    gl_Position = vec4(a_position.z, a_position.y, a_position.x , 1.0f);
    v_color = a_color;
}