#version 150 core

in vec3 vertexPosition;
in vec2 vertexTexCoord;

out vec2 texCoord;
out vec3 worldPosition;

uniform mat4 mvp;
uniform mat4 modelMatrix;

#ifdef CLIPPING
    #pragma include clipplane.shaderinc
#endif

void main()
{
    // Pass through scaled texture coordinates
    texCoord = vertexTexCoord;

    gl_Position = mvp * vec4( vertexPosition, 1.0 );

    worldPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));

#ifdef CLIPPING
    setClipDistance(worldPosition);
#endif
}
