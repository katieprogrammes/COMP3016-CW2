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
    vec3 normalMap = normalize(Normal);

    // 3. Simple lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(normalMap, lightDir), 0.0);

    vec3 lighting = base * diff;

    // 4. Emissive glow
    vec3 emissive = texture(texture_emissive1, TexCoords).rgb;

    //5. Sparkle effect

    // View direction 
    vec3 viewDir = normalize(viewPos - FragPos); 
    // Specular highlight (sharp)
    float spec = pow(max(dot(normalMap, viewDir), 0.0), 64.0); 
    // Animate the glint so it flickers 
    float flicker = sin(time * 10.0 + FragPos.x * 2.0 + FragPos.z * 2.0) * 0.5 + 0.5; 
    // Final glint intensity 
    float glint = spec * flicker * sparkleStrength;
    // Add glint to emissive 
    emissive += glint * 0.8;

    // 6. Final color
    FragColor = vec4(lighting + emissive, 1.0);
}
