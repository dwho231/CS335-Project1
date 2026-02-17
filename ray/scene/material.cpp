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

  // Initialize surfacePoint, normalVector, and viewDirection for use in the Phong model
  glm::dvec3 surfacePoint = r.at(i);
  glm::dvec3 normalVector = glm::normalize(i.getN());
  glm::dvec3 viewDirection = -glm::normalize(r.getDirection());

  // Add the emissive and ambient lights to the color
  glm::dvec3 color = ke(i);
  color += ka(i) * scene->ambient();

  // Debugging
  if (debugMode) {
	cerr << "shade: surfacePoint=" << surfacePoint << " normalVector=" << normalVector << " viewDirection=" << viewDirection << " kd=" << kd(i)
	  << " ks=" << ks(i) << " ke=" << ke(i) << " ka=" << ka(i) << "\n";
  }

  // Loop through all the lights in the scene and add their contributions
  for (const auto &pLight : scene->getAllLights()) {
    glm::dvec3 lightDirection = pLight->getDirection(surfacePoint);
    glm::dvec3 lightColor = pLight->getColor();
    double distAtt = pLight->distanceAttenuation(surfacePoint);

    // Create a shadow ray from the surface point towards the light source
    ray shadowRay(surfacePoint + normalVector * RAY_EPSILON, lightDirection, glm::dvec3(1.0), ray::SHADOW);
    glm::dvec3 shadowAtt = pLight->shadowAttenuation(shadowRay, surfacePoint);

    // Get the diffuse contribution using the Phong model
    double diffuseValue = max(0.0, glm::dot(normalVector, lightDirection));
    if (diffuseValue > 0.0) {
      glm::dvec3 diffuse = kd(i) * lightColor * diffuseValue;
      glm::dvec3 halfwayVector = glm::normalize(lightDirection + viewDirection);

      // Get the specular contribution using the Phong model
      double specularValue = max(0.0, glm::dot(normalVector, halfwayVector));
      glm::dvec3 specular = ks(i) * lightColor * pow(specularValue, shininess(i));

      // Debugging
      if (debugMode) {
      cerr << " light: lightDirection= " << lightDirection << " color= " << lightColor << " distAtt= " << distAtt << " shadowAtt= "
          << shadowAtt << " nDotL= " << diffuseValue << " nDotH= " << specularValue << "\n";
      }

      // Add the diffuse and specular contributions, and multiply them by the 2 attenuations multiplied together
      glm::dvec3 combinedAtt = shadowAtt * distAtt;
      color += (diffuse + specular) * combinedAtt;
    }
    }
    // Clamp the color to the correct range
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

  if (width <= 0 || height <= 0 || data.empty())
	return glm::dvec3(1.0, 1.0, 1.0);

  double u = coord.x;
  double v = coord.y;

  // Convert from parametric space to the bitmap coordinates
  double x = u * (width - 1);
  double y = v * (height - 1);

  // Get the coordinates of the four surrounding pixels
  int x0 = static_cast<int>(floor(x));
  int y0 = static_cast<int>(floor(y));
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  // Get the fractional part of the coordinates for bilinear interpolation
  double tx = x - x0;
  double ty = y - y0;

  // Get the color of the four surrounding pixels
  glm::dvec3 color00 = getPixelAt(x0, y0);
  glm::dvec3 color10 = getPixelAt(x1, y0);
  glm::dvec3 color01 = getPixelAt(x0, y1);
  glm::dvec3 color11 = getPixelAt(x1, y1);

  // Bilinear interpolation using the surrounding pixel values
  glm::dvec3 topBlend = color00 * (1.0 - tx) + color10 * tx;
  glm::dvec3 bottomBlend = color01 * (1.0 - tx) + color11 * tx;
  glm::dvec3 interpolatedColor = topBlend * (1.0 - ty) + bottomBlend * ty;

  return interpolatedColor;
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const {

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
  int index = (x + y * width) * 3;

  // Convert the pixel values from [0, 255] to [0.0, 1.0]
  double r = double(data[index]) / 255.0;
  double g = double(data[index + 1]) / 255.0;
  double b = double(data[index + 2]) / 255.0;
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
