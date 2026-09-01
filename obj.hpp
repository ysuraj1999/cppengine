#pragma once
#include "types.hpp"
#include <iostream>

// tinyobj loader header-only interface (no implementation)
#include "tiny_obj_loader.h"

std::vector<Mesh> loadAllMeshFromObj(const char *inputfile);
