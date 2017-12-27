# camera-sequencer

A library containing functions and callbacks for managing camera sequences.

Warning: The editor script does not compile. I might rewrite/update it in the future but only if I specifically need it for something.

## Installation

Simply install to your project:

```bash
sampctl package install Southclaws/camera-sequencer
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
