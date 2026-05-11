#version 150 core

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

out vec2 texCoord;
out vec3 worldPosition;
out vec3 worldNormal;

uniform mat4 mvp;
uniform mat4 modelMatrix;
uniform mat3 modelNormalMatrix;

#ifdef CLIPPING
    #pragma include clipplane.shaderinc
#endif

void main()
{
    // Pass through scaled texture coordinates
    texCoord = vertexTexCoord;

    gl_Position = mvp * vec4( vertexPosition, 1.0 );

    worldPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));

    worldNormal = normalize(modelNormalMatrix * vertexNormal);

#ifdef CLIPPING
    setClipDistance(worldPosition);
#endif
}
