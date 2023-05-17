# ps2gl

![CI](https://github.com/ps2dev/ps2gl/workflows/CI/badge.svg)


[PS2GL Documentation](https://ps2dev.github.io/ps2gl/)

ps2gl is an OpenGL*-like API for the ps2. It is intended to be useful for anyone looking for a familiar API on the ps2, but its main purposes in life are to provide an easier way to write games and to serve as an example of a ps2 rendering layer. In a perfect world ps2gl would be just another OpenGL* implementation, but unfortunately, that is not to be. The reason, aside from licensing issues, is that many parts of the OpenGL* API are not well suited to the PlayStation 2 architecture and would require software emulation, which is undesirable.

Still, all hope is not lost; many of the calls act the same as their OpenGL* counterparts, while the rest behave similarly.

Finally, ps2gl is still quite young, and developers who decide to use it can expect to find bugs, inconsistencies, and incomplete features. Since ps2gl is also an open source project, its users are **welcome to participate in its development**. One of ps2gl's main design requirements is that it be easy to understand and modify. CVS (version control) write access is available to frequent contributors, and read access to the working CVS tree is always available.
