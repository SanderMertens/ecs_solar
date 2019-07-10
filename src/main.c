#include <ecs_solar.h>

/* Utility type */
typedef struct randomized_t {
    float value;
    float variance;
} randomized_t;

/* Components */
typedef struct Orbit {
    float size, t, v;
    bool hide_orbit;
} Orbit;

typedef struct SunParticleSystem {
    float radius;
    float speed;
    float particle_size;
    float alpha;
    float fade_radius;
} SunParticleSystem;

typedef struct SunParticle {
    float t;
    float r;
    float a;
} SunParticle;

typedef struct AsteroidParticleSystem {
    randomized_t radius;
    randomized_t speed;
    randomized_t orbit_radius;
    randomized_t brightness;
    float alpha;
} AsteroidParticleSystem;

typedef struct StarParticleSystem {
    uint32_t width;
    uint32_t height;
} StarParticleSystem;

/* Utility functions */
float randomize(randomized_t r) {
    return r.value + ecs_randf(r.variance);
}

/* Create orbit circle, and set color based on orbit size */
void InitOrbit(ecs_rows_t *rows) {
    /* Obtain the orbit column */
    ECS_COLUMN(rows, Orbit, orbit, 1);

    /* Obtain components from ID columns, so they can be used with ecs_set */
    ECS_COLUMN_COMPONENT(rows, EcsCircle, 2);
    ECS_COLUMN_COMPONENT(rows, EcsPosition2D, 3);
    ECS_COLUMN_COMPONENT(rows, EcsLineColor, 4);

    /* Obtain a container that has the Orbit component */
    ecs_entity_t EOrbit = ecs_column_entity(rows, 1);
    ecs_entity_t parent = ecs_get_parent(rows->world, rows->entities[0], EOrbit);

    for (int i = 0; i < rows->count; i ++) {
        /* Create the orbit entity */
        if (!orbit[i].hide_orbit) {
            ecs_entity_t e = ecs_set(rows->world, 0, EcsCircle, {.radius = orbit[i].size});
            ecs_set(rows->world, e, EcsPosition2D, {0, 0});
            ecs_set(rows->world, e, EcsLineColor, {.r = 50, .g = 150, .b = 255, .a = 150});

            /* Add orbit to the parent, if there is one */
            if (parent) {
                ecs_adopt(rows->world, e, parent);
            }
        }
    }
}

/* Initialize sun particles */
void InitSunParticle(ecs_rows_t *rows) {
    ECS_COLUMN(rows, SunParticle, particle, 1);
    ECS_COLUMN(rows, EcsColor, color, 2);
    ECS_COLUMN(rows, EcsCircle, shape, 3);
    ECS_COLUMN(rows, SunParticleSystem, particle_system, 4);

    for (int i = 0; i < rows->count; i ++) {
        float r = ecs_randf(particle_system->radius);
        float a = ecs_randf(2.0 * M_PI);

        color[i].r = ecs_randf(55) + 200;
        color[i].g = ecs_randf(200) + 50;
        color[i].b = 0;
        color[i].a = 0;

        ecs_clamp(&color[i].g, 100, color[i].r);

        shape[i].radius = particle_system->particle_size;

        particle[i].t = r / particle_system->radius;
        particle[i].r = r;
        particle[i].a = a;
    }
}

/* Initialize asteroid */
void InitAsteroidParticle(ecs_rows_t *rows) {
    ECS_COLUMN(rows, EcsPolygon8, polygon, 1);
    ECS_COLUMN(rows, Orbit, orbit, 2);
    ECS_COLUMN(rows, EcsColor, color, 3);
    ECS_COLUMN(rows, AsteroidParticleSystem, particle_system, 4);

    for (int i = 0; i < rows->count; i ++) {
        /* Create random asteroid shape */
        int v;
        float step = 2.0 * M_PI / 7.0;
        for (v = 0; v < 7; v ++) {
            float radius = randomize(particle_system->radius);
            polygon[i].points[v].x = cos((float)v * step) * radius;
            polygon[i].points[v].y = sin((float)v * step) * radius;
        }
        polygon[i].points[v] = polygon[i].points[0];
        polygon[i].point_count = 8;

        /* Initialize orbit */
        orbit[i].size = randomize(particle_system->orbit_radius);
        orbit[i].t = ecs_randf(2.0 * M_PI);
        orbit[i].v = randomize(particle_system->speed);
        orbit[i].hide_orbit = true;

        /* Initialize color */
        float b = randomize(particle_system->brightness);
        color[i] = (EcsColor){b, b, b, .a = particle_system->alpha};
    }
}

