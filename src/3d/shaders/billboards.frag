#version 330 core

uniform sampler2D tex0;

in vec2 UV;

// accumulates pre-multiplied color values
layout(location = 0) out vec4 accum;
// stores pixel revealage
layout(location = 1) out float reveal;

float getWeight(float z, vec4 color) {
    // from https://learnopengl.com/Guest-Articles/2020/OIT/Weighted-Blended
    // the color-based factor
    // avoids color pollution from the edges of wispy clouds. the z-based
    // factor gives precedence to nearer surfaces

    // from https://casual-effects.blogspot.com/2015/03/implemented-weighted-blended-order.html
    return clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 *
                                 pow(1.0 - z * 0.9, 3.0), 1e-2, 3e3);


    return max(min(1.0, max(max(color.r, color.g), color.b) * color.a), color.a) * clamp(0.03 / (1e-5 + pow(z / 200, 4.0)), 1e-2, 3e3);
   //return max(0.01, (0.01 + color.a) * (25.0 * pow(1.0 - z, 4.0)));
}

void main(void) {
    vec4 color = texture(tex0, vec2(UV.x, 1.0f - UV.y));
    //if (color.a < 0.01)
    //    discard;

    vec4 unmultipliedColor = vec4(color.rgb / color.a, color.a);

    float weight = getWeight(gl_FragCoord.z, unmultipliedColor);
    // switch to pre-multiplied alpha and weight
    accum = vec4(color.rgb, color.a) * weight;
    reveal = color.a;
}
