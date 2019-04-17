# ecs_solar
This is an example that demonstrates how to use hierarchies in Flecs (https://github.com/SanderMertens/flecs), how they are used by modules to apply hierarchical transformations, and a potential approach to implementing particle effects.

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

## Hierarchies
This demo uses Flecs hierarchies to render planets orbiting a sun, and moons orbiting a planet. To make sure the positions of the moons are relative to their planet, they are added as a child to the planet. In the code, this looks like this:

```c
ecs_entity_t p = ecs_new(world, Planet);
ecs_entity_t m = ecs_new_child(world, p, Moon);
```

The `ecs_new_child` function is similar to `ecs_new`, but in addition it turns the entity `p` in a container, and adds `p` to `m`, which makes `m` a child of `p`. Containers have well-defined semantics in Flecs, and as a result Flecs modules can leverage this information to treat entities properly according to their location in the hierarchy. In this demo, systems from the [flecs.systems.transform](https://github.com/SanderMertens/flecs-systems-transform) module use the hierarchy to compute hierarchical transforms.

The key enabling feature for the hierarchical transform is the [CASCADE modifier](https://github.com/SanderMertens/flecs/blob/master/Manual.md#cascade-modifier). This is a special kind of column modifier that guarantees that a system will iterate over entities in the order of the hierarchy. Internally Flecs implements this by ordering the archetypes based on their container "depth": the deeper in the hierarchy an archetype is, the later it will be iterated over. This ordering is very efficient: instead of having to order individual entities, only the archetypes need to be ordered, and this typically only needs to happen once.

### Creating the orbit
The demo uses an `EcsOnSet` system to automatically add the orbit circle as a child to any entity that has the `Orbit` component. In the `InitOrbit` system, the code first obtains a container that has the `Orbit` component, which is the parent for the orbit. For planets, the parent will be 0 (the sun is not a parent), and for moons, the parent will be the planet. This ensures that the orbit moves relative to its parent satellite.

### Particle effects
The demo contains a possible approach towards implementing particle effects. A particle effect in the application is defined by three key parts: a component that describes the parameters of the particle effect (`*ParticleSystem`), a system that initializes the values of a particle (`Init*Particle`), and a system that progresses a particle towards the next frame. An optional fourth part is another component that captures state of the particle specific to the particle effect. The particle effect is instantiated by first creating an entity which holds the particle effect parameters. Subsequently the particles are created with the `ecs_new_w_count` function.

The example contains two particle effects: an asteroid belt and a fire effect for the sun. The asteroid effect is created from the following building blocks:

```c
ECS_COMPONENT(world, AsteroidParticleSystem);
ECS_ENTITY(world, Asteroids, AsteroidParticleSystem);
ECS_PREFAB(world, AsteroidPrefab, EcsAngularSpeed);
ECS_TYPE(world, AsteroidParticleType, AsteroidPrefab, EcsPosition2D, EcsRotation2D, EcsPolygon8, Orbit, EcsColor);
ECS_SYSTEM(world, InitAsteroidParticle, EcsOnAdd, EcsPolygon8, Orbit, EcsColor, Asteroids.AsteroidParticleSystem);
```

The `AsteroidPrefab` contains an `EcsAngularMomentum` component which is shared with all asteroids (meaning all asteroids have the same rotation speed). The `AsteroidParticleType` combines the required components for the effect. No dedicated component for the particle system is required, all components are reused from either imports or the already existing `Orbit` component. The `InitAsteroidParticle` generates a random polygon, and initializes the asteroid's orbit and color from the parameters of the particle system. This effect does not have a dedicated system that moves the asteroids, as this functionality is "borrowed" from `ProgressOrbit` (the same one that is used for the planets and moons).

The fire particle effect is a bit more complex, and is composed out of the following building blocks:

```c
ECS_COMPONENT(world, SunParticleSystem);
ECS_COMPONENT(world, SunParticle);
ECS_TYPE(world, SunParticleType, SunParticle, EcsPosition2D, EcsColor, EcsCircle);
ECS_ENTITY(world, Sun, SunParticleSystem);
ECS_SYSTEM(world, InitSunParticle, EcsOnAdd, SunParticle, EcsColor, EcsCircle, Sun.SunParticleSystem);
ECS_SYSTEM(world, ProgressSun, EcsOnUpdate, SunParticle, EcsPosition2D, EcsColor, CONTAINER.SunParticleSystem);
```

This particle effect requires a dedicated component (`SunParticle`) to hold additional state related to the movement and color of the particle. Similar to the asteroid particle system, this system has a `SunParticleType` type to describe the components of a particle, and an `InitSunParticle` system to initialize the particle with its initial values. The `ProgressSun` system is a dedicated system that updates the movement and color of the particle.

Note that the `InitSunParticle` and `ProgressSun` systems use two different methods for refering to the parent particle system data, one through a direct reference to the `Sun` entity, and one by refering it as a container (this implies that the particles are created as children of the entity that holds the particle effect parameters.

Whereas the particles in the asteroid particle effect have an infinite lifespan, the particles in the sun particle system are short lived. Every time a particle dies (it reaches the edge of the sun) it will be instantly replaced by another in the `ProgressSun` system.

### Shared geometry data
The demo uses prefabs to share geometry across planets and moons. This saves memory (albeit very little) and saves the demo from having to specify the value of the geometry for each satellite. The code defines a `PlanetPrefab` and a `MoonPrefab` prefab with an `EcsCircle` component, each set to its corresponding radius. These prefabs are then used in a `Planet` and `Moon` type, along with other components common to satellites.