/* Progress orbit */
void ProgressOrbit (ecs_rows_t *rows) {
    ECS_COLUMN(rows, Orbit, orbit, 1);
    ECS_COLUMN(rows, EcsPosition2D, position, 2);

    for (int i = 0; i < rows->count; i ++) {
        /* Simple system that keeps things moving in circles */
        orbit[i].t += orbit[i].v * rows->delta_time;
        position[i].x = cos(orbit[i].t) * orbit[i].size;
        position[i].y = sin(orbit[i].t) * orbit[i].size;
    }
}

/* Progress sun particles */
void ProgressSun (ecs_rows_t *rows) {
    ECS_COLUMN(rows, SunParticle, particle, 1);
    ECS_COLUMN(rows, EcsPosition2D, p, 2);
    ECS_COLUMN(rows, EcsColor, color, 3);
    ECS_COLUMN(rows, SunParticleSystem, particle_system, 4);
    ecs_entity_t parent = ecs_column_source(rows, 4);

    int delete_count = 0;
    ecs_type_t TParticleType = 0;

    for (int i = 0; i < rows->count; i ++) {
        /* Push the particle outwards */
        particle[i].r += particle_system->speed * rows->delta_time;

        /* Derive position of particle from angle and current radius */
        p[i].x = cos(particle[i].a) * particle[i].r;
        p[i].y = sin(particle[i].a) * particle[i].r;

        /* If r > fade radius, start fading out the particle */
        float fade_r = particle_system->fade_radius;
        if (particle[i].r > fade_r) {
            float r = particle_system->radius;
            float r_diff = r - fade_r;
            float fade_factor = ((r_diff - (particle[i].r - fade_r)) / r_diff);

            /* If particle is fully faded, delete it */
            if (fade_factor <= 0) {
                TParticleType = ecs_get_type(rows->world, rows->entities[i]);
                ecs_delete(rows->world, rows->entities[i]);

                /* Keep track of deleted particles so we can recreate them */
                delete_count ++;
            } else {
                /* Set alpha to new value */
                float new_value = fade_factor * particle_system->alpha;
                if (color[i].a > new_value) {
                    color[i].a = new_value;
                }
            }
        } else if (color[i].a < particle_system->alpha) {
            /* Particles start at alpha 0, gradually so fade in */            
            color[i].a += 1;
        }
    }

    /* Recreate deleted particles */
    if (delete_count) {
        ecs_new_child_w_count(rows->world, parent, ParticleType, delete_count);
    }
}

