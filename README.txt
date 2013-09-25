This download contains two files, one is a library and one is an in-game tool.
Use the library to create sequences in your script, use the tool to create them.


camerasequencer.inc:

	A library containing functions and callbacks for managing camera sequences.

	Use by including the library with your gamemode/filterscript:

		#include <camerasequencer>

	Now you can use LoadCameraMover() to load camera data.
	Return this function to a variable:

		myCamVar = LoadCameraMover("Location");

	Now you can use this variable in functions like

		PlayCameraMover(myCamVar);


CameraEditor.pwn

	A filterscript utility tool for creating camera sequences.

	Use by loading the filterscript and using the displayed menu.


If there are any bugs please use GitHub's issue system or email me.


(c) Southclaw - Do what you want with it, but keep my name on it.
