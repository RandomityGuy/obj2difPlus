#include <iostream>
#include <DIFBuilder/DIFBuilder.hpp>
#include <tiny_obj_loader.h>
#define GLM_FORCE_INTRINSICS
#include <glm\glm.hpp>
#include <dif\objects\dif.h>
#include <dif\base\io.h>

int main(int argc, const char **argv) 
{
	printf("obj2dif 1.2\n");
	printf("originally by HiGuy, modifications by RandomityGuy\n");

	if (argc > 1)
	{
		bool flipNormals = false;
		for (int i = 1; i < argc; i++)
		{
			const char* arg = argv[i];

			if (strcmp(arg, "-flip") == 0)
				flipNormals = true;


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
		for (const tinyobj::shape_t shape : shapes) {

			int vertStart = 0;
			if (tricount > 16000) //Max limit of BSP Nodes is 8192 because after that, the next two bits are used for checking leaf indices 
			{
				tricount = 0;
				builder = new DIF::DIFBuilder();
				builders.push_back(builder);
			}
			for (int i = 0; i < shape.mesh.num_face_vertices.size(); i++) {

				if (tricount > 4000) //Max limit of BSP Nodes is 8192 because after that, the next two bits are used for checking leaf indices 
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

					triangle.points[j].normal = glm::vec3(
						attrib.normals[(idx[j].normal_index * 3) + 0],
						-attrib.normals[(idx[j].normal_index * 3) + 2],
						attrib.normals[(idx[j].normal_index * 3) + 1]
					);
				}

				int material = shape.mesh.material_ids[i];
				tricount++;
				builder->addTriangle(triangle, (material == -1 ? shape.name : materials[material].name));
				//builder.addTriangle(invertedTriangle, (material == -1 ? shape.name : materials[material].name));

				vertStart += 3;
			}

		}

		printf("Building DIFs\n");
		int index = 0;
		for (auto& difbuilder : builders)
		{
			DIF::DIF dif;
			difbuilder->build(dif, flipNormals);


			char buf[16];
			std::ofstream outStr;
			outStr.open(std::string(argv[1]).substr(0, strlen(argv[1]) - 4) + std::string(itoa(index,buf,10)) + ".dif", std::ios::out | std::ios::binary);
			dif.write(outStr, DIF::Version());
			index++;
		}



	}
	else
	{
		printf("Usage:\n");
		printf("obj2difPlus <file> [-flip]\n");
		printf("file: path to the obj file to convert\n");
		printf("flip: (optional) flip normals\n");
	}
	return 0;
}
