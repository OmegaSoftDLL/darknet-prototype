#version 330
// Passo 1 do bloom: isola so o que e REALMENTE brilhante (laser, fogo, luz, HUD
// aceso). Threshold com joelho suave pra nao criar borda dura no que passa.
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float threshold;
uniform float knee;
out vec4 finalColor;

void main() {
    vec3 c = texture(texture0, fragTexCoord).rgb;
    float l = max(c.r, max(c.g, c.b));
    float t = smoothstep(threshold, threshold + knee, l);
    finalColor = vec4(c * t, 1.0);
}
