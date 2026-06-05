#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in vec3 instanceTranslation;
in vec3 instanceScale;
in vec4 instanceRotation;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 texCoord;

uniform mat4 modelMatrix;
uniform mat3 modelNormalMatrix;
uniform mat4 mvp;

uniform float currentTime;
uniform vec3 eyePosition;

#ifdef CLIPPING
    #pragma include clipplane.shaderinc
#endif

vec3 rotateByQuat(vec3 v, vec4 q) {
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main()
{
    const mat3 zUpTransform = mat3(
            1.0, 0.0, 0.0,
            0.0, 0.0, 1.0,
            0.0, -1.0, 0.0
        );

        const mat3 zUpNormalTransform = mat3(
            1.0, 0.0, 0.0,
            0.0, 0.0, 1.0,
            0.0, -1.0, 0.0
        );

        vec3 thisInstanceScale = instanceScale;
        vec3 thisInstanceNormalScale = 1.0 / instanceScale;
        vec4 thisInstanceRotation = instanceRotation;

        vec3 zUpPosition = zUpTransform * vertexPosition;
        vec3 scaledPosition = zUpPosition * thisInstanceScale;
        vec3 vertexPositionObject = rotateByQuat(scaledPosition, thisInstanceRotation);

        vec3 zUpNormal = zUpNormalTransform * vertexNormal;
        vec3 scaledNormal = zUpNormal * thisInstanceNormalScale;
        vec3 vertexNormalObject = rotateByQuat(scaledNormal, thisInstanceRotation);

        vec3 vertexPositionChunk = vertexPositionObject + instanceTranslation;
        worldPosition = vec3(modelMatrix * vec4(vertexPositionChunk, 1.0));

        float windInfluence = vertexTexCoord.y;

        float noise = sin(worldPosition.x * 2.0 + currentTime) * sin(worldPosition.y * 2.0 + currentTime);
        vec3 windDirection = vec3(1.0, 0.0, 0.0);
        float windStrength = 0.2;
        vec3 windOffset = windDirection * windStrength * noise * windInfluence;

        vertexPositionChunk += windOffset;

        // convert eye position to chunk space
        vec3 eyePositionChunk = (inverse(modelMatrix) * vec4(eyePosition, 1.0)).xyz;
        vec3 toCamera = normalize(eyePositionChunk - vertexPositionChunk);

        // shift vertices towards the camera to force orientation toward the view plane
        vertexPositionChunk += toCamera * windInfluence * 0.15;

        worldPosition = vec3(modelMatrix * vec4(vertexPositionChunk, 1.0));

        vec3 standardNormal = normalize(modelNormalMatrix * vertexNormalObject);
        vec3 upVector = vec3(0.0, 0.0, 1.0);
        worldNormal = normalize(mix(standardNormal, upVector, 0.75));

        texCoord = vertexTexCoord;

        gl_Position = mvp * vec4(vertexPositionChunk, 1.0);

    #ifdef CLIPPING
        setClipDistance(worldPosition);
    #endif
}
