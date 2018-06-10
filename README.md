# samp-camera-sequencer

[![sampctl](https://shields.southcla.ws/badge/sampctl-samp--camera--sequencer-2f2f2f.svg?style=for-the-badge)](https://github.com/Southclaws/samp-camera-sequencer)

A library for creating camera sequences. A camera sequence is a set of camera
nodes which contain shot data and when played for a player, their camera will
move between the nodes.

Files are used to store the coordinates and sequence data. Each camera node
consists of coordinates and timing information.

There is an editor script however it does not compile - I somehow lost track of
these two scripts over the years and the editor in this repository is miles
behind the actual library version so there are a lot of mismatched function and
variable names. I might rewrite/update it in the future but only if I
specifically need it for something.

## Installation

Simply install to your project:

```bash
sampctl package install Southclaws/samp-camera-sequencer
```

Include in your code and begin using the library:

```pawn
#include <camera-sequencer>
```

## Library Usage

Use `LoadCameraMover` to load camera data, this returns a handle:

```pawn
new camera = LoadCameraMover("sequencefile.dat");
```

Now you can use this handle in functions like

```pawn
PlayCameraMover(camera);
```

## Creating Camera Sequences

Simply run the package:

```bash
sampctl package run
```

And connect to `localhost:7777`.
