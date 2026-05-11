#version 150 core

in vec2 texCoord;

in vec3 worldPosition;
in vec3 worldNormal;

out vec4 fragColor;

uniform sampler2D diffuseTexture;

#pragma include light.inc.frag

void main()
{
    fragColor = texture( diffuseTexture, texCoord );

    if (renderShadows == 1)
    {
         vec3 lightDir = normalize(-lights[shadowLightIndex].direction);

        int cascadeIndex = calcCascadeIndexMapBased(worldPosition);
        float visibilityFactor = calcShadowFactor(cascadeIndex, worldPosition, worldNormal, lightDir);
        fragColor = vec4(fragColor.rgb * mix(0.5, 1.0, visibilityFactor), fragColor.a);
    }
}
