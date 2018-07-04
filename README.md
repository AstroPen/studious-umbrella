# studious-umbrella
My home projects, mostly in C++. They are desigined to be used with a single compilation unit, so most files don't have headers.

GameEngine is the largest, and contains code for a game engine built in C++ on top of SDL. It is currently a work in progress and is probably difficult to run on other machines.

Chess is an older project. It contains C++ code for a command line chess game and a simple ai to play against.

Common contains reuseable code, most of which was pulled out of GameEngine.

HashTable is a work in progress hash table/hash set. I plan to finish them when I actually need them. 
Note that it doesn't use templates, and instead requires \#defines for the types.
