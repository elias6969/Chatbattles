#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;

// Very lightweight bloom-ish blur + vignette.
void main()
{
    vec2 uv = fragTexCoord;
    vec2 px = 1.0 / max(resolution, vec2(1.0));

    vec3 col = texture(texture0, uv).rgb;

    // 5-tap blur for highlights
    vec3 b =
        texture(texture0, uv + vec2( 1.0, 0.0) * px).rgb +
        texture(texture0, uv + vec2(-1.0, 0.0) * px).rgb +
        texture(texture0, uv + vec2( 0.0, 1.0) * px).rgb +
        texture(texture0, uv + vec2( 0.0,-1.0) * px).rgb +
        col;
    b *= 0.2;

    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));
    float bloomMask = smoothstep(0.55, 0.90, luma);
    vec3 bloom = b * bloomMask * 0.55;

    // Vignette
    vec2 p = uv * 2.0 - 1.0;
    float vig = smoothstep(1.2, 0.2, dot(p, p));

    vec3 outCol = (col + bloom) * vig;
    finalColor = vec4(outCol, 1.0);
}

