
#version 330 core
out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;    
    float shininess;
}; 

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec4 FragPosLightSpace;

uniform vec3 viewPos;
uniform sampler2D shadowMap;
uniform Material material;
uniform Light light;

vec3 lightDir = normalize(light.direction);

float calculateShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    float bias = 0.006;
    float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for (int x = -1; x <= 1; x++) {
    	for (int y = -1; y <= 1; y++) {
        	float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
        	shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
    	}    
	}
	shadow /= 9.0;
    if(projCoords.z > 1.0)
        shadow = 0.0;
    return shadow;
}

void main()
{
    // ambient
    vec3 ambient = light.ambient * material.ambient;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);  

    float shadow = calculateShadow(FragPosLightSpace);
        
    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    FragColor = vec4(result, 1.0);
} 