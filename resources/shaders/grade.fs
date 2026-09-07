#version 330
// Final composite: scene + bloom, filmic tonemap (approximated ACES), plus
// exposure/contrast/saturation correction. This is where the scene stops looking
// flat and dark and gains dynamic range — without crushing the shadows.
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;   // scene
uniform sampler2D texture1;   // blurred bloom
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
    // contrast around medium gray (not black, so shadows do not get crushed)
    col = clamp((col - 0.5) * contrast + 0.5, 0.0, 1.0);
    float l = dot(col, vec3(0.2126, 0.7152, 0.0722));
    col = clamp(mix(vec3(l), col, saturation), 0.0, 1.0);
    // ── vignette: cinematic framing. Kept light — it only reins in edges where
    // bloom would blow out, rather than darkening the whole image.
    vec2 vc = fragTexCoord - 0.5;
    float vd = length(vc) * 1.35;
    float vig = 1.0 - smoothstep(0.52, 0.95, vd) * 0.32;
    col *= vig;
    finalColor = vec4(col, 1.0) * colDiffuse * fragColor;
}
