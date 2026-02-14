#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI *traceUI;

#include "../fileio/images.h"
#include <glm/gtx/io.hpp>
#include <iostream>

using namespace std;
extern bool debugMode;

Material::~Material() {}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene *scene, const ray &r, const isect &i) const {
  // YOUR CODE HERE

  // For now, this method just returns the diffuse color of the object.
  // This gives a single matte color for every distinct surface in the
  // scene, and that's it.  Simple, but enough to get you started.
  // (It's also inconsistent with the phong model...)

  // Your mission is to fill in this method with the rest of the phong
  // shading model, including the contributions of all the light sources.
  // You will need to call both distanceAttenuation() and
  // shadowAttenuation()
  // somewhere in your code in order to compute shadows and light falloff.
  //	if( debugMode )
  //		std::cout << "Debugging Phong code..." << std::endl;

  // When you're iterating through the lights,
  // you'll want to use code that looks something
  // like this:
  //
  // for ( const auto& pLight : scene->getAllLights() )
  // {
  //              // pLight has type Light*
  // 		.
  // 		.
  // 		.
  // }

  // Initialize S = goal pixel/position being shaded, N = surface normal, v = view direction
  glm::dvec3 S = r.at(i);
  glm::dvec3 N = glm::normalize(i.getN());
  glm::dvec3 V = -glm::normalize(r.getDirection());

  // Add the emissive and ambient lights to the color
  glm::dvec3 color = ke(i);
  color += ka(i) * scene->ambient();

  // Debug mode
  if (debugMode) {
	cerr << "shade: S=" << S << " N=" << N << " V=" << V << " kd=" << kd(i)
	  << " ks=" << ks(i) << " ke=" << ke(i) << " ka=" << ka(i) << "\n";
  }

  // Loop through all the lights in the scene and add their contributions
  for (const auto &pLight : scene->getAllLights()) {
    glm::dvec3 L = pLight->getDirection(S);
	glm::dvec3 lightColor = pLight->getColor();
	double distAtt = pLight->distanceAttenuation(S);

	// Create a shadow ray from the surface point towards the light source
	ray shadowRay(S + N * RAY_EPSILON, L, glm::dvec3(1.0), ray::SHADOW);
	glm::dvec3 shadowAtt = pLight->shadowAttenuation(shadowRay, S);

	// Get the diffuse contribution using the Phong model
	double diffuseValue = std::max(0.0, glm::dot(N, L));
	if (diffuseValue > 0.0) {
	  glm::dvec3 diff = kd(i) * lightColor * diffuseValue;
	  glm::dvec3 H = glm::normalize(L + V);

	  double specularValue = std::max(0.0, glm::dot(N, H));
	  glm::dvec3 spec = ks(i) * lightColor * pow(specularValue, shininess(i));

	  // Debug Mode
	  if (debugMode) {
		cerr << " light: L= " << L << " color= " << lightColor << " distAtt= " << distAtt << " shadowAtt= "
		     << shadowAtt << " nDotL= " << diffuseValue << " nDotH= " << specularValue << "\n";
	  }

	  glm::dvec3 att = shadowAtt * distAtt;
	  color += (diff + spec) * att;
	}
  }
  return glm::clamp(color, glm::dvec3(0.0), glm::dvec3(1.0));
}

TextureMap::TextureMap(string filename) {
  data = readImage(filename.c_str(), width, height);
  if (data.empty()) {
    width = 0;
    height = 0;
    string error("Unable to load texture map '");
    error.append(filename);
    error.append("'.");
    throw TextureMapException(error);
  }
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2 &coord) const {
  // YOUR CODE HERE
  //
  // In order to add texture mapping support to the
  // raytracer, you need to implement this function.
  // What this function should do is convert from
  // parametric space which is the unit square
  // [0, 1] x [0, 1] in 2-space to bitmap coordinates,
  // and use these to perform bilinear interpolation
  // of the values.

  if (width <= 0 || height <= 0 || data.empty())
	return glm::dvec3(1.0, 1.0, 1.0);

  double u = coord.x;
  double v = coord.y;

  double x = u * (width - 1);
  double y = v * (height - 1);

  int x0 = static_cast<int>(floor(x));
  int y0 = static_cast<int>(floor(y));
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  double tx = x - x0;
  double ty = y - y0;

  // Sample the four surrounding pixels
  glm::dvec3 c00 = getPixelAt(x0, y0);
  glm::dvec3 c10 = getPixelAt(x1, y0);
  glm::dvec3 c01 = getPixelAt(x0, y1);
  glm::dvec3 c11 = getPixelAt(x1, y1);

  // Bilinear interpolation using the surrounding pixel values
  glm::dvec3 c0 = c00 * (1.0 - tx) + c10 * tx;
  glm::dvec3 c1 = c01 * (1.0 - tx) + c11 * tx;
  glm::dvec3 c = c0 * (1.0 - ty) + c1 * ty;

  return c;
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const {
  // YOUR CODE HERE
  //
  // In order to add texture mapping support to the
  // raytracer, you need to implement this function.

  if (width <= 0 || height <= 0 || data.empty())
  	return glm::dvec3(1.0, 1.0, 1.0);

  // Make sure the coordinates are within the bounds of the image, if not clamp them to the edge
  if (x < 0)
    x = 0;
  else if (x >= width)
	x = width - 1;

  if (y < 0)
    y = 0;
  else if (y >= height)
    y = height - 1;

  // Get the index of the pixel in the data array
  int idx = (x + y * width) * 3;

  // Convert the pixel values from [0, 255] to [0.0, 1.0] and return as a glm::dvec3
  double r = double(data[idx]) / 255.0;
  double g = double(data[idx + 1]) / 255.0;
  double b = double(data[idx + 2]) / 255.0;
  return glm::dvec3(r, g, b);
}

glm::dvec3 MaterialParameter::value(const isect &is) const {
  if (0 != _textureMap)
    return _textureMap->getMappedValue(is.getUVCoordinates());
  else
    return _value;
}

double MaterialParameter::intensityValue(const isect &is) const {
  if (0 != _textureMap) {
    glm::dvec3 value(_textureMap->getMappedValue(is.getUVCoordinates()));
    return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
  } else
    return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}
