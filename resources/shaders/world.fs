#version 330
// Modelo de luz do mundo: half-lambert direcional (sol/lua) + ambiente colorido
// pela zona + rim light (separa a silhueta do fundo, truque padrao de ARPG
// isometrico) + nevoa de distancia ate a cor do horizonte.
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosW;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3  lightDir;      // direcao QUE A LUZ VIAJA (do sol para a cena)
uniform vec3  lightColor;
uniform vec3  ambientColor;
uniform vec3  camPos;
uniform vec3  fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float rimStrength;

out vec4 finalColor;

void main() {
    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    vec3  N   = normalize(fragNormal);
    vec3  L   = normalize(-lightDir);
    float ndl = max(dot(N, L), 0.0);
    // half-lambert: o lado na sombra escurece mas NAO vira preto chapado
    float lam = ndl * 0.5 + 0.5;

    vec3 lit = base.rgb * (ambientColor + lightColor * lam * 0.95);

    // Rim: so em superficie que ja tem cor (senao a SOMBRA projetada, que e
    // desenhada preta, ganharia um contorno brilhante).
    vec3  V    = normalize(camPos - fragPosW);
    float rim  = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    float lumi = max(base.r, max(base.g, base.b));
    lit += lightColor * rim * rimStrength * lumi;

    float d = length(camPos - fragPosW);
    float f = clamp((d - fogStart) / max(fogEnd - fogStart, 1.0), 0.0, 1.0);
    lit = mix(lit, fogColor, f * 0.9);

    finalColor = vec4(lit, base.a);
}
