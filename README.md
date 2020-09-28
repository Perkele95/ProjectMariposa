# Project::Mariposa
 // Mariposa is a game/game engine for windows //

This is my attempt at creating my own game/game engine as a personal project.

The main goals of Mariposa is to further my own understanding certain things:
 - windows applications
 - game engine API
 - games in general
 - C programming
 - data-oriented design
 - 3D rendering

The end goal is to use Mariposa to create executable games for windows.


## Data-Oriented Design
The entire codebase is written and compiled in .cpp and .h files. Despite using C++ I use a certain subset of C++ which is
pretty much the same as C but with a couple features from C++. The reason for this is threefold:
* My own experience with OOP for game engine development in my game engine called Dyson has left me wanting to try something else. OOP made the code "simple" but way too complex and vague. Multiple inheritance, polymorphism and the general attitude that everything needs to be a class made development harder.
* C as a language has no classes. I want to follow a data-oriented / compression oriented approach to development. This means I treat problems directly rather than abstracting them into objects/concepts.
* Performance.