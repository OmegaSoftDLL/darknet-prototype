#version 330
// Pass world-space normal and position to the fragment shader. Without these,
// directional light and distance fog do not work — this was missing for the 3D
// scene to feel volumetric.
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragPosW;

void main() {
    fragTexCoord = vertexTexCoord;
    fragColor    = vertexColor;
    fragNormal   = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    fragPosW     = vec3(matModel * vec4(vertexPosition, 1.0));
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
