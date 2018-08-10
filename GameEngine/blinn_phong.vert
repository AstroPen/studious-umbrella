R"__(

in vec4 vertex;
in vec2 vertex_uv;
in vec4 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_tangent;

out VertexData {
  smooth out vec2 uv; // TODO this is kind of redundant with scaled_uv
  smooth out vec4 color;
  smooth out vec3 normal;
  smooth out vec3 tangent;
  smooth out vec3 bitangent;
  smooth out vec3 p;
  smooth out vec3 camera_p;
  smooth out vec2 scaled_uv;
} v_out;

//uniform mat4x4 transform;
uniform mat4x4 VIEW_MATRIX;
uniform mat4x4 PROJECTION_MATRIX;

uniform float TEXTURE_WIDTH;
uniform float TEXTURE_HEIGHT;

void main() {
  vec4 adjusted_vertex = vertex;
  adjusted_vertex.w = 1;
  vec4 projected_vertex = PROJECTION_MATRIX * VIEW_MATRIX * adjusted_vertex;

  projected_vertex.z -= vertex.w;
  gl_Position = projected_vertex;
  v_out.p = adjusted_vertex.xyz;

  v_out.normal = vertex_normal;
  v_out.tangent = vertex_tangent; //vec3(1,0,0); // TODO pass this in for real
  v_out.bitangent = cross(vertex_normal, vertex_tangent);
  v_out.camera_p = vec3(inverse(VIEW_MATRIX) * vec4(0,0,0,1));
  v_out.uv = vertex_uv;
  // NOTE : Premultiplied alpha
  v_out.color = vertex_color;
  v_out.color.rgb *= vertex_color.a;
  v_out.scaled_uv = vertex_uv * vec2(TEXTURE_WIDTH, TEXTURE_HEIGHT);
}

)__";

