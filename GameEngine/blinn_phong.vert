R"__(

in vec4 vertex;
in vec2 vertex_uv;
in vec4 vertex_color;
in vec3 vertex_normal;
smooth out vec2 frag_uv;
smooth out vec4 frag_color;
smooth out vec3 frag_normal;
smooth out vec3 frag_p;
smooth out vec3 camera_p;

//uniform mat4x4 transform;
uniform mat4x4 view_matrix;
uniform mat4x4 projection_matrix;

void main() {
  vec4 adjusted_vertex = vertex;
  adjusted_vertex.w = 1;
  vec4 projected_vertex = projection_matrix * view_matrix * adjusted_vertex;

  projected_vertex.z -= vertex.w;
  gl_Position = projected_vertex;
  frag_p = adjusted_vertex.xyz;

  frag_normal = vertex_normal;
  camera_p = vec3(inverse(view_matrix) * vec4(0,0,0,1));
  frag_uv = vertex_uv;
  // NOTE : Premultiplied alpha
  frag_color = vertex_color;
  frag_color.rgb *= vertex_color.a;
}

)__";

