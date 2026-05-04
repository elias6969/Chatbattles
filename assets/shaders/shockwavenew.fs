#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 center;
uniform float time;

void main() {
    vec2 uv = fragTexCoord;

    float dist = distance(uv, center);

    float wave = sin(dist * 40.0 - time * 10.0);
    float strength = 0.02 / (dist * 20.0 + 1.0);

    uv += normalize(uv - center) * wave * strength;

    finalColor = texture(texture0, uv);
}
