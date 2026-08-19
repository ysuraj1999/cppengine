#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <cfloat>
#include <cstddef>
#include <glm/glm.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "stb_image.h"
#include "types.hpp"
#include "obj.hpp"
#include "utils.hpp"

bool renderer_init(RendererConfig config);
bool scene_load_obj(Scene *scene, const char *path);
MeshHandle scene_add_mesh(Scene *scene, Mesh m);
bool scene_remove_mesh(Scene *scene, MeshHandle mh);
InstanceHandle scene_add_instance(Scene *scene, MeshHandle m, Transform t);
bool scene_remove_instance(Scene *scene, InstanceHandle i);
bool renderer_sync_scene(RendererState *rendererState, const Scene *scene);
void renderer_render(RendererState *rendererState);
bool renderer_clear(RendererState *rendererState);
void renderer_quit();
