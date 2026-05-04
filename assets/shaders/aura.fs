#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec4  auraColor; // rgb + base alpha
uniform float auraTime;
uniform float auraType;  // float so it works on every driver

const float PI2 = 6.28318530718;

void main() {
    // Sample (and ignore) the texture so raylib's default uniforms stay happy.
    vec4 dummy = texture(texture0, fragTexCoord) * colDiffuse;

    vec2 p = fragTexCoord - vec2(0.5);
    float r = length(p) * 2.0;       // 0 at center, ~1 at the rim, > 1 in corners
    if (r > 1.0) {                   // outside the inscribed circle = transparent
        finalColor = vec4(0.0);
        return;
    }

    float t  = auraTime;
    float ang = atan(p.y, p.x);
    int   k  = int(auraType + 0.5);

    // Base soft body glow shared by every ability.
    float a = smoothstep(1.0, 0.05, r) * 0.40;
    vec3 col = auraColor.rgb;

    if (k == 1) {
        // Frost Nova: crisp icy ring + crystalline rays.
        float ringPos = 0.55 + 0.06 * sin(t * 4.0);
        a += 0.06 / (abs(r - ringPos) + 0.02);
        a += pow(0.5 + 0.5 * sin(ang * 16.0 + t * 2.0), 8.0)
             * smoothstep(0.4, 0.7, r) * 0.6;
        col = mix(col, vec3(0.92, 0.97, 1.0), 0.4);
    } else if (k == 2) {
        // Inferno: animated radial flame tongues.
        float tongues = pow(0.5 + 0.5 * sin(ang * 9.0 + t * 6.0), 3.0);
        float flicker = pow(0.5 + 0.5 * sin(ang * 23.0 - t * 11.0), 5.0);
        a += (tongues * 1.1 + flicker * 0.6) * smoothstep(1.0, 0.1, r);
        col = mix(col, vec3(1.0, 0.7, 0.2), 0.45);
    } else if (k == 3) {
        // Vortex: spiral arms.
        float spiral = pow(0.5 + 0.5 * sin(4.0 * ang + t * 3.0 - r * 8.0), 3.0);
        a += spiral * smoothstep(1.0, 0.15, r) * 0.95;
    } else if (k == 4) {
        // Black hole: dark core + bright accretion ring.
        float core = smoothstep(0.20, 0.30, r);
        float ringMask = smoothstep(0.55, 0.30, r);
        float band = (0.5 + 0.5 * sin(ang * 9.0 + r * 14.0 + t * 5.0)) * ringMask;
        a = a * core + band * 1.4;
        col = mix(col, vec3(1.0, 0.85, 1.0), band);
    } else if (k == 5) {
        // Lightning: jagged forking arms.
        float arms = pow(0.5 + 0.5 * sin(ang * 7.0 + sin(t * 30.0) * 3.0), 9.0);
        a += arms * smoothstep(1.0, 0.0, r) * 1.6;
        col = mix(col, vec3(1.0, 1.0, 0.7), 0.5);
    } else if (k == 6) {
        // Shield Bubble: hex-ribbed bubble.
        float rib = 0.5 + 0.5 * sin(r * 24.0 - t * 4.0);
        a += rib * smoothstep(1.0, 0.55, r) * smoothstep(0.45, 0.7, r) * 1.0;
    } else if (k == 7) {
        // Heal Burst: pulsing ring + cross sparkle.
        float pulse = 0.5 + 0.5 * sin(t * 6.0);
        a += 0.05 / (abs(r - (0.55 + 0.04 * pulse)) + 0.015);
        float cross = max(smoothstep(0.05, 0.0, abs(p.x)),
                          smoothstep(0.05, 0.0, abs(p.y)));
        a += cross * smoothstep(0.45, 0.0, r) * 0.6;
    } else if (k == 8) {
        // Berserk: jagged radial shards.
        float shards = pow(0.5 + 0.5 * sin(ang * 18.0 + t * 12.0), 12.0);
        a += shards * smoothstep(1.0, 0.2, r) * 1.4;
    } else if (k == 9) {
        // Time warp: concentric clock rings.
        float band = 0.5 + 0.5 * sin(r * 18.0 - t * 2.5);
        a += band * smoothstep(1.0, 0.1, r) * 0.7;
    } else if (k == 10) {
        // Phoenix: wing-shaped warm halo (biased upward).
        vec2 q = p; q.y -= 0.08;
        float wing = pow(0.5 + 0.5 * cos(atan(q.y, q.x) * 2.0), 4.0);
        a += wing * smoothstep(1.0, 0.05, length(q) * 2.0) * 1.4;
        col = mix(col, vec3(1.0, 0.85, 0.4), 0.55);
    } else if (k == 11) {
        // Rocket: forward-biased thruster halo (uses dot product with +x axis).
        float forward = smoothstep(-0.4, 0.4, p.x);
        a += forward * smoothstep(1.0, 0.1, r) * 0.7;
        col = mix(col, vec3(0.7, 0.9, 1.0), 0.5);
    } else if (k == 12) {
        // Gravity well: outward concentric ripples.
        float band = 0.5 + 0.5 * sin(r * 14.0 - t * 2.0);
        a += band * smoothstep(1.0, 0.1, r) * 0.65;
    } else if (k == 13) {
        // Chainsaw: rotating saw teeth at the rim.
        float teeth = pow(0.5 + 0.5 * sin(ang * 24.0 + t * 6.0), 18.0);
        float rim = smoothstep(0.55, 0.6, r) - smoothstep(0.62, 0.66, r);
        a += teeth * rim * 1.4;
    }

    a = clamp(a, 0.0, 1.0);
    finalColor = vec4(col, auraColor.a * a) + dummy * 0.0;
}
