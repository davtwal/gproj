struct Light {
  vec3 pos;
  vec3 dir;
  vec3 color;
  vec3 atten;
  float radius;
  int type;
};

struct ShadowLight {
  mat4 view;
  mat4 proj;
  vec3 pos;
  vec3 dir;
  vec3 color;
  vec3 atten;
  float radius;
  int type;
};

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2

#define MAX_GLOBAL_LIGHTS 1
#define MAX_DYNAMIC_LOCAL_LIGHTS 128

vec3 ComputeLighting(Light light, vec3 objColor, vec3 point, vec3 N, vec3 V, float specExp) {
  vec3 color = vec3(0, 0, 0);
  
  vec3 toPoint = light.pos - point;
  float dist = length(toPoint);
  
  if(dist > light.radius)
    return color;
  
  vec3 L = normalize(toPoint);
  
  float n_l = max(dot(N, L), 0);
  
  color += objColor * n_l * light.color;
  
  // specular
  vec3 R = reflect(-L, N);
  float r_v = pow(max(dot(R, V), 0), specExp);
  
  color += objColor * r_v * light.color;
  
  // attenuation
  vec3 atten_func = light.atten;
  float attenuation_factor = 
    atten_func.x + atten_func.y * dist + atten_func.z * dist * dist;
  
  if(attenuation_factor > 0)
    color *= 1 / attenuation_factor;
  
  return color;
}

vec3 ComputeShadowLighting(ShadowLight light, vec3 objColor, vec3 point, vec3 N, vec3 V, float specExp) {
  vec3 color = vec3(0, 0, 0);
  
  vec3 toPoint = light.pos - point;
  float dist = length(toPoint);
  
  vec3 L = normalize(toPoint);
  
  float n_l = max(dot(N, L), 0);
  
  color += objColor * n_l * light.color;
  
  // specular
  vec3 R = reflect(-L, N);
  float r_v = pow(max(dot(R, V), 0), specExp);
  
  color += objColor * r_v * light.color;
  
  // attenuation
  vec3 atten_func = light.atten;
  float attenuation_factor = 
    atten_func.x + atten_func.y * dist + atten_func.z * dist * dist;
  
  if(attenuation_factor > 0)
    color *= 1 / attenuation_factor;
  
  return color;
}