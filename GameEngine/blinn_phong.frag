R"__(

in VertexData {
  smooth in vec2 uv;
  smooth in vec4 color;
  smooth in vec3 normal;
  smooth in vec3 tangent;
  smooth in vec3 bitangent;
  smooth in vec3 p;
  smooth in vec3 camera_p;
} v_in;

out vec4 pixel_color;

uniform bool has_normal_map;
uniform sampler2D texture_sampler;
uniform sampler2D normal_sampler;

const mediump vec3 perception = vec3(0.299, 0.587, 0.114);

vec3 tone(vec3 v, float exposure) {
  float lum = dot(v, perception) * exposure;
  lum = sqrt(lum / (1 + lum));
  return v * lum;
}

void main() {
  vec4 textured_color = v_in.color * texture(texture_sampler, v_in.uv);
  if (textured_color.a < 0.001) discard;

  #if 0
  pixel_color = textured_color;
  #else

  // Directional Light Vectors

  vec3 N;
  if (has_normal_map) {

    vec3 offset_normal = texture(normal_sampler, v_in.uv).xyz;
    offset_normal = normalize(offset_normal * 2 - 1);
    //N = offset_normal;
    mat3 TBN = mat3(v_in.tangent, v_in.bitangent, v_in.normal);
    N = normalize(TBN * offset_normal);
  } else {
    N = normalize(v_in.normal);
  }

  vec3 light_p = vec3(7,6,2);
  float Shininess = 130;

  // View direction:
  vec3 V = normalize(v_in.camera_p - v_in.p);
  // vector from v_in.p to light source:
  vec3 R = light_p - v_in.p;
  // negative light direction:
  vec3 L = normalize(R);
  // Halfway vector (for specular):
  vec3 H = normalize(V+L);
  // Radius:
  float r = length(R);

  // B factor : prevents light from below the surface
  float B = 1.0;
  if (dot(N, L) < 0.00001) { B = 0.0; }

  // Contribution
  // B(N*L)
  float diffuseShade = max(dot(N, L), 0.0);
  float shininess = Shininess > 0 ? Shininess : 0.00001;
  // B(H*N)^ns
  float specularShade = B * pow(max(dot(H, N), 0.0), shininess);

  // For cast shadows :
  //float shadow_d = texture(point_light_shadowmap[light_num], -L).x;

  /*
  float in_shadow = 1;
  if (1/r < shadow_d - 0.01) {
      in_shadow = 0;
  }
  */

  float light_intensity = 12;
  float ambient_intensity = 4;
  vec3 light_ambient = vec3(0.6,0.7,0.8) * ambient_intensity;
  vec3 light_color = vec3(1.0,0.9,0.1) * light_intensity;
  vec3 diffuse_color = textured_color.xyz;
  vec3 specular_color = textured_color.xyz;

  // k*I_La
  vec3 ambient = diffuse_color * light_ambient;
  // k*I_L * B(N*L)
  vec3 diffuse = diffuseShade * diffuse_color * light_color;
  // k*I_L * B(N*H)^ns
  vec3 specular = specularShade * specular_color * light_color;

  float a = 0.2;
  float b = 0.4;
  float c = 0.6;

  float f_atten = 1.0 / (0.01 + a + b*r + c*r*r);
  //pixel_color = ambient + in_shadow * f_atten * (diffuse + specular);
  pixel_color = vec4(ambient + f_atten * (diffuse + specular), textured_color.a);
  pixel_color.rgb = tone(pixel_color.rgb, 0.04);
  #endif

}

)__";

