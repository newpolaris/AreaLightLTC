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

out vec4 FragColor;

uniform float uIntensity;
uniform float uWidth;
uniform float uHeight;

uniform mat4 uView;
uniform vec2 uResolution;

const float pi = 3.14159265;

struct Ray
{
	vec3 origin;
	vec3 dir;
};

struct Rect
{
	vec3 center;
	vec3 dirx;
	vec3 diry;

	float halfx;
	float halfy;

	vec4 plane;
};

Ray GenerateCameraRay(float u1, float u2)
{
	Ray ray;

	// Random jitter within pixel for AA
	vec2 xy = 2.0*(gl_FragCoord.xy)/uResolution - vec2(1.0);

	ray.dir = normalize(vec3(xy, 2.0));

	float focalDistance = 2.0;
	float ft = focalDistance/ray.dir.z;
	vec3 pFocus = ray.dir*ft;

	ray.origin = vec3(0);
	ray.dir = normalize(pFocus - ray.origin);

	// Apply camera transform
	ray.origin = (uView*vec4(ray.origin, 1)).xyz;
	ray.dir = (uView*vec4(ray.dir, 0)).xyz;

	return ray;
}

bool RayPlaneIntersect(Ray ray, vec4 plane, out float t)
{
	t = -dot(plane, vec4(ray.origin, 1.0))/dot(plane.xyz, ray.dir);
	return t > 0.0;
}

bool RayRectIntersect(Ray ray, Rect rect, out float t)
{
	bool intersect = RayPlaneIntersect(ray, rect.plane, t);
	if (intersect)
	{
		vec3 pos = ray.origin + ray.dir*t;
		vec3 lpos = pos - rect.center;

		float x = dot(lpos, rect.dirx);
		float y = dot(lpos, rect.diry);

		if (abs(x) > rect.halfx || abs(y) > rect.halfy)
			intersect = false;
	}
	return intersect;
}

void InitRect(out Rect rect)
{
	float width = 8.0;
	float height = 8.0;
	rect.dirx = vec3(1, 0, 0);
	rect.diry = vec3(0, 1, 0);
	rect.center = vec3(0, 6, 32);
	rect.halfx = 0.5*width;
	rect.halfy = 0.5*height;

	vec3 rectNormal = cross(rect.dirx, rect.diry);
	rect.plane = vec4(rectNormal, -dot(rectNormal, rect.center));
}

void InitRectPoints(Rect rect, out vec3 points[4])
{
	vec3 ex = rect.halfx*rect.dirx;
	vec3 ey = rect.halfy*rect.diry;

	points[0] = rect.center - ex - ey;
	points[1] = rect.center + ex - ey;
	points[2] = rect.center + ex + ey;
	points[3] = rect.center - ex + ey;
}

void main()
{
	Rect rect;
	InitRect(rect);

	vec3 points[4];
	InitRectPoints(rect, points);

	vec4 floorPane = vec4(0, 1, 0, 0);

	vec3 lcol = vec3(uIntensity);
	vec3 col = vec3(0);

	Ray ray = GenerateCameraRay(0.0, 0.0);

	float distToFloor = 1.0;
	float distToRect;
	bool hitFloor = false;
	if (RayRectIntersect(ray, rect, distToRect))
		if ((distToRect < distToFloor) || !hitFloor)
			col = lcol;

	FragColor = vec4(col, 1.0);
}