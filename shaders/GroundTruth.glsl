-- Vertex
// IN
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexcoords;

// Out
out vec4 vPositionW;
out vec3 vNormalW;

uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    mat4 worldViewProj = uProjection*uView*uWorld;

    vPositionW = uWorld * vec4(inPosition, 1.0);
    vNormalW = mat3(uWorld) * inNormal;
	gl_Position = worldViewProj * vec4(inPosition, 1.0);
}

-- Fragment

// bind roughness   {label:"Roughness", default:0.25, min:0.01, max:1, step:0.001}
// bind dcolor      {label:"Diffuse Color",  r:1.0, g:1.0, b:1.0}
// bind intensity   {label:"Light Intensity", default:4, min:0, max:10}
// bind width       {label:"Width",  default: 8, min:0.1, max:15, step:0.1}
// bind height      {label:"Height", default: 8, min:0.1, max:15, step:0.1}
// bind roty        {label:"Rotation Y", default: 0, min:0, max:1, step:0.001}
// bind rotz        {label:"Rotation Z", default: 0, min:0, max:1, step:0.001}
// bind twoSided    {label:"Two-sided", default:false}

// IN
in vec4 vPositionW;
in vec3 vNormalW;

// OUT
out vec3 FragColor;

uniform vec4 uQuadPoints[4]; // Area light quad
uniform vec4 uStarPoints[10]; // Area light star
uniform vec3 uViewPositionW;

uniform float uF0; // frenel
uniform float uRoughness;
uniform vec3 uAlbedo; // from material
uniform vec4 uAlbedo2; // additional albedo
uniform float uIntensity;
uniform float uWidth;
uniform float uHeight;
uniform float uRotY;
uniform float uRotZ;
uniform bool uTwoSided;
uniform bool uTexturedLight;

uniform sampler2D uLtcMat;
uniform sampler2D uLtcMag;
uniform sampler2D uFilteredMap;

uniform mat4 uView;
uniform vec2 uResolution;
uniform int uSampleCount;

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

// Camera functions
///////////////////

Ray GenerateCameraRay(vec3 position, vec3 target)
{
	Ray ray;

	// Random jitter within pixel for AA
	ray.origin = position;
	ray.dir = normalize(target - position);

	return ray;
}

// Use code in 'LTC demo sample'
vec3 FetchDiffuseFilteredTexture(sampler2D texLightFiltered, vec3 p1_, vec3 p2_, vec3 p3_, vec3 p4_)
{
    // area light plane basis
    vec3 V1 = p2_ - p1_;
    vec3 V2 = p4_ - p1_;
    vec3 planeOrtho = (cross(V1, V2));
    float planeAreaSquared = dot(planeOrtho, planeOrtho);
    float planeDistxPlaneArea = dot(planeOrtho, p1_);
    // orthonormal projection of (0,0,0) in area light space
    vec3 P = planeDistxPlaneArea * planeOrtho / planeAreaSquared - p1_;

    // find tex coords of P
    float dot_V1_V2 = dot(V1,V2);
    float inv_dot_V1_V1 = 1.0 / dot(V1, V1);
    vec3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
    vec2 Puv;
    Puv.y = dot(V2_, P) / dot(V2_, V2_);
    Puv.x = dot(V1, P)*inv_dot_V1_V1 - dot_V1_V2*inv_dot_V1_V1*Puv.y ;

    // invert UV
    Puv.y = 1 - Puv.y;

    // LOD
    float d = abs(planeDistxPlaneArea) / pow(planeAreaSquared, 0.75);

    // 0.125, 0.75 looks like border gap and content length
    // 2048.0 is may be image size
    return textureLod(texLightFiltered, vec2(0.125, 0.125) + 0.75 * Puv, log(2048.0*d)/log(3.0) ).rgb;
}

vec3 mul(mat3 m, vec3 v)
{
    return m * v;
}

mat3 mul(mat3 m1, mat3 m2)
{
    return m1 * m2;
}

// Scene helpers
////////////////

void InitRect(out Rect rect, in vec4 points[4])
{
    vec3 right = vec3(points[3] - points[0]);
    vec3 up = vec3(points[0] - points[1]);
    rect.dirx = normalize(right);
    rect.diry = normalize(up);
	rect.center = vec3((points[0] + points[2]))*0.5;
	rect.halfx = 0.5*length(right);
	rect.halfy = 0.5*length(up);

	vec3 rectNormal = cross(rect.dirx, rect.diry);
	rect.plane = vec4(rectNormal, -dot(rectNormal, rect.center));
}

vec3 toLinear(vec3 _rgb)
{
	return pow(abs(_rgb), vec3(2.2));
}

// Building an Orthonormal Basis from a 3D Unit Vector Without Normalization
mat3 BasisFrisvad(vec3 n)
{
    vec3 b1, b2;
    if (n.z < -0.999999) // Handle the sigularity
    {
        b1 = vec3(0.0, -1.0, 0.0);
        b2 = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
        float a = 1.0 / (1.0 + n.z);
        float b = -n.x*n.y*a;
        b1 = vec3(1.0 - n.x*n.x*a, b, -n.x);
        b2 = vec3(b, 1.0 - n.y*n.y*a, -n.y);
    }
    return mat3(b1, b2, n);
}

struct SphQuad 
{
    vec3 o, x, y, z; // local reference system 'R'
    float z0, z0sq; 
    float x0, y0, y0sq; // rectangle coords in 'R' 
    float x1, y1, y1sq; 
    float b0, b1, b0sq, k; // misc precomputed constants 
    float S; // solid angle of 'Q' 
};