/* Utility function for creating planets and moons */
void create_planet(
    ecs_world_t *world, ecs_type_t TOrbit,
    ecs_type_t TPlanet, 
    ecs_type_t TMoon, 
    float orbit_radius, 
    float t,
    float speed, 
    uint8_t moons)
{
    ecs_entity_t m, p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {orbit_radius, t, .v = speed});

    /* Create moons as children of the parent, so transforms are applied
     * hierarchically */
    for (int i = 0; i < moons; i ++) {
        m = ecs_new_child(world, p, Moon);
        ecs_set(world, m, Orbit, {20 + i * 7, rand() % 10, .v = 5 - i / 2.0});
    }
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init_w_args(argc, argv);

    ECS_IMPORT(world, FlecsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, FlecsComponentsPhysics, ECS_2D);
    ECS_IMPORT(world, FlecsComponentsGeometry, ECS_2D);
    ECS_IMPORT(world, FlecsComponentsGraphics, ECS_2D);
    ECS_IMPORT(world, FlecsComponentsInput, ECS_2D);
    ECS_IMPORT(world, FlecsSystemsPhysics, ECS_2D);
    ECS_IMPORT(world, FlecsSystemsSdl2, ECS_2D);

    ECS_COMPONENT(world, Orbit);
    ECS_COMPONENT(world, SunParticleSystem);
    ECS_COMPONENT(world, SunParticle);
    ECS_COMPONENT(world, AsteroidParticleSystem);

    /* Create a prefabs and types for planets, moons and particles */
    ECS_PREFAB(world, PlanetPrefab, EcsCircle, EcsColor);
    ECS_PREFAB(world, MoonPrefab, EcsCircle, EcsColor);
    ECS_PREFAB(world, AsteroidPrefab, EcsAngularSpeed);
    ECS_TYPE(world, SunParticleType, SunParticle, EcsPosition2D, EcsColor, EcsCircle);
    ECS_TYPE(world, AsteroidParticleType, INSTANCEOF | AsteroidPrefab, EcsPosition2D, EcsRotation2D, EcsPolygon8, Orbit, EcsColor);
    ECS_TYPE(world, Planet, INSTANCEOF | PlanetPrefab, EcsPosition2D, Orbit);
    ECS_TYPE(world, Moon, INSTANCEOF | MoonPrefab, EcsPosition2D, Orbit);
    
    /* Initialize prefabs */
    ecs_set(world, PlanetPrefab, EcsCircle, {.radius = 10});
    ecs_set(world, PlanetPrefab, EcsColor, {180, 250, 200, 255});
    ecs_set(world, MoonPrefab, EcsCircle, {.radius = 5});
    ecs_set(world, MoonPrefab, EcsColor, {75, 125, 125, 255});
    ecs_set(world, AsteroidPrefab, EcsAngularSpeed, {1});

    /* Entities for particle systems */
    ECS_ENTITY(world, Sun, SunParticleSystem);
    ECS_ENTITY(world, Asteroids, AsteroidParticleSystem);

    /* System that sets color of planet & adds a ring for the orbit */
    ECS_SYSTEM(world, InitOrbit, EcsOnSet, Orbit, .EcsCircle, .EcsPosition2D, .EcsLineColor);

    /* Systems that initialize particles for particle systems */
    ECS_SYSTEM(world, InitSunParticle,      EcsOnAdd, SunParticle, EcsColor, EcsCircle, Sun.SunParticleSystem);
    ECS_SYSTEM(world, InitAsteroidParticle, EcsOnAdd, EcsPolygon8, Orbit, EcsColor, Asteroids.AsteroidParticleSystem);

    /* System that progresses the planet along an orbit */
    ECS_SYSTEM(world, ProgressOrbit, EcsOnUpdate, Orbit, EcsPosition2D);
    ECS_SYSTEM(world, ProgressSun,   EcsOnUpdate, SunParticle, EcsPosition2D, EcsColor, CONTAINER.SunParticleSystem);

    /* Create sun */
    ecs_set(world, Sun, EcsPosition2D, {0, 0});
    ecs_set(world, Sun, EcsCircle, {.radius = 25});
    ecs_set(world, Sun, EcsColor, {.r = 255, .g = 255, .b = 255, .a = 255});

    /* Create sun particle system */    
    ecs_set(world, Sun, SunParticleSystem, {
        .radius = 30, 
        .speed = 15, 
        .particle_size = 4, 
        .alpha = 20, 
        .fade_radius = 20
    });

    ecs_new_child_w_count(world, Sun, SunParticleType, 1000);

    /* Create asteroid particle system */
    ecs_set(world, Asteroids, AsteroidParticleSystem, {
        .radius =       {.value = 2,    .variance = 8},
        .speed =        {.value = 0.01, .variance = 0.01},
        .orbit_radius = {.value = 260,  .variance = 25},
        .brightness =   {.value = 50,   .variance = 150},
        .alpha = 200
    });

    ecs_new_child_w_count(world, Asteroids, AsteroidParticleType, 250);

    /* Create planets and moons */
    create_planet(world, TOrbit, TPlanet, TMoon, 420, 0,    0.2, 6);
    create_planet(world, TOrbit, TPlanet, TMoon, 350, M_PI, 0.2, 4);
    create_planet(world, TOrbit, TPlanet, TMoon, 220, 0,    0.5, 2);
    create_planet(world, TOrbit, TPlanet, TMoon, 150, 0,    1,   1);
    create_planet(world, TOrbit, TPlanet, TMoon, 100, 0,    2,   0);
    create_planet(world, TOrbit, TPlanet, TMoon, 60,  0,    3,   0);

    /* Create drawing canvas */
    ecs_set_singleton(world, EcsCanvas2D, {
        .window = { .width = 800, .height = 800 }, .title = "Hello ecs_solar!" 
    });

    /* Enter main loop */
    ecs_set_target_fps(world, 60);

    while ( ecs_progress(world, 0)) ;

    /* Cleanup */
    return ecs_fini(world);
}
