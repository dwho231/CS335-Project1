// The main ray tracer.

#pragma warning(disable : 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/JsonParser.h"
#include "parser/Parser.h"
#include "parser/Tokenizer.h"
#include <json.hpp>

#include "ui/TraceUI.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <fstream>
#include <iostream>

using namespace std;
extern TraceUI *traceUI;

// Use this variable to decide if you want to print out debugging messages. Gets
// set in the "trace single ray" mode in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates
// (x,y), through the projection plane, and out into the scene. All we do is
// enter the main ray-tracing method, getting things started by plugging in an
// initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

glm::dvec3 RayTracer::trace(double x, double y) {
  // Clear out the ray cache in the scene for debugging purposes,
  if (TraceUI::m_debug) {
    scene->clearIntersectCache();
  }

  ray r(glm::dvec3(0, 0, 0), glm::dvec3(0, 0, 0), glm::dvec3(1, 1, 1),
        ray::VISIBILITY);
  scene->getCamera().rayThrough(x, y, r);
  double dummy;
  glm::dvec3 ret =
      traceRay(r, glm::dvec3(1.0, 1.0, 1.0), traceUI->getDepth(), dummy);
  ret = glm::clamp(ret, 0.0, 1.0);
  return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j) {
  glm::dvec3 col(0, 0, 0);

  if (!sceneLoaded())
    return col;

  double x = double(i) / double(buffer_width);
  double y = double(j) / double(buffer_height);

  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  col = trace(x, y);

  pixel[0] = (int)(255.0 * col[0]);
  pixel[1] = (int)(255.0 * col[1]);
  pixel[2] = (int)(255.0 * col[2]);
  return col;
}

#define VERBOSE 0

// Do recursive ray tracing! You'll want to insert a lot of code here (or places
// called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray &r, const glm::dvec3 &thresh, int depth,
                               double &t) {
  isect i;
  glm::dvec3 colorC;
#if VERBOSE
  std::cerr << "== current depth: " << depth << std::endl;
#endif

  if (scene->intersect(r, i)) {
    // YOUR CODE HERE

    // An intersection occurred!  We've got work to do. For now, this code gets
    // the material for the surface that was intersected, and asks that material
    // to provide a color for the ray.

    // This is a great place to insert code for recursive ray tracing. Instead
    // of just returning the result of shade(), add some more steps: add in the
    // contributions from reflected and refracted rays.

    // Time of flight
    t = i.getT();
    // Intersection Normal
    glm::dvec3 N = i.getN();
    // Material of the intersected object
    const Material &m = i.getMaterial();

    // Ray intersection
    glm::dvec3 Q = r.at(t);
    // local shading contribution
    glm::dvec3 I = m.shade(scene.get(), r, i);

    // End recursion if depth is 0
    if (depth > 0) {
      // Handle reflection
      if (m.Refl()) {
        glm::dvec3 d = r.getDirection();
        glm::dvec3 Rd = glm::normalize(glm::reflect(d, N));
        ray R(Q, Rd, r.getAtten(), ray::REFLECTION);
        double t2;
        I += m.kr(i) * traceRay(R, thresh, depth - 1, t2);
      }

      // Handle refraction
      if (m.Trans()) {
        glm::dvec3 d = r.getDirection();
        double n_i, n_t;
        glm::dvec3 Nnew;

        if (glm::dot(d, N) > 0.0) {
          // Ray is inside object, going into air
          n_i = m.index(i);
          n_t = 1.0;
          Nnew = -N;
        } else {
          // Ray is outside object, going into material
          n_i = 1.0;
          n_t = m.index(i);
          Nnew = N;
        }

        double eta = n_i / n_t;
        glm::dvec3 Td = glm::refract(d, Nnew, eta);
        if (glm::length(Td) > 0.0) { // Check for total internal reflection
          Td = glm::normalize(Td);
          ray T(Q, Td, r.getAtten(), ray::REFRACTION);
          double t3;
          I += m.kt(i) * traceRay(T, thresh, depth - 1, t3);
        }
      }
    }

    colorC = I;
  } else {
    // No intersection. This ray travels to infinity, so we color
    // it according to the background color, which in this (simple)
    // case is just black.
    //
    // FIXME: Add CubeMap support here.
    // TIPS: CubeMap object can be fetched from
    // traceUI->getCubeMap();
    //       Check traceUI->cubeMap() to see if cubeMap is loaded
    //       and enabled.
	if (traceUI->cubeMap()) {
		colorC = traceUI->getCubeMap()->getColor(r);
	} else {
    colorC = glm::dvec3(0.0, 0.0, 0.0);
	}
  }
#if VERBOSE
  std::cerr << "== depth: " << depth + 1 << " done, returning: " << colorC
            << std::endl;
#endif
  return colorC;
}

