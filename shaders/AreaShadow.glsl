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
    float a, b;
    vec4 ambient;
    vec4 position; // where are we
    vec4 diffuse; // how diffuse
    vec4 specular; // what kinda specular stuff we got going on?
    // attenuation
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float c;
    // spot and area
    vec3 spotDirection;
    float d;
    // only for area
    float width;
    float height;
    float e,f;
    vec3 right;
    float g;
    vec3 up;
    float h;
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
    lightData lights;
};

uniform mat4 view;
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

vec3 projection(in vec3 p, in vec3 p0, in vec3 normal)
{
    return dot(p - p0, normal) * normal;
}

vec3 projectOnPlane(in vec3 pos, in vec3 planeCenter, in vec3 planeNormal)
{
    return pos - projection(pos, planeCenter, planeNormal);
}

vec3 linePlaneIntersect(in vec3 lp, in vec3 lv, in vec3 pc, in vec3 pn)
{
    return lp + lv * dot(pn, pc - lp)/dot(pn, lv);
}

void areaLight( in lightData light, in vec3 normal, in vec3 ecPosition3, inout vec3 ambient, inout vec3 diffuse, inout vec3 specular )
{
    vec3 lightPos = light.position.xyz;
    vec3 right = light.right;
    // plane normal, front direction
    vec3 planeNormal = light.spotDirection;
    vec3 up = light.up;

    float width = light.width * 0.5;
    float height = light.height * 0.5;

    // project onto plane and calculate direction from center to the projection.
    vec3 projection = projectOnPlane(ecPosition3, lightPos, planeNormal);
    vec3 dir = projection - lightPos;

    // calculate distance from area:
    vec2 diagonal = vec2(dot(dir, right), dot(dir, up));
    vec2 nearest2D = vec2(clamp(diagonal.x, -width, width), clamp( diagonal.y, -height, height));
    vec3 nearestPointInside = lightPos + right*nearest2D.x + up*nearest2D.y;
    float dist = distance(ecPosition3, nearestPointInside); // real distance to area rectangle

    vec3 lightDir = normalize(nearestPointInside - ecPosition3);
    float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist);

    float ndotl = max(dot(planeNormal, -lightDir), 0.0);
    float ndotl2 = max(dot(normal, lightDir), 0.0);
    if (ndotl * ndotl2 > 0.0)
    {
        float diffuseFactor = sqrt(ndotl * ndotl2);
        vec3 r = reflect(normalize(-ecPosition3), normal);
        vec3 e = linePlaneIntersect(ecPosition3, r, lightPos, planeNormal);
        float specAngle = dot(r, planeNormal);
        if (specAngle > 0.0)
        {
            vec3 dirSpec = e - lightPos;
            vec2 dirSpec2D = vec2(dot(dirSpec, right), dot(dirSpec, up));
            vec2 nearestSpec2D = vec2(clamp(dirSpec2D.x, -width, width), clamp(dirSpec2D.y, -height, height));
            float specFactor = 1.0 - clamp(length(nearestSpec2D - dirSpec2D) * 0.5 * mat_shininess, 0.0, 1.0);
            specular += light.specular.rgb * specFactor * specAngle * diffuseFactor * attenuation;
        }
        diffuse += light.diffuse.rgb * diffuseFactor * attenuation;
    }
    ambient += light.ambient.rgb * attenuation;
}

void main()
{           
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    vec3 diffuse = vec3(0.0, 0.0, 0.0);
    vec3 specular = vec3(0.0, 0.0, 0.0);

    vec3 v_transformedNormal = fs_in.NormalV;
    vec3 v_eyePosition = vec3(fs_in.PositionV);

    if (lights.type < 0.5)
        pointLight(lights, v_transformedNormal, v_eyePosition, ambient, diffuse, specular);
    else if (lights.type < 1.5)
        areaLight(lights, v_transformedNormal, v_eyePosition, ambient, diffuse, specular);

    vec4 localColor = vec4(ambient, 1.0) * mat_ambient + vec4(diffuse, 1.0) * mat_diffuse + vec4(specular, 1.0) * mat_specular + mat_emissive;
    FragColor = localColor;
}