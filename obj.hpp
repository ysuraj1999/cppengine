#pragma once
#include <iostream>
#include "types.hpp"

// tinyobj loader header-only interface (no implementation)
#include "tiny_obj_loader.h"

std::vector<Mesh> loadAllMeshFromObj(const char *inputfile);
