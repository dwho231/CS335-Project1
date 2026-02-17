#include <cmath>
#include <iostream>

#include "light.h"
#include "ray.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3 &) const {
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}

glm::dvec3 DirectionalLight::shadowAttenuation(const ray &r, const glm::dvec3 &p) const {
  // Transparent shadow attenuation: accumulate transmissivity along the shadow ray
  glm::dvec3 lightDir = -orientation; // Direction from the point to the light source
  glm::dvec3 attenuation(1.0, 1.0, 1.0);
  glm::dvec3 start = p;
  const double RAY_EPSILON = 1e-4;
  while (true) {
    ray shadowRay(start + RAY_EPSILON * lightDir, lightDir, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
    isect i;
    if (scene->intersect(shadowRay, i)) {
      const Material &m = i.getMaterial();
      if (m.Trans()) {
        // Multiply by transmissivity and continue
        attenuation *= m.kt(i);
        // Move start to just past this intersection
        start = shadowRay.at(i.getT());
        // If attenuation is (close to) zero, return black
        if (glm::length(attenuation) < 1e-6) {
          return glm::dvec3(0.0, 0.0, 0.0);
        }
        // Continue tracing
      } else {
        // Opaque object blocks all light
        return glm::dvec3(0.0, 0.0, 0.0);
      }
    } else {
      // No more intersections, return accumulated attenuation
      return attenuation;
    }
  }
}

glm::dvec3 DirectionalLight::getColor() const { return color; }

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3 &) const {
  return -orientation;
}

double PointLight::distanceAttenuation(const glm::dvec3 &P) const {
  // YOUR CODE HERE

  // You'll need to modify this method to attenuate the intensity
  // of the light based on the distance between the source and the
  // point P.  For now, we assume no attenuation and just return 1.0

  // Compute the distance 'd' between the light source and the point P
  // Then, return the attenuation factor:

  // f(d) = min(1, 1/(a + b*d + c*d^2))
  double d = glm::length(position - P);
  double denominator = constantTerm + linearTerm * d + quadraticTerm * d * d;
  if (denominator <= 0) {
    // do not want to divide by zero or negative values. 
    return 1.0; 
  }
  double attenuation = 1.0 / denominator;
  return min(1.0, attenuation);
  
}

glm::dvec3 PointLight::getColor() const { return color; }

glm::dvec3 PointLight::getDirection(const glm::dvec3 &P) const {
  return glm::normalize(position - P);
}

glm::dvec3 PointLight::shadowAttenuation(const ray &r, const glm::dvec3 &p) const {
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  // to avoid self-shadowing we want it to be offset from the light source aka greater than epsilon.

  isect i;
  ray shadowRay(r);
  if (scene->intersect(shadowRay, i)){
	// Check if the intersection is between the point and the light source
	double lightDist = glm::length(position - p);
	// Intersect is closer to light source than p, then p is in shadow.
	if (i.getT() > RAY_EPSILON && i.getT() < lightDist) {
		// The point is in shadow, return the attenuation factor
		return glm::dvec3(0.0,0.0,0.0); 
	}
  }

  return glm::dvec3(1.0,1.0,1.0); 
}

#define VERBOSE 0