RayTracer::RayTracer()
    : scene(nullptr), buffer(0), thresh(0), buffer_width(0), buffer_height(0),
      m_bBufferReady(false) {
}

RayTracer::~RayTracer() {}

void RayTracer::getBuffer(unsigned char *&buf, int &w, int &h) {
  buf = buffer.data();
  w = buffer_width;
  h = buffer_height;
}

double RayTracer::aspectRatio() {
  return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char *fn) {
  ifstream ifs(fn);
  if (!ifs) {
    string msg("Error: couldn't read scene file ");
    msg.append(fn);
    traceUI->alert(msg);
    return false;
  }

  // Check if fn ends in '.ray'
  bool isRay = false;
  const char *ext = strrchr(fn, '.');
  if (ext && !strcmp(ext, ".ray"))
    isRay = true;

  // Strip off filename, leaving only the path:
  string path(fn);
  if (path.find_last_of("\\/") == string::npos)
    path = ".";
  else
    path = path.substr(0, path.find_last_of("\\/"));

  if (isRay) {
    // .ray Parsing Path
    // Call this with 'true' for debug output from the tokenizer
    Tokenizer tokenizer(ifs, false);
    Parser parser(tokenizer, path);
    try {
      scene.reset(parser.parseScene());
    } catch (SyntaxErrorException &pe) {
      traceUI->alert(pe.formattedMessage());
      return false;
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (TextureMapException e) {
      string msg("Texture mapping exception: ");
      msg.append(e.message());
      traceUI->alert(msg);
      return false;
    }
  } else {
    // JSON Parsing Path
    try {
      JsonParser parser(path, ifs);
      scene.reset(parser.parseScene());
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (const json::exception &je) {
      string msg("Invalid JSON encountered ");
      msg.append(je.what());
      traceUI->alert(msg);
      return false;
    }
  }

  if (!sceneLoaded())
    return false;

  return true;
}

void RayTracer::traceSetup(int w, int h) {
  size_t newBufferSize = w * h * 3;
  if (newBufferSize != buffer.size()) {
    bufferSize = newBufferSize;
    buffer.resize(bufferSize);
  }
  buffer_width = w;
  buffer_height = h;
  std::fill(buffer.begin(), buffer.end(), 0);
  m_bBufferReady = true;

  /*
   * Sync with TraceUI
   */

  threads = traceUI->getThreads();
  block_size = traceUI->getBlockSize();
  thresh = traceUI->getThreshold();
  samples = traceUI->getSuperSamples();
  aaThresh = traceUI->getAaThreshold();

  // YOUR CODE HERE
  // FIXME: Additional initializations
}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */
void RayTracer::traceImage(int w, int h) 
{
  // Always call traceSetup before rendering anything.
  traceSetup(w, h);

  // Loop through every pixel and call tracePixel on it to render the entire image
  for (int i = 0; i < w; i++) 
  {
	  for (int j = 0; j < h; j++) 
    {
		tracePixel(i, j);
	  }
  }

    // Triggers the image to actually show all rendered pixels (it's ready to be shown to the user)
    m_bBufferReady = true;
}

// Implementing Adaptive Anti-Aliasing - the basic function that the UI calls when we want to do antiAliasing
int RayTracer::aaImage() 
{
  
  // Ensure we actually have a scene to perform this on to avoid problems
  if (sceneLoaded())
  {

    // Initialize a variable for the maximum number of times we can call our recursive function (so we don't go infinitely)
    int n = samples;

    // Loop over the entire image (as we will do this for all pixels)
    for (int i = 0; i < buffer_width; i++) 
    {
	    for (int j = 0; j < buffer_height; j++) 
      {
      // Divide the pixel into 4 rays (the corners)

      /* Diagram for how a pixel gets broken up at the first stage
    i,j+1     i+1,j+1
      |--------|
      | _  | _ |
      |    |   |
      |--------|
    i,j      i+1,j
      */

      // p1 = left of the pixel, p2 = bottom of the pixel, p3 = right of the pixel, p4 = top of the pixel
      double p1 = (i) / static_cast<double>(buffer_width);
      double p2 = (j) / static_cast<double>(buffer_height);
      double p3 = (i+1) / static_cast<double>(buffer_width);
      double p4 = (j+1) / static_cast<double>(buffer_height);

      // Call recursion to get our final pixel color, ensure that we don't go past the specificed depth the user set
      glm::dvec3 pixelColor = subsectionsAA(p1, p2, p3, p4, n);
      setPixel(i, j, pixelColor);
      
	    }
    }

    // Triggers the image to actually show all rendered pixels (it's ready to be shown to the user, now with the adaptive anti-aliasing)
    m_bBufferReady = true;
    return 1;

  }

  // The scene is not loaded so don't do anything
  else
  {
    return 0;
  }

}

// Implementing Adaptive Anti-Aliasing - the function we call to do recursion
glm::dvec3 RayTracer::subsectionsAA(double s1, double s2, double s3, double s4, int depth)
{

  // Get the color of each of these pixels to determine if we will need to break it up any further
  // (s1,s2) = bottomLeft, (s3,s2) = bottomRight, (s1,s4) = topLeft, (s3,s4) = topRight
  glm::dvec3 bottomLeft = trace(s1,s2);
  glm::dvec3 bottomRight = trace(s3,s2);
  glm::dvec3 topLeft = trace(s1,s4);
  glm::dvec3 topRight = trace(s3,s4);

  // Compare the color differences to find where the greatest difference lies (ie where we need to divide more)
  double colorDiff1 = glm::length(glm::abs(bottomLeft-bottomRight));
  double colorDiff2 = glm::length(glm::abs(bottomLeft-topLeft));
  double colorDiff3 = glm::length(glm::abs(bottomRight-topRight));

  // The two subsections that have the greatest difference
  double maxDiff = glm::max(colorDiff1, glm::max(colorDiff2, colorDiff3));

  // Compare this difference to our threshold to determine if it's necessary to break it up further 
  // Also ensure that we have not gone deeper than what our samples value says
  if ((maxDiff >= aaThresh) && (depth > 0))
  {
    // Decrement depth since we are going one deeper
    depth--;

    /* Diagram for how a pixel gets broken up at the midpoints
    s1,s4     s3,s4
      |--------|
      | _  | _ |midy
      |    |   |
      |--------|
    s1,s2 midx s3,s2
      */

    // Divide each of the subsections into 4 more sections (ie - split them at the midpoints of their current positions)
    double midX = (s1+s3)/2;
    double midY = (s2+s4)/2;

    // Call the recursive function for each of these newly broken up sections
    glm::dvec3 final1 = subsectionsAA(s1, s2, midX, midY, depth);
    glm::dvec3 final2 = subsectionsAA(midX, s2, s3, midY, depth);
    glm::dvec3 final3 = subsectionsAA(s1, midY, midX, s4, depth);
    glm::dvec3 final4 = subsectionsAA(midX, midY, s3, s4, depth);

    // Return the final color based on the average of all these recursed subsections
    glm::dvec3 subsectionPixelColor = (final1 + final2 + final3 + final4) / 4.0;
    return subsectionPixelColor;
  }

  // Otherwise, if we have determined that we don't need to break it up any further
  else
  {
    // Set the new color for that pixel to be the average color of our subsections
    glm::dvec3 pixelColor = (bottomLeft + bottomRight + topLeft + topRight) / 4.0;
    return pixelColor;
  }
}

// We aren't utilizing this function since we aren't using threads
bool RayTracer::checkRender() {
  return true;
}

// We aren't utilizing this function since we aren't using threads
void RayTracer::waitRender() {
}


glm::dvec3 RayTracer::getPixel(int i, int j) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  return glm::dvec3((double)pixel[0] / 255.0, (double)pixel[1] / 255.0,
                    (double)pixel[2] / 255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;

  pixel[0] = (int)(255.0 * color[0]);
  pixel[1] = (int)(255.0 * color[1]);
  pixel[2] = (int)(255.0 * color[2]);
}
