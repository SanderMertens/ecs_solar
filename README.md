# ecs_solar
This is an example that demonstrates how to use hierarchies in Flecs (https://github.com/SanderMertens/flecs), and how they are used by modules to apply hierarchical transformations. 

## Getting Started
To run this demo, you need the bake build system (https://github.com/SanderMertens/bake). To install bake on Linux and MacOS, do:

```
git clone https://github.com/SanderMertens/bake
make -C bake/build-$(uname)
bake/bake setup
```

Before you can run the demo, make sure to have SDL2 installed.

On MacOS:

```
brew install sdl2
```

On Debian/Ubuntu:

```
sudo apt-get install sdl2
```

To install and run the demo, do:

```
bake clone SanderMertens/ecs_solar
bake run ecs_solar
```

## Implementation
This demo uses Flecs hierarchies to render planets orbiting a sun, and moons orbiting a planet. To make sure the positions of the moons are relative to their planet, they are added as a child to the planet. In the code, this looks like this:

```c
ecs_entity_t p = ecs_new(world, Planet);
ecs_entity_t m = ecs_new_child(world, p, Moon);
```

The `ecs_new_child` function is similar to `ecs_new`, but in addition it turns the entity `p` in a container, and adds `p` to `m`, which makes `m` a child of `p`. Containers have well-defined semantics in Flecs, and as a result Flecs modules can leverage this information to treat entities properly according to their location in the hierarchy. In this demo, systems from the [flecs.systems.transform](https://github.com/SanderMertens/flecs-systems-transform) module use the hierarchy to compute hierarchical transforms.

The key enabling feature for the hierarchical transform is the [CASCADE modifier](https://github.com/SanderMertens/flecs/blob/master/Manual.md#cascade-modifier). This is a special kind of column modifier that guarantees that a system will iterate over entities in the order of the hierarchy. Internally Flecs implements this by ordering the archetypes based on their container "depth": the deeper in the hierarchy an archetype is, the later it will be iterated over. This ordering is very efficient: instead of having to order individual entities, only the archetypes need to be ordered, and this typically only needs to happen once.

### Creating the orbit
The demo uses an `EcsOnSet` system to automatically add the orbit circle as a child to any entity that has the `Orbit` component. In the `InitOrbit` system, the code first obtains a container that has the `Orbit` component, which is the parent for the orbit. For planets, the parent will be 0 (the sun is not a parent), and for moons, the parent will be the planet. This ensures that the orbit moves relative to its parent sattelite.

### Shared geometry data
The demo uses prefabs to share geometry across planets and moons. This saves memory (albeit very little) and saves the demo from having to specify the value of the geometry for each satellite. The code defines a `PlanetPrefab` and a `MoonPrefab` prefab with an `EcsCircle` component, each set to its corresponding radius. These prefabs are then used in a `Planet` and `Moon` type, along with other components common to satellites.
