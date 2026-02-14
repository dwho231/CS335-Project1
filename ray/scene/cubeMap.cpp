#include "cubeMap.h"
#include "../scene/material.h"
#include "../ui/TraceUI.h"
#include "ray.h"
extern TraceUI *traceUI;

glm::dvec3 CubeMap::getColor(ray r) const {
  // YOUR CODE HERE
  // FIXME: Implement Cube Map here

  glm::dvec3 dir = r.getDirection();
  double absX = fabs(dir.x);
  double absY = fabs(dir.y);
  double absZ = fabs(dir.z);
  int face;
  double u, v;

  // Find the major axis
  if (absX >= absY && absX >= absZ) {
    // X
    if (dir.x > 0) {
      face = 0; // +X
      u = dir.z / absX;
    } else {
      face = 1; // -X
      u = -dir.z / absX;
    }
	v = dir.y / absX;
  } else if (absY >= absX && absY >= absZ) {
    // Y
    if (dir.y > 0) {
      face = 2; // +Y
      v = dir.z / absY;
    } else {
      face = 3; // -Y
      v = -dir.z / absY;
    }
	u = dir.x / absY;
  } else {
    // Z
    if (-dir.z > 0) {
      face = 4; // +Z
      u = dir.x / absZ;
    } else {
      face = 5; // -Z
      u = -dir.x / absZ;
    }
	v = dir.y / absZ;
  }

  // Convert u,v from [-1,1] to [0,1]
  u = 0.5 * (u + 1.0);
  v = 0.5 * (v + 1.0);

  // In case there is no map for this face
  if (!tMap[face])
    return glm::dvec3(1.0, 1.0, 1.0); 

  return tMap[face]->getMappedValue(glm::dvec2(u, v));
}

CubeMap::CubeMap() {}

CubeMap::~CubeMap() {}

void CubeMap::setNthMap(int n, TextureMap *m) {
  if (m != tMap[n].get())
    tMap[n].reset(m);
}
