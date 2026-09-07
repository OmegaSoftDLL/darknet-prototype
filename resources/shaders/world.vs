#version 330
// Passa normal e posicao de MUNDO para o fragment: sem isso nao existe luz
// direcional nem nevoa por distancia — era o que faltava para o 3D ter volume.
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
