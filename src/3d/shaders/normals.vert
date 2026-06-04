#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
// 0 for start point, 1 for tip point
in float isTip;

uniform mat4 modelViewProjection;
uniform float normalLength;

void main()
{
    vec3 pos = vertexPosition + float(isTip) * vertexNormal * normalLength;
    gl_Position = modelViewProjection * vec4(pos, 1.0);
}