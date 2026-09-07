#version 330
// World lighting model: directional half-Lambert (sun/moon) + environment tinted
// by zone + rim light (separates the silhouette from the background, a standard
// isometric ARPG trick) + distance fog that fades toward the horizon color.
//
// "Anti-plastic" layer (evolution): large opaque faces stop looking like a flat
// wall of uniform color and gain subtle, coherent variation in WORLD space
// (3-octave FBM in global coordinates, NOT in UV space), breaking the plastic
// look without adding texture cost. Extremely glowing surfaces get a tight
// specular highlight (metal horizons/enforcers) that adds a bit of "pop".
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosW;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3  lightDir;      // direction the light travels (from sun/moon into the scene)
uniform vec3  lightColor;
uniform vec3  ambientColor;
uniform vec3  camPos;
uniform vec3  fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float rimStrength;
uniform float specularK;     // specular highlight strength (0 = none)
uniform float worldPeriod;   // world "size" used to scale the global variation

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
    // Half-Lambert: the shadow side darkens but does NOT turn into flat black
    float lam = ndl * 0.5 + 0.5;

    // ── anti-plastic: coherent variation in world space ─────────────────────
    // Only for OPAQUE surfaces (alpha > 0.999): floors, walls, metal bodies.
    // Semi-transparent decals (painted floor, projected shadow) skip it —
    // otherwise a dirt patch would turn into a streak across intentionally flat
    // surfaces.
    float detail = 0.0;
    if (base.a > 0.999) {
        float pw = 1.06 / max(worldPeriod, 1.0);
        vec2  wp = vec2(fragPosW.x * pw, fragPosW.z * pw * 0.87);
        float cloud = fbm2(wp);                    // large/medium blotches
        float shaft = 0.5 + 0.5 * sin(fragPosW.x * 0.017 + fragPosW.z * 0.013);
        detail = (cloud - 0.5) * 0.13 + (shaft - 0.5) * 0.05;
    }

    vec3 lit = base.rgb * (ambientColor + lightColor * lam * 0.95) * (1.0 + detail);

    // Rim: only on surfaces that already have color (otherwise a projected
    // shadow, which is drawn black, would get a bright outline).
    float rim  = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    float lumi = max(base.r, max(base.g, base.b));
    lit += lightColor * rim * rimStrength * lumi;

    // ── specular: tight highlight on bright opaque surfaces ────────────────
    // Mirrors the sun/moon on opaque material: adds "pop" to metal, sheet
    // metal and plates.
    float sp = pow(max(dot(N, H), 0.0), 26.0);
    if (base.a > 0.999) lit += lightColor * sp * specularK * lumi;

    float d = length(camPos - fragPosW);
    float f = clamp((d - fogStart) / max(fogEnd - fogStart, 1.0), 0.0, 1.0);
    lit = mix(lit, fogColor, f * 0.9);

    finalColor = vec4(lit, base.a);
}