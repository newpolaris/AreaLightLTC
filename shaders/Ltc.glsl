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

// bind roughness   {label:"Roughness", default:0.25, min:0.01, max:1, step:0.001}
// bind dcolor      {label:"Diffuse Color",  r:1.0, g:1.0, b:1.0}
// bind scolor      {label:"Specular Color", r:1.0, g:1.0, b:1.0}
// bind intensity   {label:"Light Intensity", default:4, min:0, max:10}
// bind width       {label:"Width",  default: 8, min:0.1, max:15, step:0.1}
// bind height      {label:"Height", default: 8, min:0.1, max:15, step:0.1}
// bind roty        {label:"Rotation Y", default: 0, min:0, max:1, step:0.001}
// bind rotz        {label:"Rotation Z", default: 0, min:0, max:1, step:0.001}
// bind twoSided    {label:"Two-sided", default:false}

out vec4 FragColor;

uniform float uRoughness;
uniform vec3 uDcolor;
uniform vec3 uScolor;
uniform float uIntensity;
uniform float uWidth;
uniform float uHeight;
uniform float uRotY;
uniform float uRotZ;
uniform bool ubTwoSided;
uniform bool ubClipless;
uniform bool ubTextured;

uniform sampler2D uLtc1;
uniform sampler2D uLtc2;

uniform sampler2DArray tex;
uniform mat4 uView;
uniform vec2 uResolution;
uniform int uSampleCount;

const float LUT_SIZE = 64.0;
const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
const float LUT_BIAS = 0.5/LUT_SIZE;

const float pi = 3.14159265;

// Tracing and intersection
///////////////////////////

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

vec2 RectUVs(vec3 pos, Rect rect)
{
    vec3 lpos = pos - rect.center;

    float x = dot(lpos, rect.dirx);
    float y = dot(lpos, rect.diry);

    return vec2(
        0.5*x/rect.halfx + 0.5,
        0.5*y/rect.halfy + 0.5);
}

mat3 transpose(mat3 v)
{
    mat3 tmp;
    tmp[0] = vec3(v[0].x, v[1].x, v[2].x);
    tmp[1] = vec3(v[0].y, v[1].y, v[2].y);
    tmp[2] = vec3(v[0].z, v[1].z, v[2].z);

    return tmp;
}

vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

vec3 rrt_odt_fit(vec3 v)
{
    vec3 a = v*(         v + 0.0245786) - 0.000090537;
    vec3 b = v*(0.983729*v + 0.4329510) + 0.238081;
    return a/b;
}

mat3 mat3_from_rows(vec3 c0, vec3 c1, vec3 c2)
{
    mat3 m = mat3(c0, c1, c2);
    m = transpose(m);

    return m;
}

vec3 mul(mat3 m, vec3 v)
{
    return m * v;
}

mat3 mul(mat3 m1, mat3 m2)
{
    return m1 * m2;
}

float saturate(float v)
{
    return clamp(v, 0.0, 1.0);
}

vec3 saturate(vec3 v)
{
    return clamp(v, 0.0, 1.0);
}

vec3 aces_fitted(vec3 color)
{
	mat3 ACES_INPUT_MAT = mat3_from_rows(
	    vec3( 0.59719, 0.35458, 0.04823),
	    vec3( 0.07600, 0.90834, 0.01566),
	    vec3( 0.02840, 0.13383, 0.83777));

	mat3 ACES_OUTPUT_MAT = mat3_from_rows(
	    vec3( 1.60475,-0.53108,-0.07367),
	    vec3(-0.10208, 1.10813,-0.00605),
	    vec3(-0.00327,-0.07276, 1.07602));

    color = mul(ACES_INPUT_MAT, color);

    // Apply RRT and ODT
    color = rrt_odt_fit(color);

    color = mul(ACES_OUTPUT_MAT, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

// Camera functions
///////////////////

Ray GenerateCameraRay()
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

vec3 rotation_y(vec3 v, float a)
{
    vec3 r;
    r.x =  v.x*cos(a) + v.z*sin(a);
    r.y =  v.y;
    r.z = -v.x*sin(a) + v.z*cos(a);
    return r;
}

vec3 rotation_z(vec3 v, float a)
{
    vec3 r;
    r.x =  v.x*cos(a) - v.y*sin(a);
    r.y =  v.x*sin(a) + v.y*cos(a);
    r.z =  v.z;
    return r;
}

vec3 rotation_yz(vec3 v, float ay, float az)
{
    return rotation_z(rotation_y(v, ay), az);
}

// Linearly Transformed Cosines
///////////////////////////////

vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;

    return cross(v1, v2)*theta_sintheta;
}

float IntegrateEdge(vec3 v1, vec3 v2)
{
    return IntegrateEdgeVec(v1, v2).z;
}

void ClipQuadToHorizon(inout vec3 L[5], out int n)
{
    // detect clipping config
    int config = 0;
    if (L[0].z > 0.0) config += 1;
    if (L[1].z > 0.0) config += 2;
    if (L[2].z > 0.0) config += 4;
    if (L[3].z > 0.0) config += 8;

    // clip
    n = 0;

    if (config == 0)
    {
        // clip all
    }
    else if (config == 1) // V1 clip V2 V3 V4
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2) // V2 clip V1 V3 V4
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3) // V1 V2 clip V3 V4
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4) // V3 clip V1 V2 V4
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5) // V1 V3 clip V2 V4) impossible
    {
        n = 0;
    }
    else if (config == 6) // V2 V3 clip V1 V4
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7) // V1 V2 V3 clip V4
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8) // V4 clip V1 V2 V3
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] =  L[3];
    }
    else if (config == 9) // V1 V4 clip V2 V3
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10) // V2 V4 clip V1 V3) impossible
    {
        n = 0;
    }
    else if (config == 11) // V1 V2 V4 clip V3
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12) // V3 V4 clip V1 V2
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13) // V1 V3 V4 clip V2
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14) // V2 V3 V4 clip V1
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15) // V1 V2 V3 V4
    {
        n = 4;
    }
    
    if (n == 3)
        L[3] = L[0];
    if (n == 4)
        L[4] = L[0];
}

