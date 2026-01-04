#version 330 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 ClipPos;


out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_emissive1;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float time; 
uniform float sparkleStrength; // 0 for fake, >0 when torch hits real crystal
uniform bool isRealCrystal;

void main()
{
    vec2 screenUV = (ClipPos.xy / ClipPos.w) * 0.5 + 0.5;
    vec3 shellPos = FragPos + Normal * 0.5; // 0.25 = shell thickness

    // 1. Base color
    vec3 base = texture(texture_diffuse1, TexCoords).rgb;

    // 2. Normal map (you are using vertex normals)
    vec3 normalMap = normalize(Normal);

    // 3. Simple lighting (darkened)
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(normalMap, lightDir), 0.0);
    vec3 lighting = base * diff * 0.6;

    // 4. Emissive glow from texture
    vec3 emissive = texture(texture_emissive1, TexCoords).rgb * 0.75;

    // 5. Glint effect
    vec3 viewDir = normalize(viewPos - FragPos); 
    float spec = pow(max(dot(normalMap, viewDir), 0.0), 64.0); 
    float flicker = sin(time * 10.0 + FragPos.x * 2.0 + FragPos.z * 2.0) * 0.5 + 0.5; 
    float glint = spec * flicker * sparkleStrength; 
    emissive += glint * 0.8;

    //6. Pulsing
    float pulse = (sin(time * 3.0) * 0.5 + 0.5); 
    pulse = pow(pulse, 2.0); 
    pulse *= sparkleStrength; 
    emissive += pulse * 0.3;

    // 7. Halo glow
    float halo = 0.0; 
    if (isRealCrystal && sparkleStrength > 0.0)
    { 
        float distToCamera = length(viewPos - FragPos); 
        halo = 0.4 / (distToCamera * distToCamera); 
    } 
    vec3 haloColor = vec3(0.4, 0.7, 1.2);
    emissive += halo * haloColor;

    // 7.5 Colour shift when torch hits a real crystal 
    vec3 hitColor = vec3(0.2, 0.6, 1.5); // bright blue-white 
    emissive += hitColor * sparkleStrength * 0.8;

    // Overall bloom boost 
    float bloom = sparkleStrength * 0.5; 
    emissive *= (1.0 + bloom);

    // 8. Final color
    FragColor = vec4(lighting + emissive, 1.0);
}
