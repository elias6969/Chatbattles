#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float softEdge; // 0..0.2 recommended

void main()
{
    vec2 p = fragTexCoord - vec2(0.5);
    float d = length(p);
    float r = 0.5;
    float a = 1.0 - smoothstep(r - softEdge, r, d);
    vec4 tex = texture(texture0, fragTexCoord) * colDiffuse;
    finalColor = vec4(tex.rgb, tex.a * a);
}

