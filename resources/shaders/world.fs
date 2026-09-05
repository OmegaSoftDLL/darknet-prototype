#version 330
// Modelo de luz do mundo: half-lambert direcional (sol/lua) + ambiente colorido
// pela zona + rim light (separa a silhueta do fundo, truque padrao de ARPG
// isometrico) + nevoa de distancia ate a cor do horizonte.
//
// Camada "anti-plastico" (evolucao): as grandes faces opacas deixam de ser um
// muro plano de cor unica — ganham variacao sutil e coerente no ESPACO do
// mundo (fbm de 3 oitavas na coordenada global, NAO no UV), quebrando o
// "plastic look" sem custar textura. Superficies com extremo brilho recebem
// um especular seco (horizontes metalicos/enforcers) que devolve "venda".
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
uniform float specularK;     // forca do brilho especular (0 = sem)
uniform float worldPeriod;   // "tamanho" do mundo p/ a escala da variacao global

out vec4 finalColor;

float ihash(vec2 p) {
    vec2 q = fract(p * 0.3183099);
    return fract(sin(dot(q, vec2(12.9898, 78.233))) * 43758.5453);
}
float vnoise(vec2 p) {
    vec2 i = floor(p); vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = ihash(i);
    float b = ihash(i + vec2(1, 0));
    float c = ihash(i + vec2(0, 1));
    float d = ihash(i + vec2(1, 1));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
float fbm2(vec2 p) {
    float v = 0.0, amp = 0.5;
    for (int i = 0; i < 3; i++) { v += vnoise(p) * amp; p *= 2.07; amp *= 0.5; }
    return v;
}

void main() {
    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    vec3  N   = normalize(fragNormal);
    vec3  L   = normalize(-lightDir);
    vec3  V   = normalize(camPos - fragPosW);
    vec3  H   = normalize(L + V);
    float ndl = max(dot(N, L), 0.0);
    // half-lambert: o lado na sombra escurece mas NAO vira preto chapado
    float lam = ndl * 0.5 + 0.5;

    // ── anti-plastico: variacao coerente no espaco do mundo ──────────────────
    // So para superficie OPAQUA (alpha > 0.999): chao, paredes, corpos metalicos.
    // Decal semi-transparente (chao pintado, sombra projetada) nao recebe —
    // senao a "sujeira" vira listra sobre o que e flat de proposito.
    float detail = 0.0;
    if (base.a > 0.999) {
        float pw = 1.06 / max(worldPeriod, 1.0);
        vec2  wp = vec2(fragPosW.x * pw, fragPosW.z * pw * 0.87);
        float cloud = fbm2(wp);                    // manchas grandes/medias
        float shaft = 0.5 + 0.5 * sin(fragPosW.x * 0.017 + fragPosW.z * 0.013);
        detail = (cloud - 0.5) * 0.13 + (shaft - 0.5) * 0.05;
    }

    vec3 lit = base.rgb * (ambientColor + lightColor * lam * 0.95) * (1.0 + detail);

    // Rim: so em superficie que ja tem cor (senao a SOMBRA projetada, que e
    // desenhada preta, ganharia um contorno brilhante).
    float rim  = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    float lumi = max(base.r, max(base.g, base.b));
    lit += lightColor * rim * rimStrength * lumi;

    // ── especular: rodela seca em superficie clara e opaca ───────────────────
    // Espelha o sol/lua em materiaopa: da "venda" em metal, lataria e placas.
    float sp = pow(max(dot(N, H), 0.0), 26.0);
    if (base.a > 0.999) lit += lightColor * sp * specularK * lumi;

    float d = length(camPos - fragPosW);
    float f = clamp((d - fogStart) / max(fogEnd - fogStart, 1.0), 0.0, 1.0);
    lit = mix(lit, fogColor, f * 0.9);

    finalColor = vec4(lit, base.a);
}