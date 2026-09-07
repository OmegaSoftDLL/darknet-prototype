#version 330
// Separable 9-tap Gaussian blur. Runs twice (horizontal + vertical) on a
// quarter-resolution target — cheap and enough for the bloom halo.
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 direction;    // (1/w, 0) or (0, 1/h)
out vec4 finalColor;

const float W[5] = float[](0.227027, 0.194595, 0.121622, 0.054054, 0.016216);

void main() {
    vec3 s = texture(texture0, fragTexCoord).rgb * W[0];
    for (int i = 1; i < 5; ++i) {
        vec2 o = direction * float(i) * 1.35;
        s += texture(texture0, fragTexCoord + o).rgb * W[i];
        s += texture(texture0, fragTexCoord - o).rgb * W[i];
    }
    finalColor = vec4(s, 1.0);
}