vec3 FetchColorTexture(vec2 uv, float lod)
{
    if (!ubTextured)
        return vec3(1, 1, 1);
    uv.y = 1.0 - uv.y;
    return texture(tex, vec3(uv, lod)).rgb;
}

vec3 FetchDiffuseFilteredTexture(vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3 dir)
{
    if (ubTextured == false)
        return vec3(1, 1, 1);
    
    // area light plane basis
    vec3 V1 = p2 - p1;
    vec3 V2 = p4 - p1;
    vec3 planeOrtho = cross(V1, V2);
    float planeAreaSquared = dot(planeOrtho, planeOrtho);

    Ray ray;
    ray.origin = vec3(0, 0, 0);
    ray.dir = dir;
    vec4 plane = vec4(planeOrtho, -dot(planeOrtho, p1));
    float planeDist;
    RayPlaneIntersect(ray, plane, planeDist);
 
    vec3 P = planeDist*ray.dir - p1;
 
    // find tex coords of P
    float dot_V1_V2 = dot(V1, V2);
    float inv_dot_V1_V1 = 1.0 / dot(V1, V1);
    vec3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
    vec2 Puv;
    Puv.y = dot(V2_, P) / dot(V2_, V2_);
    Puv.x = dot(V1, P)*inv_dot_V1_V1 - dot_V1_V2*inv_dot_V1_V1*Puv.y;

    // LOD
    float d = abs(planeDist) / pow(planeAreaSquared, 0.25);
    
    // Flip texture to match OpenGL conventions
    Puv = Puv*vec2(1, -1) + vec2(0, 1);
    
    float lod = log(2048.0*d)/log(3.0);
    lod = min(lod, 7.0);
    
    float lodA = floor(lod);
    float lodB = ceil(lod);
    float t = lod - lodA;
    
    vec3 a = FetchColorTexture(Puv, lodA);
    vec3 b = FetchColorTexture(Puv, lodB);

    return mix(a, b, t);
}

