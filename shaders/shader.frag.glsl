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

// Normal shading
// void main()
// {
//   vec3 N = normalize(v_normal);
//   FragColor = vec4(-N * 0.5 + 0.5, 1.0);
// }

// Lambert shading
void main()
{
  vec3 L = vec3(-0.5, 0.8, -0.5);
  float diffuse = max(dot(normalize(v_normal), L), 0.0);

  FragColor = vec4(vec3(diffuse), 1.0);
}

// Fragment playground
// void main(){
//   vec2 P = gl_FragCoord.xy / uResolution;
//   vec2 A = vec2(0.2);
//   vec2 B = vec2(0.8);
//   vec2 AB = B - A;
//   vec2 AP = P - A;
//   float projectionParameter = dot(AP, AB) / dot(AB, AB);
//   vec2 projection = A + projectionParameter * AB;
//   float distance = length(P - projection);
//   float projectionSide = AB.x * AP.y - AB.y * AP.x;
//   float d = distance * sign(projectionSide);
//   if (projectionParameter >= 0.0 && projectionParameter <= 1.0 && projectionSide >= 0.0){
//     FragColor = vec4(vec3(0.0), 1.0);
//   } else {
//     FragColor = vec4(vec3(1.0), 1.0);
//   }
// }
