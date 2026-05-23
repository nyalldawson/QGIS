
// copy of light.inc.frag from qt3d extras

const int MAX_LIGHTS = 8;
const int TYPE_POINT = 0;
const int TYPE_DIRECTIONAL = 1;
const int TYPE_SPOT = 2;
struct Light {
    int type;
    vec3 position;
    vec3 color;
    float intensity;
    vec3 direction;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float cutOffAngle;
};
uniform Light lights[MAX_LIGHTS];
uniform int lightCount;

// Pre-convolved environment maps
struct EnvironmentLight {
    samplerCube irradiance; // For diffuse contribution
    samplerCube specular; // For specular contribution
        int specularMipLevels;
};

struct LightParams
{
  vec3 s; // light direction vector (from surface to light source)
  float sDotN; // NoL (filament). Cosine of the angle between light direction and surface normal. Clamped to >= 0.0
  float att; // Distance-based attenuation and spot light cone falloff multiplier.
  float visibilityFactor; // visibility after shadowing applied. 0 = no visibility, completely shadowed. 1 = completely visible, no shadowing
  vec3 h; // Half-vector between the light direction (s) and the view direction.
  float sDotH; // Cosine of the angle between light direction and half-vector (L dot H). Clamped to >= 0.0.
  float nDotH; // Cosine of the angle between surface normal and half-vector (N dot H). Clamped to >= 0.0.
};

uniform EnvironmentLight envLight;
uniform int envLightCount = 0;

#pragma include shadows.inc.frag

void adsModelNormalMapped(const in vec3 worldPos,
                          const in vec3 tsNormal,
                          const in vec3 worldEye,
                          const in float shininess,
                          const in mat3 tangentMatrix,
                          out vec3 diffuseColor,
                          out vec3 specularColor)
{
    diffuseColor = vec3(0.0);
    specularColor = vec3(0.0);

    // We perform all work in tangent space, so we convert quantities from world space
    vec3 n = normalize(tsNormal);
    vec3 v = normalize(tangentMatrix * (worldEye - worldPos));
    vec3 s = vec3(0.0);

    for (int i = 0; i < lightCount; ++i) {
        float att = 1.0;
        float sDotN = 0.0;
        float visibilityFactor = 1.0;

        if (lights[i].type != TYPE_DIRECTIONAL) {
            // Point and Spot lights

            // Transform the light position from world to tangent space
            vec3 worldLightDir = lights[i].position - worldPos;
            vec3 sUnnormalized = tangentMatrix * worldLightDir;
            s = normalize(sUnnormalized); // Light direction in tangent space

            // Calculate the attenuation factor
            sDotN = dot(s, n);
            if (sDotN > 0.0) {
                if (lights[i].constantAttenuation != 0.0
                 || lights[i].linearAttenuation != 0.0
                 || lights[i].quadraticAttenuation != 0.0) {
                    float dist = length(sUnnormalized);
                    att = 1.0 / (lights[i].constantAttenuation +
                                 lights[i].linearAttenuation * dist +
                                 lights[i].quadraticAttenuation * dist * dist);
                }

                // The light direction is in world space, convert to tangent space
                if (lights[i].type == TYPE_SPOT) {
                    // Check if fragment is inside or outside of the spot light cone
                    vec3 tsLightDirection = normalize(tangentMatrix * lights[i].direction);
                    float cutOffCos = cos(radians(lights[i].cutOffAngle));
                    if (dot(-s, tsLightDirection) < cutOffCos)
                        sDotN = 0.0;
                }
            }
        } else {
            // Directional lights
            // The light direction is in world space, convert to tangent space
            s = normalize(tangentMatrix * -lights[i].direction);
            sDotN = dot(s, n);

            if (renderShadows == 1 && i == shadowLightIndex)
            {
                visibilityFactor = calcVisibilityAfterShadowing(worldPos);
            }
        }

        // Calculate the diffuse factor
        float diffuse = max(sDotN, 0.0);

        // Calculate the specular factor
        float specular = 0.0;
        if (diffuse > 0.0 && shininess > 0.0) {
            float normFactor = (shininess + 2.0) / (2.0 * 3.14159);
            vec3 r = reflect(-s, n);   // Reflection direction in tangent space
            specular = normFactor * pow(max(dot(r, v), 0.0), shininess);
        }

        // Accumulate the diffuse and specular contributions
        diffuseColor += visibilityFactor * att * lights[i].intensity * diffuse * lights[i].color;
        specularColor += visibilityFactor * att * lights[i].intensity * specular * lights[i].color;
    }
}