vec3 LTC_Evaluate(
    vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N*dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = mul(Minv, transpose(mat3(T1, T2, N)));
    
    mat3 MM = mat3(1);

    // polygon (allocate 5 vertices for clipping)
    vec3 L[5];
    L[0] = mul(Minv, points[0] - P);
    L[1] = mul(Minv, points[1] - P);
    L[2] = mul(Minv, points[2] - P);
    L[3] = mul(Minv, points[3] - P);
    
    vec3 LL[4];
    LL[0] = L[0];
    LL[1] = L[1];
    LL[2] = L[2];
    LL[3] = L[3];

    // integrate
    float sum = 0.0;
    vec3 colorMap;

    if (ubClipless)
    {
        vec3 dir = points[0].xyz - P;
        vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
        bool behind = (dot(dir, lightNormal) < 0.0);

        L[0] = normalize(L[0]);
        L[1] = normalize(L[1]);
        L[2] = normalize(L[2]);
        L[3] = normalize(L[3]);

        vec3 vsum = vec3(0.0);

        vsum += IntegrateEdgeVec(L[0], L[1]);
        vsum += IntegrateEdgeVec(L[1], L[2]);
        vsum += IntegrateEdgeVec(L[2], L[3]);
        vsum += IntegrateEdgeVec(L[3], L[0]);

        float len = length(vsum);
        float z = vsum.z/len;

        if (behind)
            z = -z;

        vec2 uv = vec2(z*0.5 + 0.5, len);
        uv.y = 1 - uv.y;
        uv = uv*LUT_SCALE + LUT_BIAS;

        float scale = texture(uLtc2, uv).w;

        sum = len*scale;

        if (behind && !twoSided)
            sum = 0.0;

        vec3 fetchDir = vsum/len;
        colorMap = FetchDiffuseFilteredTexture(LL[0], LL[1], LL[2], LL[3], fetchDir);
    }
    else
    {
        int n;
        ClipQuadToHorizon(L, n);

    if (n == 0)
        return vec3(0, 0, 0);

    // project onto sphere
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);
    L[4] = normalize(L[4]);
        
        vec3 vsum;

        // integrate
        vsum  = IntegrateEdgeVec(L[0], L[1]);
        vsum += IntegrateEdgeVec(L[1], L[2]);
        vsum += IntegrateEdgeVec(L[2], L[3]);
        if (n >= 4)
            vsum += IntegrateEdgeVec(L[3], L[4]);
        if (n == 5)
            vsum += IntegrateEdgeVec(L[4], L[0]);

        sum = twoSided ? abs(vsum.z) : max(0.0, vsum.z);

        vec3 fetchDir = normalize(vsum);
        colorMap = FetchDiffuseFilteredTexture(LL[0], LL[1], LL[2], LL[3], fetchDir);
    }

    vec3 Lo_i = sum * colorMap;
    return Lo_i;
}

// Scene helpers
////////////////

void InitRect(out Rect rect)
{
    rect.dirx = rotation_yz(vec3(1, 0, 0), uRotY*2.0*pi, uRotZ*2.0*pi);
    rect.diry = rotation_yz(vec3(0, 1, 0), uRotY*2.0*pi, uRotZ*2.0*pi);
	rect.center = vec3(0, 6, 32);
	rect.halfx = 0.5*uWidth;
	rect.halfy = 0.5*uHeight;

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

// Misc. helpers
////////////////

const float gamma = 2.2;

vec3 ToLinear(vec3 v) { return PowVec3(v,     gamma); }
vec3 ToSRGB(vec3 v)   { return PowVec3(v, 1.0/gamma); }

void main()
{
	Rect rect;
	InitRect(rect);

	vec3 points[4];
	InitRectPoints(rect, points);

	vec4 floorPlane = vec4(0, 1, 0, 0);

	vec3 lcol = vec3(uIntensity);
    vec3 dcol = ToLinear(uDcolor);
    vec3 scol = ToLinear(uScolor);
	vec3 col = vec3(0);

    Ray ray = GenerateCameraRay();

    float distToFloor;
    bool hitFloor = RayPlaneIntersect(ray, floorPlane, distToFloor);
    if (hitFloor)
    {
        vec3 pos = ray.origin + ray.dir*distToFloor;

        vec3 N = floorPlane.xyz;
        vec3 V = -ray.dir;

        float ndotv = saturate(dot(N, V));
        vec2 uv = vec2(uRoughness, sqrt(1.0 - ndotv));
        uv.y = 1.0 - uv.y;
        uv = uv*LUT_SCALE + LUT_BIAS;

        vec4 t1 = texture(uLtc1, uv);
        vec4 t2 = texture(uLtc2, uv);

        mat3 Minv = mat3(
            vec3(t1.x, 0, t1.y),
            vec3(  0,  1,    0),
            vec3(t1.z, 0, t1.w)
        );

        vec3 spec = LTC_Evaluate(N, V, pos, Minv, points, ubTwoSided);
        // BRDF shadowing and Fresnel
        spec *= scol*t2.x + (1.0 - scol)*t2.y;

        vec3 diff = LTC_Evaluate(N, V, pos, mat3(1), points, ubTwoSided);

        col = lcol*(spec + dcol*diff);
    }

    float distToRect;
    if (RayRectIntersect(ray, rect, distToRect))
    {
        if ((distToRect < distToFloor) || !hitFloor)
        {
            vec3 pos = ray.origin + ray.dir*distToRect;
            vec2 uv  = RectUVs(pos, rect);
            uv = uv*vec2(1, -1) + vec2(0, 1);
            col = lcol*texture(tex, vec3(uv, 0.0)).rgb;
            if (!ubTextured)
                col = lcol;
        }
    }


	col = aces_fitted(col);
	col = ToSRGB(col);

	FragColor = vec4(col, 1.0);
}