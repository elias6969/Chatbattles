#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform float time;

void main()
{
    float glow = sin(time + fragTexCoord.x * 10.0) * 0.5 + 0.5;
    finalColor = vec4(glow, 0.2, 1.0, 1.0);
}
