#include <include/ecs_solar.h>

typedef struct Orbit {
    float size, t, v;
    bool hide_orbit;
} Orbit;

typedef struct Rings {
    float radius;
    float width;
} Rings;

/* Create orbit circle, and set color based on orbit size */
void InitOrbit(ecs_rows_t *rows) {
    /* Obtain the orbit column */
    ECS_COLUMN(rows, Orbit, orbit, 1);

    /* Obtain components from ID columns, so they can be used with ecs_set */
    ECS_COLUMN_COMPONENT(rows, EcsCircle, 2);
    ECS_COLUMN_COMPONENT(rows, EcsPosition2D, 3);
    ECS_COLUMN_COMPONENT(rows, EcsLineColor, 4);
    ECS_COLUMN_COMPONENT(rows, EcsColor, 5);

    /* Obtain a container that has the Orbit component */
    ecs_entity_t EOrbit = ecs_column_entity(rows, 1);
    ecs_entity_t parent = ecs_get_parent(rows->world, rows->entities[0], EOrbit);

    for (int i = 0; i < rows->count; i ++) {
        float r = 255 - (orbit[i].size * 3);
        ecs_clamp(&r, 0, 255);
        
        /* Create the orbit entity */
        if (!orbit[i].hide_orbit) {
            ecs_entity_t e = ecs_set(rows->world, 0, EcsCircle, {.radius = orbit[i].size});
            ecs_set(rows->world, e, EcsPosition2D, {0, 0});
            ecs_set(rows->world, e, EcsLineColor, {.r = r, .g = 50, .b = 255, .a = 150});

            /* Add orbit to the parent, if there is one */
            if (parent) {
                ecs_adopt(rows->world, e, parent);
            }
        }

        /* Set the color for the orbit and satellite */
        ecs_set(rows->world, rows->entities[i], EcsColor, {.r = r, .g = 50, .b = 255, .a = 255});
    }   
}

/* Initialize rings */
void InitRings(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Rings, rings, 1);
    ECS_COLUMN_COMPONENT(rows, EcsCircle, 2);
    ECS_COLUMN_COMPONENT(rows, EcsPosition2D, 3);
    ECS_COLUMN_COMPONENT(rows, EcsLineColor, 4);

    for (int i = 0; i < rows->count; i ++) {
        float w = rings[i].width;
        float rad = rings[i].radius;

        for (int r = 0; r < w; r ++) {
            ecs_entity_t ring = ecs_set(rows->world, 0, EcsCircle, {.radius = rad + (float)r / 2.0});
            ecs_set(rows->world, ring, EcsPosition2D, {0, 0});
            if (r < 15) {
                ecs_set(rows->world, ring, EcsLineColor, {0, 0, 255, 100});
            } else if (r < 18) {
                ecs_set(rows->world, ring, EcsLineColor, {0, 50, 255, 150});
            } else if (r < 30) {
                ecs_set(rows->world, ring, EcsLineColor, {0, 0, 255, 40});
            } else {
                ecs_set(rows->world, ring, EcsLineColor, {0, 50, 255, 60});
            }
            ecs_adopt(rows->world, ring, rows->entities[i]);
        }
    }
}

/* Initialize asteroid */
void InitAsteroid(ecs_rows_t *rows) {
    ECS_COLUMN(rows, EcsPolygon8, polygon, 1);
    ECS_COLUMN(rows, Orbit, orbit, 2);

    for (int i = 0; i < rows->count; i ++) {
        /* Create random asteroid shape */
        int v;
        float step = 2 * M_PI / 7.0;
        for (v = 0; v < 7; v ++) {
            uint8_t radius = 2 + rand() % 7;
            polygon[i].points[v].x = cos((float)v * step) * (float)radius;
            polygon[i].points[v].y = sin((float)v * step) * (float)radius;
        }
        polygon[i].points[v] = polygon[i].points[0];
        polygon[i].point_count = 8;

        /* Initialize orbit */
        orbit[i].size = 260 + rand() % 25;
        orbit[i].t = ((float)rand() / (float)RAND_MAX) * 2 * M_PI;
        orbit[i].v = ((float)rand() / (float)RAND_MAX) / 30.0;
        orbit[i].hide_orbit = true;
    }
}

