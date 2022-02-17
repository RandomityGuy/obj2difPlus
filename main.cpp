#include <iostream>
#include <DIFBuilder/DIFBuilder.hpp>
#include <tiny_obj_loader.h>
#define GLM_FORCE_INTRINSICS
#include <glm/glm.hpp>
#include <dif/objects/dif.h>
#include <dif/base/io.h>
#include <chrono>

bool flipNormals = false;
bool doublesidedfaces = false;
bool splitbyaxis = false;
int splitcount = 12000;

std::vector<DIF::DIF> buildInteriors(const char* objpath, std::vector<DIF::Interior>* pathedInteriors = NULL)
{

	printf("Loading obj file\n");
	//Read everything we can
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objpath);

	printf(err.c_str());

	//Default material
	materials.push_back(tinyobj::material_t());

	std::vector<DIF::DIFBuilder*> builders;
	DIF::DIFBuilder* builder = new DIF::DIFBuilder();
	builders.push_back(builder);
	int tricount = 0;
	int alltris = 0;

	// Ok so we calculate the bounding box to offset all geometry to fix the weird origin thing

	glm::vec3 min;
	glm::vec3 max;

	for (const tinyobj::shape_t shape : shapes)
	{
		int vertStart = 0;
		for (int i = 0; i < shape.mesh.num_face_vertices.size(); i++)
		{
			tinyobj::index_t idx[3] = {
					shape.mesh.indices[vertStart + 2],
					shape.mesh.indices[vertStart + 1],
					shape.mesh.indices[vertStart + 0]
			};

			for (int j = 0; j < 3; j++) {
				glm::vec3 vertex = glm::vec3(
					attrib.vertices[(idx[j].vertex_index * 3) + 0],
					-attrib.vertices[(idx[j].vertex_index * 3) + 2],
					attrib.vertices[(idx[j].vertex_index * 3) + 1]
				);

				if (min.x > vertex.x)
					min.x = vertex.x;
				if (min.y > vertex.y)
					min.y = vertex.y;
				if (min.z > vertex.z)
					min.z = vertex.z;

				if (max.x < vertex.x)
					max.x = vertex.x;
				if (max.y < vertex.y)
					max.y = vertex.y;
				if (max.z < vertex.z)
					max.z = vertex.z;
			}

			vertStart += 3;
		}
	}

	glm::vec3 size = max - min;
	glm::vec3 off = glm::vec3(1, 1, 1);


	for (const tinyobj::shape_t shape : shapes) {

		int vertStart = 0;
		if (tricount > splitcount) //Max BSP Node limit: 32767, max BSP Leaf limit: 16383, hence max polygons = 16383
		{
			tricount = 0;
			builder = new DIF::DIFBuilder();
			builders.push_back(builder);
		}
		for (int i = 0; i < shape.mesh.num_face_vertices.size(); i++) {

			if (tricount > splitcount) //Max BSP Node limit: 32767, max BSP Leaf limit: 16383, hence max polygons = 16383
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
				triangle.points[j].vertex = size + off + glm::vec3(
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
					invertedTriangle.points[j].vertex = size + off + glm::vec3(
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

	printf("Building DIFs for %d triangles\n", alltris);

	std::vector<DIF::DIF> interiors;
	int index = 0;
	for (auto& difbuilder : builders)
	{
		printf("Building DIF %d/%d\n", index + 1, builders.size());

		DIF::DIF dif;
		if (index == 0)
		{
			if (pathedInteriors != NULL)
			{
				for (int i = 0; i < pathedInteriors->size(); i++)
				{
					difbuilder->addPathedInterior(pathedInteriors->at(i), std::vector<DIF::DIFBuilder::Marker>());
				}
			}
		}
		difbuilder->build(dif, flipNormals);
		interiors.push_back(dif);
		index++;
		delete difbuilder;
	}
	return interiors;
}

int main(int argc, const char **argv) 
{
	printf("obj2difplus 1.2.7\n");
	printf("originally by HiGuy, rewrite by RandomityGuy\n");

	if (argc > 1)
	{
		std::vector<std::string> mppaths;

		bool scanningMPpaths = false;

		for (int i = 1; i < argc; i++)
		{
			const char* arg = argv[i];

			if (!scanningMPpaths)
			{
				if (strcmp(arg, "-flip") == 0)
					flipNormals = true;

				if (strcmp(arg, "-double") == 0)
					doublesidedfaces = true;

				if (strcmp(arg, "-splitcount") == 0)
					splitcount = fmin(atoi(argv[i + 1]), 16000);

				if (strcmp(arg, "-mp") == 0)
					scanningMPpaths = true;
			}
			else
			{
				mppaths.push_back(std::string(argv[i]));
			}
		}

		std::vector<DIF::Interior> mps;

		for (int i = 0; i < mppaths.size(); i++)
		{
			std::vector<DIF::DIF> mp = buildInteriors(mppaths[i].c_str());
			for (int j = 0; j < mp.size(); j++)
				mps.push_back(mp[j].interior[0]);
		}

		std::vector<DIF::DIF> interiors = buildInteriors(argv[1]);


		for (int i = 0; i < interiors.size(); i++)
		{
			DIF::DIF dif = interiors[i];
			std::ofstream outStr;
			char buf[16];
			outStr.open(std::string(argv[1]).substr(0, strlen(argv[1]) - 4) + std::string(itoa(i, buf, 10)) + ".dif", std::ios::out | std::ios::binary);
			dif.write(outStr, DIF::Version());
		}

	}
	else
	{
		printf("Usage:\n");
		printf("obj2difplus <file> [-flip] [-double] [-splitcount <count>] [-mp <path1> [<path2> ...]]\n");
		printf("file: path to the obj file to convert\n");
		printf("flip: (optional) flip normals\n");
		printf("double: (optional) make all faces double sided\n");
		printf("splitcount <count>: (optional) changes the amount of triangles required till a split is required\n");
		printf("mp <path1> [<paths>..]: (optional) list of paths to obj files to use as moving platforms\n");
	}
	return 0;
}
