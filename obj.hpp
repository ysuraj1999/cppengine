#include <iostream>
#include "types.hpp"

#define TINYOBJLOADER_IMPLEMENTATION // needed for tinyobj
#define TINYOBJLOADER_USE_MULTITHREADING  // multi-threaded parsing
#define TINYOBJLOADER_USE_SIMD            // SIMD (SSE2/AVX2/NEON) newline scan
#include "tiny_obj_loader.h"



// Only load the first shape
void loadObjIntoScene(const char* inputfile, Scene &scene) {
  std::string warn, err;
  
  tinyobj::ObjReaderConfig reader_config;
  reader_config.vertex_color = true;
  reader_config.triangulate = true;

  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(inputfile, reader_config)) {
    if (!reader.Error().empty()) {
      std::cerr << "TinyObjReader: " << reader.Error();
    }
    exit(1);
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader: " << reader.Warning();
  }

  auto& attrib = reader.GetAttrib();
  auto &shapes = reader.GetShapes();


  
  for (u32 shapeIndex = 0; shapeIndex < shapes.size(); shapeIndex++) {
    auto &shape = shapes[shapeIndex];

    Mesh mesh{};
    std::map<std::tuple<int, int, int>, u32> vertexMap;
    u32 indexOffset = 0;
    for (u32 faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size();
         faceIndex++) {
      
      // Always 3 since triangulation enabled
      // const u32 vertCountForFace = u32(shape.mesh.num_face_vertices[faceIndex]);

      Vertex vertexStorage{};
      for (u32 vertIndex = 0; vertIndex < 3; vertIndex++) {
        auto idx = shape.mesh.indices[indexOffset + vertIndex];

        tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
	tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
        tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

        vertexStorage.position = {vx, vy, vz};

        if (idx.normal_index >= 0) {
          tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
	  tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
          tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

          vertexStorage.normal = {nx, ny, nz};
        }

        if (idx.texcoord_index >= 0) {
          tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
          tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];

          vertexStorage.uv = {tx, ty};
        }

        if (!attrib.colors.empty()){
          tinyobj::real_t red = attrib.colors[3 * idx.vertex_index + 0];
          tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
          tinyobj::real_t blue = attrib.colors[3 * idx.vertex_index + 2];
          vertexStorage.color = {red, green, blue, 1.0f};
        } else {
          vertexStorage.color = glm::vec4(1.0f);
        }

	auto key = std::make_tuple(
				   idx.vertex_index,
				   idx.texcoord_index,
				   idx.normal_index
				   );

	auto [it, inserted] =
	  vertexMap.emplace(key, mesh.vertices.size());

	if (inserted) {
	  mesh.vertices.push_back(vertexStorage);
	}

	mesh.indices.push_back(it->second);
      }

      indexOffset += 3;
    }

    scene.meshes.push_back(mesh);
  }
}