SphQuad SphQuadInit(vec3 s, vec3 ex, vec3 ey, vec3 o) 
{
    const float pi = 3.14159265;

    SphQuad squad;

    squad.o = o;
    float exl = length(ex), eyl = length(ey); // compute local reference system 'R' 
    squad.x = ex / exl;
    squad.y = ey / eyl;
    squad.z = cross(squad.x, squad.y); // compute rectangle coords in local reference system 
    vec3 d = s - o;
    squad.z0 = dot(d, squad.z); // flip 'z' to make it point against 'Q' 
    if (squad.z0 > 0) 
    {
        squad.z *= -1; 
        squad.z0 *= -1; 
    } 
    squad.z0sq = squad.z0 * squad.z0;
    squad.x0 = dot(d, squad.x);
    squad.y0 = dot(d, squad.y);
    squad.x1 = squad.x0 + exl;
    squad.y1 = squad.y0 + eyl;
    squad.y0sq = squad.y0 * squad.y0;
    squad.y1sq = squad.y1 * squad.y1; // create vectors to four vertices 
    vec3 v00 = vec3(squad.x0, squad.y0, squad.z0);
    vec3 v01 = vec3(squad.x0, squad.y1, squad.z0);
    vec3 v10 = vec3(squad.x1, squad.y0, squad.z0);
    vec3 v11 = vec3(squad.x1, squad.y1, squad.z0); // compute normals to edges 
    vec3 n0 = normalize(cross(v00, v10));
    vec3 n1 = normalize(cross(v10, v11));
    vec3 n2 = normalize(cross(v11, v01));
    vec3 n3 = normalize(cross(v01, v00)); // compute internal angles (gamma_i) 
    float g0 = acos(-dot(n0,n1));
    float g1 = acos(-dot(n1,n2));
    float g2 = acos(-dot(n2,n3));
    float g3 = acos(-dot(n3,n0)); // compute predefined constants 
    squad.b0 = n0.z;
    squad.b1 = n2.z;
    squad.b0sq = squad.b0 * squad.b0;
    squad.k = 2*pi - g2 - g3; // compute solid angle from internal angles 
    squad.S = g0 + g1 - squad.k; 
    return squad;
}

vec3 SphQuadSample(SphQuad squad, float u, float v) 
{
    // 1. compute 'cu' 
    float au = u * squad.S + squad.k;
    float fu = (cos(au) * squad.b0 - squad.b1) / sin(au);
    float cu = 1/sqrt(fu*fu + squad.b0sq) * (fu>0 ? +1 : -1);
    cu = clamp(cu, -1, 1); // avoid NaNs 
    // 2. compute 'xu' 
    float xu = -(cu * squad.z0) / sqrt(1 - cu*cu);
    xu = clamp(xu, squad.x0, squad.x1); // avoid Infs 
    // 3. compute 'yv' 
    float d = sqrt(xu*xu + squad.z0sq);
    float h0 = squad.y0 / sqrt(d*d + squad.y0sq);
    float h1 = squad.y1 / sqrt(d*d + squad.y1sq);
    float hv = h0 + v * (h1-h0), hv2 = hv*hv;
    float yv = (hv2 < 1 - 1e-6) ? (hv*d)/sqrt(1-hv2) : squad.y1;
    // 4. transform (xu,yv,z0) to world coords 
    return (squad.o + xu*squad.x + yv*squad.y + squad.z0*squad.z); 
}

// From: https://briansharpe.wordpress.com/2011/11/15/a-fast-and-simple-32bit-floating-point-hash-function/
vec4 FAST_32_hash(vec2 gridcell)
{
    // gridcell is assumed to be an integer coordinate
    const vec2 OFFSET = vec2(26.0, 161.0);
    const float DOMAIN = 71.0;
    const float SOMELARGEFLOAT = 951.135664;
    vec4 P = vec4(gridcell.xy, gridcell.xy + vec2(1, 1));
    P = P - floor(P * (1.0 / DOMAIN)) * DOMAIN;    //    truncate the domain
    P += OFFSET.xyxy;                              //    offset to interesting part of the noise
    P *= P;                                        //    calculate and return the hash
    return fract(P.xzxz * P.yyww * (1.0 / SOMELARGEFLOAT));
}

void main()
{
    const float pi = 3.14159265;
    const float minRoughness = 0.03;
    float metallic = 0.f;
    float roughness = max(uRoughness*uRoughness, minRoughness);
    vec3 normal = normalize(vec3(vNormalW));
	vec3 lcol = vec3(uIntensity);
    vec3 albedo = toLinear(vec3(uAlbedo2));
    vec3 baseColor = toLinear(uAlbedo);
    vec3 dcol = baseColor*(1.0 - metallic);
    vec3 scol = mix(vec3(uF0), baseColor, metallic);

    float alpha = 0.001;

    mat3 t2w = BasisFrisvad(normal);
    mat3 w2t = transpose(t2w);

	Ray ray = GenerateCameraRay(uViewPositionW, vPositionW.xyz);

    // express receiver dir in tangent space
    vec3 o = mul(w2t, ray.dir);

    vec3 ex = uQuadPoints[1].xyz - uQuadPoints[0].xyz;
    vec3 ey = uQuadPoints[3].xyz - uQuadPoints[0].xyz;
    vec2 uvScale = vec2(length(ex), length(ey));
    SphQuad squad = SphQuadInit(uQuadPoints[0].xyz, ex, ey, ray.origin);

    float rcpSolidAngle = 1.0/squad.S;

    vec3 quadn = normalize(cross(ex, ey));
    quadn = mul(w2t, quadn);

    vec3 col = vec3(1, 0, 0);

	FragColor = col;
}