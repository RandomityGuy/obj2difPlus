#include <iostream>
#include <DIFBuilder/DIFBuilder.hpp>
#include <tiny_obj_loader.h>
#define GLM_FORCE_INTRINSICS
#include <glm\glm.hpp>
#include <dif\objects\dif.h>
#include <dif\base\io.h>
#include <chrono>

int main(int argc, const char **argv) 
{
	printf("obj2dif 1.2.3\n");
	printf("originally by HiGuy, modifications by RandomityGuy\n");

	if (argc > 1)
	{
		bool flipNormals = false;
		bool doublesidedfaces = false;
		bool splitbyaxis = false;
		for (int i = 1; i < argc; i++)
		{
			const char* arg = argv[i];

			if (strcmp(arg, "-flip") == 0)
				flipNormals = true;

			if (strcmp(arg, "-double") == 0)
				doublesidedfaces = true;
		}


		printf("Loading obj file\n");
		//Read everything we can
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		tinyobj::LoadObj(&attrib, &shapes, &materials, &err, argv[1]);

		printf(err.c_str());

		//Default material
		materials.push_back(tinyobj::material_t());

		std::vector<DIF::DIFBuilder*> builders;
		DIF::DIFBuilder* builder = new DIF::DIFBuilder();
		builders.push_back(builder);
		int tricount = 0;
		int alltris = 0;
		for (const tinyobj::shape_t shape : shapes) {

			int vertStart = 0;
			if (tricount > 16000) //Max BSP Node limit: 32767, max BSP Leaf limit: 16383, hence max polygons = 16383
			{
				tricount = 0;
				builder = new DIF::DIFBuilder();
				builders.push_back(builder);
			}
			for (int i = 0; i < shape.mesh.num_face_vertices.size(); i++) {

				if (tricount > 16000) //Max BSP Node limit: 32767, max BSP Leaf limit: 16383, hence max polygons = 16383
				{
					tricount = 0;
					builder = new DIF::DIFBuilder();
					builders.push_back(builder);
				}

				tinyobj::index_t idx[3] = {
						shape.mesh.indices[vertStart + 2],
						shape.mesh.indices[vertStart + 1],
						shape.mesh.indices[vertStart + 0]
				};

				DIF::DIFBuilder::Triangle triangle;

				DIF::DIFBuilder::Triangle invertedTriangle;

				for (int j = 0; j < 3; j++) {
					triangle.points[j].vertex = glm::vec3(
						attrib.vertices[(idx[j].vertex_index * 3) + 0],
						-attrib.vertices[(idx[j].vertex_index * 3) + 2],
						attrib.vertices[(idx[j].vertex_index * 3) + 1]
					);
					triangle.points[j].uv = glm::vec2(
						attrib.texcoords[(idx[j].texcoord_index * 2) + 0],
						-attrib.texcoords[(idx[j].texcoord_index * 2) + 1]
					);


					if (attrib.normals.size() != 0)
					triangle.points[j].normal = glm::vec3(
						attrib.normals[(idx[j].normal_index * 3) + 0],
						-attrib.normals[(idx[j].normal_index * 3) + 2],
						attrib.normals[(idx[j].normal_index * 3) + 1]
					);

					if (doublesidedfaces)
					{
						invertedTriangle.points[j].vertex = glm::vec3(
							attrib.vertices[(idx[j].vertex_index * 3) + 1],
							-attrib.vertices[(idx[j].vertex_index * 3) + 2],
							attrib.vertices[(idx[j].vertex_index * 3) + 0]
						);
						invertedTriangle.points[j].uv = glm::vec2(
							attrib.texcoords[(idx[j].texcoord_index * 2) + 0],
							-attrib.texcoords[(idx[j].texcoord_index * 2) + 1]
						);


						if (attrib.normals.size() != 0)
							invertedTriangle.points[j].normal = glm::vec3(
								-attrib.normals[(idx[j].normal_index * 3) + 0],
								attrib.normals[(idx[j].normal_index * 3) + 2],
								-attrib.normals[(idx[j].normal_index * 3) + 1]
							);
					}

				}

				int material = shape.mesh.material_ids[i];
				tricount++;
				alltris++;
				builder->addTriangle(triangle, (material == -1 ? shape.name : materials[material].diffuse_texname.substr(0, materials[material].diffuse_texname.length() - 4)));
				if (doublesidedfaces)
				{
					tricount++;
					alltris++;
					builder->addTriangle(invertedTriangle, (material == -1 ? shape.name : materials[material].diffuse_texname.substr(0, materials[material].diffuse_texname.length() - 4)));
				}
				//builder.addTriangle(invertedTriangle, (material == -1 ? shape.name : materials[material].name));

				vertStart += 3;
			}

		}

		printf("Building DIFs for %d triangles\n",alltris);
		int index = 0;
		//long totduration = 0;
		for (auto& difbuilder : builders)
		{
			printf("Building DIF %d/%d\n", index + 1, builders.size());

			//auto start = std::chrono::high_resolution_clock::now();

			DIF::DIF dif;
			difbuilder->build(dif, flipNormals);

			//auto finish = std::chrono::high_resolution_clock::now();

			//long duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
			//totduration += duration;

			char buf[16];
			std::ofstream outStr;
			outStr.open(std::string(argv[1]).substr(0, strlen(argv[1]) - 4) + std::string(itoa(index,buf,10)) + ".dif", std::ios::out | std::ios::binary);
			dif.write(outStr, DIF::Version());
			index++;
		}
		//printf("Time Taken:%d\n", totduration);
		//getchar();



	}
	else
	{
		printf("Usage:\n");
		printf("obj2dif <file> [-flip] [-double]\n");
		printf("file: path to the obj file to convert\n");
		printf("flip: (optional) flip normals\n");
		printf("double: (optional) make all faces double sided\n");
	}
	return 0;
}
