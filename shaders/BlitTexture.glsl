-- Vertex

// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec2 vTexcoords;

void main()
{
	vTexcoords = inTexcoords;
	gl_Position = vec4(inPosition, 1.0);
}

-- Fragment

// IN
in vec2 vTexcoords;
uniform sampler2D uTexSource;

// OUT
out vec3 fragColor;

// ----------------------------------------------------------------------------
void main() 
{
    fragColor = texture(uTexSource, vTexcoords).rgb;
}