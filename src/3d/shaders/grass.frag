#version 330 core

in vec2 texCoord;

in vec3 worldPosition;
in vec3 worldNormal;

out vec4 fragColor;

uniform vec4 tipColor;
uniform vec4 color;
uniform vec4 color2;
uniform vec4 aoColor;

#ifdef ENABLE_IBL
uniform samplerCube globalSpecularMap;
uniform int globalSpecularMipLevels;
uniform vec3 envLightSh[9];
uniform int envLightMode; // 1 = Skybox IBL, 0 = Solid Background
uniform float envLightStrength;
#endif

#pragma include light.inc.frag

void main(void)
{
  float h = texCoord.y;

  vec4 c1 = mix(aoColor, color, clamp(h * 3.0, 0.0, 1.0));
  vec4 c2 = mix(c1, color2, clamp((h - 0.3333) * 3.0, 0.0, 1.0));
  vec4 baseColor = mix(c2, tipColor, clamp((h - 0.6666) * 3.0, 0.0, 1.0));

  vec3 n = normalize(worldNormal);
    vec3 wView = normalize(eyePosition - worldPosition);

    // apply a basic ambient term to prevent completely black shadowed areas

vec3 ambientLighting = vec3(0.0);
#ifdef ENABLE_IBL
  if ( envLightMode == 1 && envLightStrength > 0.0 )
  {
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;

    vec3 envIrradiance =
        c1 * envLightSh[8] * (n.x * n.x - n.y * n.y) +
        c3 * envLightSh[6] * n.z * n.z +
        c4 * envLightSh[0] -
        c5 * envLightSh[6] +
        2.0 * c1 * (envLightSh[4] * n.x * n.y + envLightSh[7] * n.x * n.z + envLightSh[5] * n.y * n.z) +
        2.0 * c2 * (envLightSh[3] * n.x + envLightSh[1] * n.y + envLightSh[2] * n.z);

    envIrradiance = max(envIrradiance, vec3(0.0));
    ambientLighting = envIrradiance * envLightStrength;
  }
#endif
vec3 finalColor = baseColor.rgb * ambientLighting;
    // accumulate diffuse lighting from all active scene lights
    for (int i = 0; i < lightCount; ++i) {
      LightParams light = calculateLightParams(i, worldPosition, n, wView);

      vec3 diffuse = baseColor.rgb * lights[i].color * light.sDotN;

      // multiply by distance attenuation, spotlight falloff, and shadow visibility
      finalColor +=  light.att * lights[i].intensity * diffuse;
    }

    fragColor = mix(baseColor,vec4(finalColor, baseColor.a), 1.0);
}
