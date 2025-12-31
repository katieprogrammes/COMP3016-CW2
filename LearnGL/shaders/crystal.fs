#version 330 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_emissive1;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float time; 
uniform float sparkleStrength; // 0 for fake, 1 for real

void main()
{
    // 1. Base color
    vec3 base = texture(texture_diffuse1, TexCoords).rgb;

    // 2. Normal map
    vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0);

    // 3. Simple lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(normalMap, lightDir), 0.0);

    vec3 lighting = base * diff;

    // 4. Emissive glow
    vec3 emissive = texture(texture_emissive1, TexCoords).rgb;

    // 5. Sparkle
    float pulse = sin(time * 8.0 + FragPos.x * 3.0 + FragPos.z * 3.0) * 0.5 + 0.5; // Boost emissive only if sparkleStrength > 0 
    emissive *= 1.0 + sparkleStrength * pulse * 1.5;

    // 6. Final color
    FragColor = vec4(lighting + emissive, 1.0);
}
