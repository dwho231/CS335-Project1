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
  // YOUR CODE HERE:

  isect i;
  if (scene->intersect(r, i)&& i.getT() > RAY_EPSILON){ {
	// The point is in shadow, return the attenuation factor
  //should fix the self shadow issue by checking the intersect point. 
	return glm::dvec3(0.0,0.0,0.0); 
  }
  return glm::dvec3(1.0,1.0,1.0);
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
  if (scene->intersect(r, i)){
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
