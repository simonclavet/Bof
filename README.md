
My engine, in which I test stuff.


To get and build:

Install Git, CMake, Visual Studio.
```
git clone https://github.com/simonclavet/Bof.git
cd Bof
GenerateProject.bat
StartVisualStudioSolution.bat

```

Then hit F5.

For now I just reproduced the vulkan tutorial, using the modern cpp bindings vulkan.hpp.

I follow the [Carmack coding style](http://number-none.com/blow/blog/programming/2014/09/26/carmack-on-inlined-code.html). 
This means just a small number of large methods for init and tick, then a bunch of functionnal helpers. 
Functionnal helpers have some arguments const, some arguments are the results, some arguments are inout (so, not really functionnal). 
If a method is called only once, it should be either inlined, or made functional.

Some goals:

Completelly separate data from procedures (data-oriented-design)
with an entity-component-system. No data exists outside of a simple plain-old-data component, that can
exist on any machine, in a file or in memory (no init code is needed to wake from memory). 
This means no pointers, no inheritance. Just a good distributed physics engine referenced by simple data components and
modified by distributed systems. Creating a level means playing the game and making the things. It saves
and replicates automatically. A saved level on disk is not necessary since the simulation is expected to run
indefinitelly (still need it for backup). There is no 'editor', just a simulation in which players can create and move
cubes. There is only one level.

Concretelly, make a persistent world where players can create, destroy, and move cubes. Cubes have different sizes, from 0.2m to 20m.
Moving cubes around by picking them and throwing them happens at 60 fps on local machines, but the simulation
is shared between players that are 200ms away. So we hide the lag with something new.



Come up with a new way to think about multiplayer engines: 'Relativistic deterministic lockstep with rollback'
pushed to the limit with a fine grained distributed physics engine 
that is shared by an unlimited number of players in a large persistant world. 

The world is separated in sectors. Sectors are independent during simulation, but can 'send' entities to neighboring 
sectors. Interactions between entities in different sectors are impossible, so the game needs to take care of that: the
border region is a 'no action zone', in which players and objects move in straight line with no acceleration or
interaction until they are in the other zone. This should actually take just one frame. We can see but not interact with
entities in other sectors. Sectors are large enough so that it does not matter. Let's say 100 or 1000 players should
fit confortably in a sector, and it is expected that players don't really need to switch sector very often.

A single server machine should be able to simulate a sector. Clients also simulate their sector as best as they can,
but get ground truth from the server once in a while (every couple of seconds, because this is big).


One server process per sector holds the ground truth. Each client has many local 
players (splitscreen). Each client runs two simulations. One that is an approximation of the server simulation, that also
lags behind the present time, and uses inputs from players to update as best as it can 
(maybe loding out very far entities in the sector, which means this sim will have errors).

But this is still a 'single time' simulation', that needs player inputs before updating. So it lags.

We hide this lag with a second simulation, which is predicted up to the present with local inputs.
A predicted simulation is restarted from the real simulation every frame, for a couple of large dts that are not deterministic 
and advance just a small subset of the entities, including the local players and their immediate surrounding entities.
The time of each entity depends on distance from local players. So we have a relativistic predicted state.



Anyways. This is in my head now. It should move into this github repo in the following months.

Using: vulkan, glfw, glm, imgui, spdlog, pods, rapidjson, kissnet, magic_enum, stb_image, tiny_obj_loader.




