R"__(
smooth in vec2 frag_uv;
smooth in vec4 frag_color;
smooth in vec3 frag_normal;
smooth in vec3 frag_p;
smooth in vec3 camera_p;
out vec4 pixel_color;

uniform sampler2D texture_sampler;

void main() {
  vec4 textured_color = frag_color * texture(texture_sampler, frag_uv);
  if (textured_color.a < 0.01) discard;

  vec3 light_p = vec3(7,6,20);
  float Shininess = 1;

  // Directional Light Vectors
  vec3 N = normalize(frag_normal);
  // View direction:
  vec3 V = normalize(camera_p - frag_p);
  // vector from frag_p to light source:
  vec3 R = light_p - frag_p;
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

  float light_intensity = 21;
  float ambient_intensity = 0.8;
  vec3 light_ambient = vec3(0.6,0.7,0.8) * ambient_intensity;
  vec3 light_color = vec3(1.0,0.9,0.7) * light_intensity;
  vec3 diffuse_color = textured_color.xyz;
  vec3 specular_color = textured_color.xyz;

  // k*I_La
  vec3 ambient = diffuse_color * light_ambient;
  // k*I_L * B(N*L)
  vec3 diffuse = diffuseShade * diffuse_color * light_color;
  // k*I_L * B(N*H)^ns
  vec3 specular = specularShade * specular_color * light_color;

  float a = 0.1;
  float b = 0.3;
  float c = 0.2;



  float f_atten = 1.0 / (0.01 + a + b*r + c*r*r);
  //pixel_color = ambient + in_shadow * f_atten * (diffuse + specular);
  pixel_color = vec4(ambient + f_atten * (diffuse + specular), textured_color.a);

}

)__";