void adsModel(const in vec3 worldPos,
              const in vec3 worldNormal,
              const in vec3 worldView,
              const in float shininess,
              out vec3 diffuseColor,
              out vec3 specularColor)
{
    diffuseColor = vec3(0.0);
    specularColor = vec3(0.0);

    // We perform all work in world space
    vec3 n = normalize(worldNormal);
    vec3 s = vec3(0.0);

    for (int i = 0; i < lightCount; ++i) {
        float att = 1.0;
        float sDotN = 0.0;
        float visibilityFactor = 1.0;

        if (lights[i].type != TYPE_DIRECTIONAL) {
            // Point and Spot lights

            // Light position is already in world space
            vec3 sUnnormalized = lights[i].position - worldPos;
            s = normalize(sUnnormalized); // Light direction

            // Calculate the attenuation factor
            sDotN = dot(s, n);
            if (sDotN > 0.0) {
                if (lights[i].constantAttenuation != 0.0
                 || lights[i].linearAttenuation != 0.0
                 || lights[i].quadraticAttenuation != 0.0) {
                    float dist = length(sUnnormalized);
                    att = 1.0 / (lights[i].constantAttenuation +
                                 lights[i].linearAttenuation * dist +
                                 lights[i].quadraticAttenuation * dist * dist);
                }

                // The light direction is in world space already
                if (lights[i].type == TYPE_SPOT) {
                    // Check if fragment is inside or outside of the spot light cone
                    float cutOffCos = cos(radians(lights[i].cutOffAngle));
                    if (dot(-s, lights[i].direction) < cutOffCos)
                        sDotN = 0.0;
                }
            }
        } else {
            // Directional lights
            // The light direction is in world space already
            s = normalize(-lights[i].direction);
            sDotN = dot(s, n);

            if (renderShadows == 1 && i == shadowLightIndex)
            {
                visibilityFactor = calcVisibilityAfterShadowing(worldPos);
            }
        }

        // Calculate the diffuse factor
        float diffuse = max(sDotN, 0.0);

        // Calculate the specular factor
        float specular = 0.0;
        if (diffuse > 0.0 && shininess > 0.0) {
            float normFactor = (shininess + 2.0) / (2.0 * 3.14159);
            vec3 r = reflect(-s, n);   // Reflection direction in world space
            specular = normFactor * pow(max(dot(r, worldView), 0.0), shininess);
        }

        // Accumulate the diffuse and specular contributions
        diffuseColor += visibilityFactor * att * lights[i].intensity * diffuse * lights[i].color;
        specularColor += visibilityFactor * att * lights[i].intensity * specular * lights[i].color;
    }
}

void adModel(const in vec3 worldPos,
             const in vec3 worldNormal,
             out vec3 diffuseColor)
{
    diffuseColor = vec3(0.0);

    // We perform all work in world space
    vec3 n = normalize(worldNormal);
    vec3 s = vec3(0.0);

    for (int i = 0; i < lightCount; ++i) {
        float att = 1.0;
        float sDotN = 0.0;
        float visibilityFactor = 1.0;

        if (lights[i].type != TYPE_DIRECTIONAL) {
            // Point and Spot lights

            // Light position is already in world space
            vec3 sUnnormalized = lights[i].position - worldPos;
            s = normalize(sUnnormalized); // Light direction

            // Calculate the attenuation factor
            sDotN = dot(s, n);
            if (sDotN > 0.0) {
                if (lights[i].constantAttenuation != 0.0
                 || lights[i].linearAttenuation != 0.0
                 || lights[i].quadraticAttenuation != 0.0) {
                    float dist = length(sUnnormalized);
                    att = 1.0 / (lights[i].constantAttenuation +
                                 lights[i].linearAttenuation * dist +
                                 lights[i].quadraticAttenuation * dist * dist);
                }

                // The light direction is in world space already
                if (lights[i].type == TYPE_SPOT) {
                    // Check if fragment is inside or outside of the spot light cone
                    float cutOffCos = cos(radians(lights[i].cutOffAngle));
                    if (dot(-s, lights[i].direction) < cutOffCos)
                        sDotN = 0.0;
                }
            }
        } else {
            // Directional lights
            // The light direction is in world space already
            s = normalize(-lights[i].direction);
            sDotN = dot(s, n);

            if (renderShadows == 1 && i == shadowLightIndex)
            {
                visibilityFactor = calcVisibilityAfterShadowing(worldPos);
            }
        }

        // Calculate the diffuse factor
        float diffuse = max(sDotN, 0.0);

        // Accumulate the diffuse contributions
        diffuseColor += visibilityFactor * att * lights[i].intensity * diffuse * lights[i].color;
    }
}

LightParams calculatePbrLightParams(const in int lightIndex,
                                   const in vec3 wPosition,
                                   const in vec3 wNormal,
                                   const in vec3 wView)
{
    LightParams res;
    res.s = vec3(0.0);
    res.att = 1.0;
    res.sDotN = 0.0;
    res.visibilityFactor = 1.0;

    if (lights[lightIndex].type != TYPE_DIRECTIONAL)
    {
        // Point and Spot lights
        vec3 sUnnormalized = vec3(lights[lightIndex].position) - wPosition;
        res.s = normalize(sUnnormalized);

        // Calculate the attenuation factor
        res.sDotN = dot(res.s, wNormal);
        if (res.sDotN > 0.0) {
            if (lights[lightIndex].constantAttenuation != 0.0
             || lights[lightIndex].linearAttenuation != 0.0
             || lights[lightIndex].quadraticAttenuation != 0.0) {
                float dist = length(sUnnormalized);
                res.att = 1.0 / (lights[lightIndex].constantAttenuation +
                             lights[lightIndex].linearAttenuation * dist +
                             lights[lightIndex].quadraticAttenuation * dist * dist);
            }

            // The light direction is in world space already
            if (lights[lightIndex].type == TYPE_SPOT) {
                // Check if fragment is inside or outside of the spot light cone
                if (degrees(acos(dot(-res.s, lights[lightIndex].direction))) > lights[lightIndex].cutOffAngle)
                    res.sDotN = 0.0;
            }
        }
    } else {
        // Directional lights
        // The light direction is in world space already
        res.s = normalize(-lights[lightIndex].direction);
        res.sDotN = dot(res.s, wNormal);

        if (renderShadows == 1 && lightIndex == shadowLightIndex)
        {
            res.visibilityFactor = calcVisibilityAfterShadowing(wPosition);
        }
    }
    res.h = normalize(res.s + wView);
    res.sDotH = max(dot(res.s, res.h), 0.0);
    res.sDotN = max(res.sDotN, 0.0);
    res.nDotH = max(dot(wNormal, res.h), 0.0);
    return res;
}
