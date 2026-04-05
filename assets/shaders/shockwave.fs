#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec2 center;
uniform float shockRadius;

void main()
{
    vec2 uv = fragTexCoord * resolution;

    float dist = distance(uv, center);

    // 🔥 sharp ring
    float thickness = 8.0;
    float ring = smoothstep(shockRadius - thickness, shockRadius, dist)
               - smoothstep(shockRadius, shockRadius + thickness, dist);

    // 🔥 fade out over distance
    float fade = exp(-shockRadius * 0.01);

    vec3 base = vec3(0.02, 0.02, 0.05);

    // 🔥 strong color
    vec3 waveColor = vec3(0.8, 0.3, 1.0) * ring * fade * 2.0;

    finalColor = vec4(base + waveColor, 1.0);
}
