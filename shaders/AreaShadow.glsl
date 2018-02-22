-- Vertex

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 NormalV;
    vec4 PositionV;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.NormalV = mat3(view*model) * aNormal;
    vs_out.PositionV = view * model * vec4(aPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

-- Fragment

struct lightData
{
    float enabled;
    float type; // 0 = pointlight 1 = directionlight
    vec4 ambient;
    vec4 position; // where are we
    vec4 diffuse; // how diffuse
    vec4 specular; // what kinda specular stuff we got going on?
    // attenuation
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    // spot and area
    vec3 spotDirection;
    // only for area
    float width;
    float height;
    vec3 right;
    vec3 up;
};

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 NormalV;
    vec4 PositionV;
} fs_in;

const int numberOfLights = 1;
uniform Light
{
    lightData lights[numberOfLights];
};

uniform mat4 view;
uniform vec4 lightPos;
uniform vec4 mat_ambient;
uniform vec4 mat_diffuse;
uniform vec4 mat_specular;
uniform vec4 mat_emissive;
uniform float mat_shininess;

void pointLight( in lightData light, in vec3 normal, in vec3 ecPosition3, inout vec3 ambient, inout vec3 diffuse, inout vec3 specular )
{ 
    float nDotVP;       // normal . light direction
    float nDotHV;       // normal . light half vector
    float pf;           // power factor
    float attenuation;  // computed attenuation factor
    float d;            // distance from surface to light source
    vec3  VP;           // direction from surface to light position
    vec3  halfVector;   // direction of maximum highlights
    vec3  eye = -ecPosition3;

    // Compute vector from surface to light position
    VP = vec3(light.position.xyz) - ecPosition3;

    // Compute distance between surface and light position
    d = length(VP);

    // Compute attenuation
    attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * d + light.quadraticAttenuation * d * d);

    // Normalize the vector from surface to light position
    VP = normalize(VP);
    halfVector = normalize(VP + eye);
    nDotHV = max(0.0, dot(normal, halfVector));
    nDotVP = max(0.0, dot(normal, VP));

    ambient += light.ambient.rgb * attenuation;
    diffuse += light.diffuse.rgb * nDotVP * attenuation;

    // fresnel factor
    // http://en.wikibooks.org/wiki/GLSL_Programming/Unity/Specular_Highlights_at_Silhouettes
    float w = pow(1.0 - max(0.0, dot(halfVector, VP)), 5.0);
    vec3 specularReflection = attenuation * vec3(light.specular.rgb)
        * mix(vec3(mat_specular.rgb), vec3(1.0), w)
        * pow(nDotHV, mat_shininess);
    specular += mix(vec3(0.0), specularReflection, step(0.0000001, nDotVP));
}

void main()
{           
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    vec3 diffuse = vec3(0.0, 0.0, 0.0);
    vec3 specular = vec3(0.0, 0.0, 0.0);

    vec3 v_transformedNormal = fs_in.NormalV;
    vec3 v_eyePosition = vec3(fs_in.PositionV);

    for (int i = 0; i < numberOfLights; i++)
    {
        pointLight(lights[i], v_transformedNormal, v_eyePosition, ambient, diffuse, specular);
    }
    vec4 localColor = vec4(ambient, 1.0) * mat_ambient + vec4(diffuse, 1.0) * mat_diffuse + vec4(specular, 1.0) * mat_specular + mat_emissive;
    FragColor = localColor;
}