# MARIPOSA
*author: Arle Åmdal Vaara*

*I am forever grateful to C. Muratori and his project "Handmade Hero", which was both inspiring and extremely helpful.*

This is my attempt at creating my own game/game engine as a personal project. Mariposa will have minimal use of external libraries.

The main goals of Mariposa is to further my own understanding of:
 - C programming
 - Windows applications
 - Game development
 - Game engine API design
 - Data-oriented design
 - 3D rendering

### Future goals
The end goal is to use Mariposa to create executable games for windows. Mariposa will preferrably use Vulkan SDK as its rendering API.

### Unity build
Mariposa has all it's .cpp files included into one .cpp file to create a single compilation unit instead of one for each source file. This is mainly a personal choice,
as well as an experiment to see if I prefer it over the standard way (header files declare, compilation files define).

### Hot-loading
Mariposa.cpp is compiled as a DLL that can be dynamically loaded and unloaded at will. This means I can edit mariposa.cpp/.h and rebuild while running. Of course this is
only intended as a dev tool, and not for release. This feature is only for the DLL.

### Looped Live recording
Gamestate and input can be recorded by pressing 'L'. This is saved as .mpr files and the input recording is looped after recording ends.

### Data-Oriented Design
The entire codebase is written and compiled in .cpp and .h files. Despite using C++ I use a certain subset of C++ which is
pretty much the same as C but with a couple features from C++. The reason for this is threefold:
* My own experience with OOP for game engine development in my game engine called Dyson has left me wanting to try something else. OOP made the code "simple" but way too complex and vague. Multiple inheritance, virtual functions and the general attitude that everything needs to be a class made development harder.
* C as a language has no classes. I want to follow a data-oriented / compression oriented approach to development. This means I treat problems directly rather than abstracting them into objects/concepts.
* Performance.

C++ features that come in handy:
* Function overloading. Provides some polymorphism to functions. Who needs classes anyway?
* No longer need to typedef structs and enums