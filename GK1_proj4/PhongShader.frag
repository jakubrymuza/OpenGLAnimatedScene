#version 330 core

// lights types
#define DIRECTIONAL_LIGHT_TYPE 0
#define POINT_LIGHT_TYPE 1
#define SPOTLIGHT_TYPE 2

// specular component
#define BLINN_SPECULAR 0
#define PHONG_SPECULAR 1

struct Light {
    int type;
    vec3 color;
    vec3 position;   
    vec3 direction;
    float attentuation;
    float innerAngle;
    float outerAngle;
};

#define MAX_LIGHTS_COUNT 6

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 cameraPosition;
uniform int lights_count;
uniform Light lights[MAX_LIGHTS_COUNT];

uniform int specular_type;

uniform float fog_maxdist;
uniform float fog_mindist;

uniform bool use_color;
uniform vec3 color;

uniform float Ka;
uniform float Kd;
uniform float Ks;
uniform float m;

vec3 calculateLight(Light light, vec3 L);
float calculate_fog(vec3 V, vec3 FragPos);

void main()
{    
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPosition - FragPos);

    vec3 finalColor = vec3(0.0, 0.0, 0.0);
    
    // DEBUG
    vec3 Io = use_color ? color : vec3(0.1, 0.1, 0.1);

    vec3 Iamb = Ka * vec3(1.0);

    for (int i = 0; i < lights_count; i++)
    {
        vec3 L;
        if(lights[i].type == DIRECTIONAL_LIGHT_TYPE)
            L = normalize(lights[i].direction); 
        else 
            L = normalize(lights[i].position - FragPos);   
                  

        vec3 Il = calculateLight(lights[i], L);

        
        vec3 Idiff = Kd * Il * Io * max(dot(N,L), 0.0);
        Idiff = clamp(Idiff, 0.0, 1.0);


        vec3 Ispec; 
        if(specular_type == PHONG_SPECULAR)
        {
            vec3 R = normalize(reflect(-L,N));
            Ispec = Ks * Il * Io * pow(max(dot(R,V),0.0), m);
        }
        else 
        {
            vec3 LV = L + V;
            vec3 H = LV/length(LV);
            Ispec = Ks * Il * Io * pow(max(dot(N,H),0.0), m);
        }

        Ispec = clamp(Ispec, 0.0, 1.0);
        finalColor += Idiff + Ispec;
    }
    vec4 finalColor4 = clamp(vec4(finalColor + Iamb * Io, 1.0), 0.0, 1.0);


    float fog_factor = calculate_fog(V, FragPos);
    vec4 fog_color = vec4(0.0, 0.0, 0.0, 0.0);

    gl_FragColor = mix(fog_color, finalColor4, fog_factor);
}

float calculate_fog(vec3 V, vec3 FragPos)
{   
    float dist = length(V - FragPos);
    float fog_factor = (fog_maxdist - dist) / (fog_maxdist - fog_mindist);
    fog_factor = clamp(fog_factor, 0.0, 1.0);
    return fog_factor;
}

vec3 calculateLight(Light light, vec3 L)
{
    float dist = length(light.position - FragPos);
    float distSq = dist * dist;
    float att;

    switch(light.type){
        case DIRECTIONAL_LIGHT_TYPE:
            return light.color;

        case POINT_LIGHT_TYPE:
            att = 1.0 / (light.attentuation * distSq);
            return light.color * att;

        case SPOTLIGHT_TYPE:
            float theta = dot(L, normalize(light.direction)); 
            float epsilon = (light.innerAngle - light.outerAngle);
            return clamp((theta - light.outerAngle) / epsilon, 0.0, 1.0) * light.color;
            
    }

    return vec3(0.0, 0.0, 0.0);
}

