# ps2gl

![CI](https://github.com/ps2dev/ps2gl/workflows/CI/badge.svg)

- [Introduction](#introduction)
- [Documentation](#documentation)
- [Latest version](#latest-version)
- [Installing](#installing)
- [Building](#building)
- [Building the examples](#building-the-examples)
- [Completeness](#completeness)
- [Performance](#performance)
- [Gotchas](#gotchas)
- [Bugs](#bugs)
- [About](#about)
- [Changelog](#changelog)

## Introduction
ps2gl is an OpenGL*-like API for the ps2. It is intended to be useful for anyone looking for a familiar API on the ps2, but its main purposes in life are to provide an easier way to write games and to serve as an example of a ps2 rendering layer. In a perfect world ps2gl would be just another OpenGL* implementation, but unfortunately that is not to be. The reason, aside from licensing issues, is that many parts of the OpenGL* API are not well suited to the PlayStation 2 architecture and would require software emulation, which is undesirable.

Still, all hope is not lost; many of the calls act the same as their OpenGL* counterparts, while the rest behave similarly.

Finally, ps2gl is still quite young, and developers who decide to use it can expect to find bugs, inconsistencies, and incomplete features. Since ps2gl is also an open source project, its users are **welcome to participate in its development**. One of ps2gl's main design requirements is that it be easy to understand and modify. CVS (version control) write access is available to frequent contributors, and read access to the working CVS tree is always available.

## Documentation

The API documentation for ps2gl is generated from source comments using a wonderful program called [doxygen](http://www.doxygen.org). The full documentation for both the external API (gl* and pgl*) and the internal implementation is included:

- [Public API (gl* and pgl*)](https://ps2dev.github.io/ps2gl/modules.html)
- [Full Documentation](https://ps2dev.github.io/ps2gl/)

You'll notice that internal documentation is quite scarce at the moment... It will gradually get better.

## Latest Version
The latest version can always be found on the homepage: [https://www.ps2-pro.com/projects/ps2gl/](https://www.ps2-pro.com/projects/ps2gl/) for game development and now [http://ps2gl.playstation2-linux.com](http://ps2gl.playstation2-linux.com) for the Linux version. The homepage also hosts bug tracking, news, CVS versions, and discussion groups.

## Installing
ps2gl depends on another package called ps2stuff. You'll find it at [https://www.ps2-pro.com/projects/ps2gl/](https://www.ps2-pro.com/projects/ps2gl/) or [http://ps2gl.playstation2-linux.com](http://ps2gl.playstation2-linux.com). ps2stuff is a collection of utilities for dealing with the ps2 hardware and forms the layer on top of which ps2gl is built.

To use ps2gl, you'll want to link against the `libps2stuff.a` and `libps2gl.a` libraries. There is also a very poor imitation of glut used mainly for testing, but useful to get something running quickly. The libraries are found in the directories `obj_buildname`, where `buildname` is the name of the build you want to use.

Here are the most useful game development builds:

- **debug** - No optimization, contains debug information and error checking. This build is useful for stepping through in a debugger.
- **optimized** - Optimized, but contains debug info and error checking. Mainly for day-to-day coding.
- **release** - Optimized, no debug info or error checking. For, well... release.

The Linux binary packages only contain the 'linux' build, which is similar to the 'optimized' build described above.

## Building

Who can build it, and why

Right now, building ps2gl requires a patched version of gcc, the ps2stuff project, make, perl, and sed. (The compiler patches can be found on the ps2stuff homepage). ps2gl will probably not build with CodeWarrior.

The reason for using a patched version of gcc is only because some of the ps2stuff header files contain code that require the patches, but this code is not actually used by ps2gl. This is an annoying restriction which should change in the future.

My reasoning for using gcc and not maintaining compatibility with CodeWarrior is as follows: gcc is the de facto standard from SCEI, so it needs to build with gcc; the two compilers have very different strengths and weaknesses, and rather than taking the lowest common denominator of both, I feel that it's better to optimize for one compiler; at this point, most studios will have gcc installed somewhere, so it's at least accessible to CodeWarrior users.

Obviously, not using CodeWarrior makes things more difficult for a lot of developers and warrants some discussion. In that light, I'll make a forum on the homepage for that purpose!

### Linux/gcc

The only configuration necessary should be to set the 'PS2STUFF' variable so that it points to the ps2stuff project. After that 'make' should do it. See the comments at the top of the Makefile for more build targets.

When writing the makefile for your own project, I've recently had to link in this order (note that ps2gl is there twice): ps2gl ps2glut ps2gl ps2stuff. Go figure..

### Windows/gcc

??? - the Makefile uses several other programs to do its job (make, sed, perl) which aren't normally present on Windows systems... It shouldn't be difficult to port the Makefile to windows; in fact, it has been done, but I don't have a copy of the results. If any Windows users out there would like to contribute their port, I'm sure it would be very welcome...

## Building the examples
**linux/gcc**

After expanding the ps2gl and ps2stuff archives into the same directory, `chdir` to `ps2gl/example/box`. Now 'make run' should do it.

NOTE: There is a known bug where `make` sometimes tries to run the 'vcl' program. If this happens, `cd` to `ps2gl/vu1` and do a 'touch *_vcl.vsm.'
## Completeness

### Complete
Functions and such listed below should be as complete as they're going to get and are fair game for bug reports. Be sure to check "Ain't no way" below for exceptions. Note: everything should work as well or as poorly for display lists as for immediate execution.

- glBegin/End, glVertex3/4f, glNormal, glTexCoord2f
- glDrawArrays
- 8/24/32 bit textures
- materials
- glMultMatrix, glRotate, Translate, Scale, Frustum..
- projection and modelview stacks -- depth is 16
- depth and alpha tests
- per-vertex diffuse material changes with glColorMaterial
- face culling (back/front)

### Partially complete

- lighting -- no spots, only local_viewer for speculars

### Ain't no way
Here are some things that are not considered a priority, for various reasons. Basically, these are known to be missing/broken but are not on the todo list. If you want something here and think it can be implemented efficiently, let us know.

- no GL_LINE_LOOP - treated as a line strip

## Gotchas

- Textures are upside down! GL defines images as starting from the top, right corner, but the PS2 doesn't work that way.
- `glDrawArrays` does not [mostly] copy data. See the API documentation.
- `glDrawArrays` does not accept non-zero strides.
- Limited alpha blending (src_alpha and one_minus_src_alpha).

## Performance

There are pretty much two ps2gl performance bottlenecks over which the application has control: DMA transfer and VU1 rendering. ps2gl uses a number of different VU1 renderers to do transform and lighting, choosing the fastest one that fits the current rendering requirements. Take a look at the 'performance' example to get an idea of how different parameters influence the choice of VU1 renderer and the impact on speed.

### Tips

What YOU can do to make things go faster!

- **Use display lists**: Display lists have been optimized at the expense of immediate-mode. The main problem with them now is inefficient use of memory when used to cache glBegin/glEnd draw commands, which brings us to...
- **Use DrawArrays**: Memory is almost allocated efficiently (at least it's loosely related to the size of the input data) and there's no copying. When rendering a model, group each of [vertices, normals, tex coords, colors] contiguously in memory. For example:
  
  <all vertices>
  <all normals>
  <all tex coords>
  
  NOT:
  
  <vertex0, normal0, texCoord0>
  <vertex1, normal1, texCoord1>
  ...
  
- **For geometry that changes frequently**: We have a problem. The DrawArrays call and the creation of display lists take a fair amount of time so we don't want to be doing it every frame. Furthermore, if only the values of vertices and normals are changing (and not the topology), like with a skinned model, we shouldn't need to rebuild the display list since the data is passed by reference. It would be nice if we could just create one display list that contains calls to DrawArrays pointing at our data, and then change the data behind the display list's back. But according to the documentation, glDrawArrays only *mostly* references the array data, i.e., some data does get copied.
  
  Fear not, for all hope is not lost. The only time the display list will copy any data is when it needs to transfer elements that start on a non-qword-aligned boundary. That means that if all your vertices, normals, tex coords, and colors are either 2 or 4 floats everything should be aligned correctly and nothing will be copied. (It's useful to note at this point that the "w" field of all vertices is implicitly forced to 1.0f, so it doesn't matter what is actually written to that field in memory.) The only hitch in this plan is that glNormalPointer implicitly sets the length of normals to be 3 elements. For this reason, ps2gl has a new call 'pglNormalPointer' that allows you to specify the length of the normals, as in glVertexPointer.
  
  So to render geometry that's changing frequently, here's the plan:
  
  1. Allocate memory for the data starting on a qword boundary (malloc/new).
  2. Store vertices as (xyz?), tex coords as (uv), and normals as (xyz?).
  3. Create a display list and render with glDrawArrays.
  4. Now the data can be modified and glCallList will still render it correctly.
  
- **Writing custom renderers** is, of course, the best way to optimize your app. Everything from the DMA chains that are created to the microcode used can be overridden by the application. Some ideas:
  - Write a dummy renderer that builds DMA chains that can be saved to a file, then another renderer that just calls those chains.


## About
Tyler Daniel, SCEA R&D  
tyler_daniel@playstation.sony.com  
Copyright © 2002 Sony Computer Entertainment America  
All Rights Reserved
## Bugs
Bug reports should be submitted to the appropriate homepage, which hosts a bug tracking system. (Be sure to check the bug tracker to see if the problem has already been reported and/or resolved.)

## Changelog

### 0.3
- Can now define custom prim types and attributes, tying them to custom renderers and override default renderers.
- Lots of bug fixes!

### New in version 0.2.1
- `pglNormalPointer` and corresponding tip in the "performance" page.
- 8-bit texture support.

### New in version 0.2
- Documentation!
- CodeWarrior-linkable version and example project.
- Many ps2gl implementation changes to make it easier to understand and modify.
- Fixed bug where state changes sometimes affected geometry drawn previously.
- Draw and display buffers can be set up/changed freely by the application and used as textures.
- Gs memory initialization is much more flexible.
- `glutInit()` will take an optional iop module path argument.

### New in version 0.1.1
- Depth and alpha tests.
- [Very] limited alpha blending; see Status.
- Per-vertex diffuse material changes using `glColorMaterial`.
- Backface/frontface culling.
- Metrics module to monitor uploads of textures, cluts, microcode, etc. -- see `metrics.h`.

---
*OpenGL(R) is a registered trademark of Silicon Graphics, Inc.*
