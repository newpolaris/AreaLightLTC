--Vertex

// IN
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// OUT
out vec2 vTexcoords;

uniform mat4 uModelViewProj;

void main()
{
    vTexcoords = aTexCoords;
    gl_Position = uModelViewProj* vec4(aPosition, 1.0);
}

--Fragment

// IN
in vec2 vTexcoords;

// OUT
out vec4 FragColor;

uniform bool ubTexturedLight;
uniform vec4 uDiffuseColor;
uniform float uIntensity;
uniform sampler2D uTexColor;

vec3 toLinear(vec3 _rgb)
{
	return pow(abs(_rgb), vec3(2.2));
}
  
void main()
{
    vec3 color = uDiffuseColor.rgb;
    if (ubTexturedLight)
    	color = texture(uTexColor, vTexcoords).rgb;
    color = toLinear(color);
    color *= uIntensity;
    FragColor = vec4(color, 1.0);
}