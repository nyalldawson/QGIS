#version 330
uniform sampler2D srcTexture;
in vec2 texCoord;
out vec4 fragColor;

#ifdef FIRST_PASS
vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0 / 2.2;
vec3 ToSRGB(vec3 v) { return PowVec3(v, invGamma); }

float RGBToLuminance(vec3 col)
{
    return dot(col, vec3(0.2126f, 0.7152f, 0.0722f));
}

float KarisAverage(vec3 col)
{
    // Clamp to prevent NaN/inversion from negative HDR values
        vec3 safeCol = max(col, vec3(0.0f));

        // Calculate linear luminance (do NOT convert to sRGB first)
        float luma = RGBToLuminance(safeCol) * 0.25f;

        return 1.0f / (1.0f + luma);
}
#endif

void main() {
    vec2 srcTexelSize = 1.0 / textureSize(srcTexture, 0);
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    // 13-Tap filter (following CoD: Advanced Warfare approach, from ACM Siggraph 2014)
    // see also https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===

    vec3 a = texture(srcTexture, vec2(texCoord.x - 2.0*x, texCoord.y + 2.0*y)).rgb;
    vec3 b = texture(srcTexture, vec2(texCoord.x,         texCoord.y + 2.0*y)).rgb;
    vec3 c = texture(srcTexture, vec2(texCoord.x + 2.0*x, texCoord.y + 2.0*y)).rgb;

    vec3 d = texture(srcTexture, vec2(texCoord.x - 2.0*x, texCoord.y)).rgb;
    vec3 e = texture(srcTexture, vec2(texCoord.x,         texCoord.y)).rgb;
    vec3 f = texture(srcTexture, vec2(texCoord.x + 2.0*x, texCoord.y)).rgb;

    vec3 g = texture(srcTexture, vec2(texCoord.x - 2.0*x, texCoord.y - 2.0*y)).rgb;
    vec3 h = texture(srcTexture, vec2(texCoord.x,         texCoord.y - 2.0*y)).rgb;
    vec3 i = texture(srcTexture, vec2(texCoord.x + 2.0*x, texCoord.y - 2.0*y)).rgb;

    vec3 j = texture(srcTexture, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 k = texture(srcTexture, vec2(texCoord.x + x, texCoord.y + y)).rgb;
    vec3 l = texture(srcTexture, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 m = texture(srcTexture, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    // From https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
    // "Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1" (Jorge Jimenez)
#ifdef FIRST_PASS
    vec3 sub0 = (a + b + d + e) * 0.25f;
        vec3 sub1 = (b + c + e + f) * 0.25f;
        vec3 sub2 = (d + e + g + h) * 0.25f;
        vec3 sub3 = (e + f + h + i) * 0.25f;
        vec3 sub4 = (j + k + l + m) * 0.25f;
        float w0 = KarisAverage(sub0);
            float w1 = KarisAverage(sub1);
            float w2 = KarisAverage(sub2);
            float w3 = KarisAverage(sub3);
            float w4 = KarisAverage(sub4);

            vec3 colorOut = sub0 * w0 * 0.125f +
                                sub1 * w1 * 0.125f +
                                sub2 * w2 * 0.125f +
                                sub3 * w3 * 0.125f +
                                sub4 * w4 * 0.500f;
            float totalWeight = (w0 * 0.125f) + (w1 * 0.125f) + (w2 * 0.125f) + (w3 * 0.125f) + (w4 * 0.500f);

                fragColor = vec4(colorOut / totalWeight, 1.0);
#else
    vec3 bloom = e * 0.125;
    bloom += (a + c + g + i) * 0.03125;
    bloom += (b + d + f + h) * 0.0625;
    bloom += (j + k + l + m) * 0.125;
    fragColor = vec4(bloom, 1.0);
#endif
}
