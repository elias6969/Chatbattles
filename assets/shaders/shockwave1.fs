#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec2 center;
uniform float shockRadius;

void main()
{
    vec2 uv = fragTexCoord;
    vec2 screenUV = uv * resolution;

    float dist = distance(screenUV, center);

    float thickness = 10.0;

    float ring = smoothstep(shockRadius - thickness, shockRadius, dist)
               - smoothstep(shockRadius, shockRadius + thickness, dist);

    vec2 dir = normalize(screenUV - center);

    // distortion
    vec2 distortedUV = uv + dir * ring * 0.02;

    vec3 base = vec3(0.02, 0.02, 0.05);

    vec3 glow = vec3(0.8, 0.3, 1.0) * ring * 3.0;

    finalColor = vec4(base + glow, 1.0);
}