/* Progress orbit proportionally to delta_time */
void ProgressOrbit (ecs_rows_t *rows) {
    ECS_COLUMN(rows, Orbit, orbit, 1);
    ECS_COLUMN(rows, EcsPosition2D, position, 2);

    for (int i = 0; i < rows->count; i ++) {
        orbit[i].t += orbit[i].v * rows->delta_time;
        position[i].x = cos(orbit[i].t) * orbit[i].size;
        position[i].y = sin(orbit[i].t) * orbit[i].size;
    }
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init_w_args(argc, argv);

    ECS_IMPORT(world, EcsComponentsTransform, ECS_2D);
    ECS_IMPORT(world, EcsComponentsPhysics, ECS_2D);
    ECS_IMPORT(world, EcsComponentsGeometry, ECS_2D);
    ECS_IMPORT(world, EcsComponentsGraphics, ECS_2D);
    ECS_IMPORT(world, EcsComponentsInput, ECS_2D);
    ECS_IMPORT(world, EcsSystemsPhysics, ECS_2D);
    ECS_IMPORT(world, EcsSystemsSdl2, ECS_2D);

    ECS_COMPONENT(world, Orbit);
    ECS_COMPONENT(world, Rings);

    /* Create a prefab that shared the EcsCircle component with satellites */
    ECS_PREFAB(world, PlanetPrefab, EcsCircle);
    ECS_PREFAB(world, MoonPrefab, EcsCircle);
    ECS_PREFAB(world, AsteroidPrefab, EcsAngularSpeed, EcsColor);
    ECS_TYPE(world, Asteroid, AsteroidPrefab, EcsPosition2D, EcsRotation2D, EcsPolygon8, Orbit);
    ECS_TYPE(world, Planet, PlanetPrefab, EcsPosition2D, Orbit);
    ECS_TYPE(world, Moon, MoonPrefab, EcsPosition2D, Orbit);
    ecs_set(world, PlanetPrefab, EcsCircle, {.radius = 10});
    ecs_set(world, MoonPrefab, EcsCircle, {.radius = 5});
    ecs_set(world, AsteroidPrefab, EcsAngularSpeed, {1});
    ecs_set(world, AsteroidPrefab, EcsColor, {100, 100, 100, 100});

    /* System that sets color of planet & adds a ring for the orbit */
    ECS_SYSTEM(world, InitOrbit, EcsOnSet, Orbit, ID.EcsCircle, ID.EcsPosition2D, ID.EcsLineColor, ID.EcsColor);
    ECS_SYSTEM(world, InitRings, EcsOnSet, Rings, ID.EcsCircle, ID.EcsPosition2D, ID.EcsLineColor);
    ECS_SYSTEM(world, InitAsteroid, EcsOnAdd, EcsPolygon8, Orbit);

    /* System that progresses the planet along an orbit */
    ECS_SYSTEM(world, ProgressOrbit, EcsOnUpdate, Orbit, EcsPosition2D);

    ecs_set_singleton(world, EcsCanvas2D, {
        .window = { .width = 1024, .height = 800 }, .title = "Hello ecs_solar!" 
    });

    /* Create asteroids */
    ecs_new_w_count(world, Asteroid, 500);

    /* Create sun, planets, moons */
    ecs_entity_t sun = ecs_new(world, 0);
    ecs_set(world, sun, EcsCircle, {.radius = 25});
    ecs_set(world, sun, EcsPosition2D, {0, 0});
    ecs_set(world, sun, EcsColor, {.r = 255, .g = 255, .b = 160, .a = 255});

    ecs_entity_t m, p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {350, 0, .v = 0.2});

        for (int i = 0; i < 6; i ++) {
            m = ecs_new_child(world, p, Moon);
            ecs_set(world, m, Orbit, {20 + i * 7, rand() % 10, .v = 5 - i / 2.0}); 
        }

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {420, M_PI, .v = 0.2});
    ecs_set(world, p, Rings, {.width = 40, .radius = 22});

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {220, 0, .v = 0.5});

        m = ecs_new_child(world, p, Moon);
        ecs_set(world, m, Orbit, {20, 0, .v = 4});   

        m = ecs_new_child(world, p, Moon);
        ecs_set(world, m, Orbit, {30, 0, .v = 7});  

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {150, 0, .v = 1});

        m = ecs_new_child(world, p, Moon);
        ecs_set(world, m, Orbit, {35, 0, .v = 4});   

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {100, 0, .v = 2});     

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {50, 0, .v = 3});    

    /* Enter main loop */
    ecs_set_target_fps(world, 120);

    while ( ecs_progress(world, 0)) ;

    /* Cleanup */
    return ecs_fini(world);
}
