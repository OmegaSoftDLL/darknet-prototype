#version 330
// Bloom pass 1: isolate only what is REALLY bright (lasers, fire, lights, lit HUD).
// Smooth knee threshold so bright objects do not produce a hard edge in later passes.
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
