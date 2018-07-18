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

uniform bool HAS_NORMAL_MAP;
uniform sampler2D TEXTURE_SAMPLER;
uniform sampler2D NORMAL_SAMPLER;
uniform vec3 LIGHT_P;

const mediump vec3 perception = vec3(0.299, 0.587, 0.114);

vec3 tone(vec3 v, float exposure) {
  float lum = dot(v, perception) * exposure;
  lum = sqrt(lum / (1 + lum));
  return v * lum;
}

vec3 point_light(vec3 diffuse_color, vec3 specular_color, vec3 N, vec3 V, vec3 light_p, vec3 light_color, vec3 abc, float shininess) {

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
  shininess = max(shininess, 0.00001); //Shininess > 0 ? Shininess : 0.00001;
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

  // k*I_L * B(N*L)
  vec3 diffuse = diffuseShade * diffuse_color * light_color;
  // k*I_L * B(N*H)^ns
  vec3 specular = specularShade * specular_color * light_color;

  float a = abc.x;
  float b = abc.y;
  float c = abc.z;

  float f_atten = 1.0 / (0.01 + a + b*r + c*r*r);
  //return in_shadow * f_atten * (diffuse + specular);
  return f_atten * (diffuse + specular);
}

void main() {
  vec4 textured_color = v_in.color * texture(TEXTURE_SAMPLER, v_in.uv);
  if (textured_color.a < 0.001) discard;

  #if 0
  pixel_color = textured_color;
  #else

  vec3 N;
  if (HAS_NORMAL_MAP) {

    vec3 offset_normal = texture(NORMAL_SAMPLER, v_in.uv).xyz;
    offset_normal = normalize(offset_normal * 2 - 1);
    //offset_normal.y *= -1;
    //offset_normal.z *= -1;
    //N = offset_normal;
    // TODO figure out what is going on with Z being the wrong way...
    mat3 TBN = mat3(v_in.tangent, v_in.bitangent, v_in.normal);
    N = normalize(TBN * offset_normal);
  } else {
    N = normalize(v_in.normal);
  }

  float shininess = 40;

  // View direction:
  vec3 V = normalize(v_in.camera_p - v_in.p);

  float light_intensity = 10;
  vec3 light_color = vec3(1.0,0.9,0.1) * light_intensity;

  vec3 diffuse_color = textured_color.xyz;
  vec3 specular_color = textured_color.xyz;

  vec3 abc = vec3(0.2, 0.5, 0.05);

  vec3 point_light_color = point_light(diffuse_color, specular_color, N, V, LIGHT_P, light_color, abc, shininess);

  vec3 other_light_color = vec3(0, 1, 1) * 4;
  vec3 point_light_color_2 = point_light(diffuse_color, specular_color, N, V, vec3(6,8,4), other_light_color, abc / 2, shininess);


  float ambient_intensity = 4;
  vec3 ambient_light = vec3(0.6,0.7,0.8) * ambient_intensity;

  // k*I_La
  vec3 ambient_color = diffuse_color * ambient_light;

  pixel_color = vec4(ambient_color + point_light_color + point_light_color_2, textured_color.a);
  pixel_color.rgb = tone(pixel_color.rgb, 0.04);
  #endif

}

)__";

