#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2
#define PI 3.1415926538

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
  float nearDist;
  float farDist;
  float radius;
  int type;
};

/*float GSchlickGGX(float N_V, float k) {
  return N_V / (N_V * (1 - k) + k);
}

float GSmithGGX(float N_V, float N_L, float k) {
  return GSchlickGGX(N_V, k) * GSchlickGGX(N_L, k);
}

float NDFGGXTR(float N_H, float alpha) {
  float alpha2 = alpha * alpha;
  float denom = N_H * N_H * (alpha2 - 1);
  denom = denom * denom * PI;
  
  return alpha2 / denom; 
}

vec3 FSchlick(vec3 F_0, float H_V) {
  float val = 1 - H_V;
  return F_0 + (vec3(1) - F_0) * val * val * val * val * val;
}*/

float computeAttenFactor(vec3 attenuation, float dist) {
  float attenuation_factor = 
    attenuation.x + attenuation.y * dist + attenuation.z * dist * dist;
  
  if(attenuation_factor > 0)
    return 1.0 / attenuation_factor;
  
  return 1.0;
}

vec3 computeAttenuation(vec3 color, vec3 attenuation, float dist) {
  float attenuation_factor = 
    attenuation.x + attenuation.y * dist + attenuation.z * dist * dist;
  
  if(attenuation_factor > 0)
    color /= attenuation_factor;
  
  return color;
}

vec3 fresnelSchlick(float HdotV, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  
  float numer = a2;
  float denom = NdotH2 * (a2 - 1.0) + 1.0;
  
  // Roughness = 1
  // a = 1
  // denom = 1, numerator = 0, therefore D = 0, no specular.
  
  // Roughness = 0
  // a2 = 0
  // denom = -NdotH2 + 1
  // Denom == 0 IFF NdotH == 1
  
  // Why does specular disappear at roughness == 0?
  
  denom = PI * denom * denom;
  return numer / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1;
  float k = (r * r) / 8.0; // for direct light only
  
  float numer = NdotV;
  float denom = NdotV * (1.0 - k) + k;
  
  // k E [.125, .5]
  // .875 * NdotV + .125
  // .5   * NdotV + .5
  // so E [.125, 1] where NdotV is [0, 1]
  // Denominator is NEVER 0
  
  // Roughness = 1
  // r = 2
  // k = 1.5
  
  return numer / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {  
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);
  return ggx1;
}

vec3 computeDirectPBR(vec3 lightColor, vec3 lightPos, vec3 attenFn, float r,
                      vec3 objColor,  vec3 objPos,  vec3 N, vec3 V, float roughness, float metallic) {
  vec3 toPoint = lightPos - objPos;
  
  vec3 L = normalize(toPoint);
  vec3 H = normalize(L + V);
  
  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, V), 0.0);
  float HdotV = max(dot(H, V), 0.0);
  
  float distance = length(toPoint);
  
  if(distance > r)
    return vec3(0);
  
  float attenuation = computeAttenFactor(attenFn, distance);
  vec3  radiance  = lightColor * attenuation;
  
  vec3 F0 = mix(vec3(0.04), objColor, metallic);
  
  float NDF = DistributionGGX(N, H, roughness);
  float G   = GeometrySmith(NdotV, NdotL, roughness);
  vec3  F   = fresnelSchlick(HdotV, F0);
  
  vec3 numer    = NDF * G * F;
  float denom   = 4.0 * NdotV * NdotL;
  
  // the little rings we get are from the fresnel
  vec3 specular = numer / max(denom, 0.01);  
  
  // calculate diffuse light
  vec3 kS = F;
  vec3 diffuse = vec3(1.0) - kS;
  diffuse *= 1.0 - metallic;
  
  return (diffuse * objColor / PI + specular) * radiance * NdotL;
}

vec3 computeIBLPBR(vec3 bgColor, vec3 bgIrradiance, 
                   vec3 objColor, vec3 objPos, vec3 N, vec3 V, float roughness, float metallic) {
  // diffuse portion of IBL:
  vec3 kD = objColor;
  return kD * bgIrradiance / PI;

  // L in this case will be the normal vector, I think?

  // Specular portion requires some Monte Carlo shtuff
  // Choose N random directions, 20 to 40, W_k where the probability
  // of W_k is p(W_k) = D(H), or DistributionGGX(W_k, H, roughness)
  // // For each W_k, evaluate the light by getting the color from a MIPMAP at a specific level
  // // oh god I have to add mipmap generation to textures now
  // // frickems
}

/*vec3 computeDirectPBR(vec3 Ic, vec3 Ip, vec3 Ia, float r,
                      vec3 C,  vec3 P,  vec3 N, vec3 V, float alpha, float metallic) {
  vec3 toPoint = Ip - P;
  float dist = length(toPoint);
  
  if(dist > r)
    return vec3(0, 0, 0);
  
  // calculate radiance
  vec3 radiance = computeAttenuation(Ic, Ia, dist);
  
  vec3 L = normalize(toPoint);
  vec3 H = normalize(L + V);
  
  float N_V = max(dot(N, V), 0);
  float N_L = max(dot(N, L), 0);
  float N_H = max(dot(N, H), 0);
  float H_V = max(dot(H, V), 0);
  
  float k = (alpha + 1);
  k = k * k / 8;
  
  vec3 F_0 = vec3(0.04);
  F_0 = mix(F_0, C, metallic);
  
  float D = NDFGGXTR(N_H, alpha);
  float G = GSmithGGX(N_V, N_L, k);
  vec3  F = FSchlick(F_0, H_V);
  
  vec3  numerator   = D * G * F;
  float denominator = max(4 * N_L * N_V, 0.001); // in case of 0
  
  vec3 diffuseColor = C / PI;
  vec3 specularColor = numerator / denominator;
  
  //return specularColor + diffuseColor;
  
  return (diffuseColor + specularColor) * radiance * N_L;
  //return computeAttenuation(C, light.atten, dist);
}*/

// Phong non-shadowed light
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
  
  return computeAttenuation(color, light.atten, dist);
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