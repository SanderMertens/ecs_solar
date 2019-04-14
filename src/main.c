#include <include/ecs_solar.h>

typedef struct Orbit {
    float size, t, v;
} Orbit;

void InitOrbit(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Orbit, orbit, 1);
    ECS_COLUMN_COMPONENT(rows, EcsCircle, 2);
    ECS_COLUMN_COMPONENT(rows, EcsPosition2D, 3);
    ECS_COLUMN_COMPONENT(rows, EcsLineColor, 4);
    ECS_COLUMN_COMPONENT(rows, EcsColor, 5);

    ecs_entity_t EOrbit = ecs_column_entity(rows, 1);
    ecs_entity_t parent = ecs_get_parent(rows->world, rows->entities[0], EOrbit);

    for (int i = 0; i < rows->count; i ++) {
        ecs_entity_t e = ecs_set(rows->world, 0, EcsCircle, {.radius = orbit[i].size});
        ecs_set(rows->world, e, EcsPosition2D, {0, 0});

        float r = 255 - (orbit[i].size);
        ecs_clamp(&r, 0, 255);
        ecs_set(rows->world, e, EcsLineColor, {.r = r, .g = 50, .b = 255, .a = 255});
        ecs_set(rows->world, rows->entities[i], EcsColor, {.r = r, .g = 50, .b = 255, .a = 255});

        if (parent) {
            ecs_adopt(rows->world, e, parent);
        }
    }   
}

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
    ECS_IMPORT(world, EcsComponentsGeometry, ECS_2D);
    ECS_IMPORT(world, EcsComponentsGraphics, ECS_2D);
    ECS_IMPORT(world, EcsComponentsInput, ECS_2D);
    ECS_IMPORT(world, EcsSystemsSdl2, ECS_2D);

    ECS_COMPONENT(world, Orbit);

    /* Create a prefab that shared the EcsCircle component with satellites */
    ECS_PREFAB(world, PlanetPrefab, EcsCircle);
    ECS_PREFAB(world, MoonPrefab, EcsCircle);
    ECS_TYPE(world, Planet, PlanetPrefab, EcsPosition2D, Orbit);
    ECS_TYPE(world, Moon, MoonPrefab, EcsPosition2D, Orbit);
    ecs_set(world, PlanetPrefab, EcsCircle, {.radius = 10});
    ecs_set(world, MoonPrefab, EcsCircle, {.radius = 5});

    /* System that sets color of planet & adds a ring for the orbit */
    ECS_SYSTEM(world, InitOrbit, EcsOnSet, Orbit, ID.EcsCircle, ID.EcsPosition2D, ID.EcsLineColor, ID.EcsColor);

    /* System that progresses the planet along an orbit */
    ECS_SYSTEM(world, ProgressOrbit, EcsOnUpdate, Orbit, EcsPosition2D);

    ecs_set_singleton(world, EcsCanvas2D, {
        .window = { .width = 800, .height = 600 }, .title = "Hello ecs_solar!" 
    });

    /* Create sun, planets, moons */
    ecs_entity_t sun = ecs_new(world, 0);
    ecs_set(world, sun, EcsCircle, {.radius = 25});
    ecs_set(world, sun, EcsPosition2D, {0, 0});
    ecs_set(world, sun, EcsColor, {.r = 255, .g = 255, .b = 160, .a = 255});

    ecs_entity_t p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {275, 0, .v = 0.5});

    ecs_entity_t m = ecs_new_child(world, p, Moon);
    ecs_set(world, m, Orbit, {40, 0, .v = 5});   

    m = ecs_new_child(world, p, Moon);
    ecs_set(world, m, Orbit, {25, 0, .v = 7});   

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {200, 0, .v = 1});

    m = ecs_new_child(world, p, Moon);
    ecs_set(world, m, Orbit, {35, 0, .v = 4});   

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {125, 0, .v = 2});

    p = ecs_new(world, Planet);
    ecs_set(world, p, Orbit, {60, 0, .v = 3});

    /* Enter main loop */
    ecs_set_target_fps(world, 120);

    while ( ecs_progress(world, 0)) ;

    /* Cleanup */
    return ecs_fini(world);
}
