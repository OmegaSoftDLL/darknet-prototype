#version 330
// Composicao final: cena + bloom, tonemap filmico (ACES aproximado), correcao de
// exposicao/contraste/saturacao. E aqui que a cena deixa de ser "chapada e
// escura" e ganha faixa dinamica — sem apagar as sombras.
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;   // cena
uniform sampler2D texture1;   // bloom borrado
uniform vec4 colDiffuse;
uniform float bloomStrength;
uniform float exposure;
uniform float saturation;
uniform float contrast;
out vec4 finalColor;

vec3 aces(vec3 x) {           // Narkowicz 2015
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 scene = texture(texture0, fragTexCoord).rgb;
    vec3 bloom = texture(texture1, fragTexCoord).rgb;
    vec3 col   = scene + bloom * bloomStrength;
    col = aces(col * exposure);
    // contraste em torno do cinza medio (nao do preto: nao afunda as sombras)
    col = clamp((col - 0.5) * contrast + 0.5, 0.0, 1.0);
    float l = dot(col, vec3(0.2126, 0.7152, 0.0722));
    col = clamp(mix(vec3(l), col, saturation), 0.0, 1.0);
    finalColor = vec4(col, 1.0) * colDiffuse * fragColor;
}
