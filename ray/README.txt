CS335 Project 1 - RayTracing
Authors: Jesse Cottrell, Fatima Fayazi, Daniel Howard, Madison Lanaghan, Katie Lester, Dillon Scott
Pink Group

Compile Instructions:
  Ensure all files are correctly downloaded, including the files within scenemodels and cubemapping
  In the terminal, change your directory so that you are within ray/build/
  Run "make" *
    * A few errors will appear, but will not hinder the build/make process
  Run "./ray"
  A UI window will now appear and will give you many options for image rendering:
    Load a file by selecting "File", "Load Scene", and then select your .json file
    Cubemapping can be done by selecting "File", "Load Cubemap" and ensuring that you load each dimension's (X, Y, and Z) positive and negative orientations
    Anti-Aliasing can be done by checking the "Antialias" box and adjusting your sample size and AA threshold (only after a scene is loaded)

Common Usage:
  Rendering Images
    Using this image, we have a way to render images utilizing the fundamentals of Ray Tracing. It handles reflections and refractions of light, considerations
    based on the materials of objects in our scene, and the ability to utilize trimesh to create an appealing looking image.
  Anti-Aliasing
    Anti-Aliasing will make the edges on shapes look a lot less jagged. Increasing the samples and threshold will increase just how much less jagged the 
    shape will appear, thus making your image look a lot "realer".
  CubeMapping
    CubeMapping is used to give us an environment that our objects "live in". It is implemented to show a different angle depending on where the camera
    is positioned/facing.


