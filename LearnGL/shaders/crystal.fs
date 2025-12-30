#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform vec3 tint;

void main()
{
    vec3 base = texture(texture_diffuse1, TexCoords).rgb;
    FragColor = vec4(base * tint, 1.0);
}
