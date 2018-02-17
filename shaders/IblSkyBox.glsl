/*
 *          IblSkyBox.glsl
 *
 */

//------------------------------------------------------------------------------


-- Vertex

// IN
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

// OUT
out vec3 vDirection;

// UNIFORM
uniform mat4 uViewMatrix;
uniform mat4 uProjMatrix;

void main()
{
  mat4 rotView = mat4(mat3(uViewMatrix));
  vec4 clipPos = uProjMatrix * rotView * inPosition;
  gl_Position = clipPos.xyww;

  vDirection = inPosition.xyz;
}


--

//------------------------------------------------------------------------------


-- Fragment

#include "ToneMappingUtility.glsli"

// IN
in vec3 vDirection;

// OUT
layout(location = 0) out vec4 fragColor;

// UNIFORM
uniform samplerCube uEnvmap;
uniform samplerCube uEnvmapIrr;
uniform samplerCube uEnvmapPrefilter;
uniform float uBgType;
uniform float uExposure;

vec3 fixCubeLookup(vec3 _v, float _lod, float _topLevelCubeSize)
{
  // Reference:
  // Seamless cube-map filtering
  // http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
  float ax = abs(_v.x);
  float ay = abs(_v.y);
  float az = abs(_v.z);
  float vmax = max(max(ax, ay), az);
  float scale = 1.0 - exp2(_lod) / _topLevelCubeSize;
  if (ax != vmax) { _v.x *= scale; }
  if (ay != vmax) { _v.y *= scale; }
  if (az != vmax) { _v.z *= scale; }
  return _v;
}

void main()
{  
  vec3 dir = normalize(vDirection);
  fragColor = texture( uEnvmap, dir );

  vec4 color;

  // float lod = uBgType;
  // dir = fixCubeLookup(dir, lod, 256.0);
  if (uBgType == 7.0)
  {
    color = texture(uEnvmapIrr, dir);
  }
  else if (uBgType == 0.0)
  {
    color = texture(uEnvmap, dir);
  }
  else
  {
    // Omit 0th mip which roughness is 0.0. it is similar to the result of non filtered
    color = textureLod(uEnvmapPrefilter, dir, uBgType-1);
  }

  color *= exp2(uExposure);

  // Is gamma correction is included?
  fragColor = toFilmic(color);
}

