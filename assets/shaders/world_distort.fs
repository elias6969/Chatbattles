#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2  resolution;
uniform float time;

#define MAX_FX 6
uniform int   fxCount;
uniform vec2  fxCenter[MAX_FX];   // pixel coords (top-left origin)
uniform float fxRadius[MAX_FX];   // pixels
uniform float fxStrength[MAX_FX]; // 0..1
uniform int   fxType[MAX_FX];     // 0=BlackHole 1=TimeWarp 2=Nuke 3=Vortex

void main() {
    vec2 uv = fragTexCoord;
    vec2 px = uv * resolution;

    vec2 disp = vec2(0.0);
    float chrom = 0.0;
    float darken = 0.0;

    for (int i = 0; i < MAX_FX; ++i) {
        if (i >= fxCount) break;

        vec2 d = px - fxCenter[i];
        float dist = length(d);
        float r = fxRadius[i];
        if (r < 1.0 || dist > r) continue;

        float k = 1.0 - dist / r;
        vec2 dir = (dist > 0.001) ? d / dist : vec2(1.0, 0.0);
        vec2 tang = vec2(-dir.y, dir.x);
        float s = fxStrength[i];

        if (fxType[i] == 0) {
            // Black hole: pull screen radially toward center, darken core.
            disp -= dir * (k * k * 80.0 * s) / resolution;
            darken += smoothstep(0.55, 1.0, k) * 0.85;
        } else if (fxType[i] == 1) {
            // Time warp: outward sinusoidal ripple.
            float ripple = sin(dist * 0.05 - time * 7.0) * (k * 28.0 * s);
            disp += dir * ripple / resolution;
        } else if (fxType[i] == 2) {
            // Nuke: hard chromatic aberration ring.
            chrom += k * s;
        } else if (fxType[i] == 3) {
            // Vortex: tangential swirl.
            disp += tang * (k * 55.0 * s) / resolution;
        }
    }

    vec2 srcUv = clamp(uv + disp, vec2(0.0), vec2(1.0));
    vec3 col;

    if (chrom > 0.001) {
        float c = clamp(chrom, 0.0, 1.0) * 0.014;
        col.r = texture(texture0, srcUv + vec2(c, 0.0)).r;
        col.g = texture(texture0, srcUv).g;
        col.b = texture(texture0, srcUv - vec2(c, 0.0)).b;
    } else {
        col = texture(texture0, srcUv).rgb;
    }

    col *= 1.0 - clamp(darken, 0.0, 0.85);
    finalColor = vec4(col, 1.0);
}
