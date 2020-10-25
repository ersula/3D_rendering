#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <time.h>

using namespace std;
using namespace glm;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);
float AngleToRadion = 3.14159 / 180.0;
void Swap(float *a, int i, int j) {
	float temp = a[i];
	a[i] = a[j];
	a[j] = temp;
}
int qsort(float *a, int begin, int end) {
	int i, j, temp;
	i = begin - 1; j = begin;
	for (; j < end; j++)
	{
		if (a[j] <= a[end - 1])
			Swap(a, ++i, j);
	}
	return i;
}
void randqsort(float *a, int begin, int n) {
	while (begin >= n)
		return;
	srand((unsigned)time(NULL));
	int key = (begin + rand() % (n - begin));
	Swap(a, key, n - 1);
	int m = qsort(a, begin, n);
	randqsort(a, begin, m);
	randqsort(a, m + 1, n);
}

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{

	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;

	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

class Model
{
public:
	/*  Model Data */
	vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
	vector<Mesh> meshes;
	string directory;
	string name;
	int size;

	bool obj_choosen;
	vec3 obj_pos;
	vec4 rotate;
	vec3 scale;
	
	GLint v_num;
	vector<vec3> vertex;
	vector<GLfloat> center;
	vector<GLfloat> one0fcatercorner;		//包围盒对角线中一个点
	vector<GLfloat> other0fcatercorner;		//包围盒对角线中另一个点

	void getmatrix(vec3 obj_pos, vec4 rotate, vec3 scale) {
		this->obj_pos = obj_pos;
		this->rotate = rotate;
		this->scale = scale;
	}

	/*  Functions   */
	// constructor, expects a filepath to a 3D model.
	Model(string const &path, bool gamma = false) 
	{
		obj_choosen = false;
		loadModel(path);
		//getCenter();
	}

	// draws the model, and thus all its meshes
	void Draw(Shader shader)
	{
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i].Draw(shader);
	}

private:
	/*  Functions   */
	// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(string const &path)
	{
		name = path;
		// read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// check for errors
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// retrieve the directory path of the filepath
		directory = path.substr(0, path.find_last_of('/'));

		// process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode *node, const aiScene *scene)
	{
		// process each mesh located at the current node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// the node object only contains indices to index the actual objects in the scene. 
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}

	}

	Mesh processMesh(aiMesh *mesh, const aiScene *scene)
	{
		// data to fill
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		// Walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;
			// texture coordinates
			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			// tangent
			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.Tangent = vector;
			// bitangent
			vector.x = mesh->mBitangents[i].x;
			vector.y = mesh->mBitangents[i].y;
			vector.z = mesh->mBitangents[i].z;
			vertex.Bitangent = vector;
			vertices.push_back(vertex);
		}
		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		// process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN

		// 1. diffuse maps
		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		// 2. specular maps
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		// 3. normal maps
		std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		// 4. height maps
		std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		for (int i = 0; i < vertices.size(); i++) {
			vertex.push_back(vertices[i].Position);
		}
		v_num = vertices.size();
		// return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices, textures);
	}

	// checks all material textures of a given type and loads the textures if they're not loaded yet.
	// the required info is returned as a Texture struct.
	vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
	{
		vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
					break;
				}
			}
			if (!skip)
			{   // if texture hasn't been loaded already, load it
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), this->directory);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			}
		}
		return textures;
	}

	vector<GLfloat> getCenter(/*GLfloat &x,GLfloat &y,GLfloat &z*/) {
		GLfloat x_max, x_min, y_max, y_min, z_max, z_min, x_center, y_center, z_center, x_sum, y_sum, z_sum;
		x_max = x_min = y_max = y_min = z_max = z_min = 0.0f;
		float *x = new float[v_num];
		float *y = new float[v_num];
		float *z = new float[v_num];

		for (int i = 0; i < v_num; i++) {
			x[i] = vertex[i][0];
			y[i] = vertex[i][1];
			z[i] = vertex[i][2];
		}

		randqsort(x, 0, v_num);		
		randqsort(y, 0, v_num);
		randqsort(z, 0, v_num);

		for (int i = 0; i < 10; i++) {			//取最小10个数的平均作为最小值
			x_min += x[i];
			y_min += y[i];
			z_min += z[i];
		}
		x_min /= 10; y_min /= 10; z_min /= 10;
		for (int i = v_num - 10; i < v_num; i++) {		//取最大10个数的平均作为最大值
			x_max += x[i];
			y_max += y[i];
			z_max += z[i];
		}
		x_max /= 10; y_max /= 10; z_max /= 10;

		x_center = (x_min + x_max) / 2.0;
		y_center = (y_min + y_max) / 2.0;
		z_center = (z_min + z_max) / 2.0;
		x_sum = fabs(x_max - x_min);
		y_sum = fabs(y_max - y_min);
		z_sum = fabs(z_max - z_min);

		one0fcatercorner.push_back(x_max);
		one0fcatercorner.push_back(y_max);
		one0fcatercorner.push_back(z_max);

		other0fcatercorner.push_back(x_min);
		other0fcatercorner.push_back(y_min);
		other0fcatercorner.push_back(z_min);

		center.push_back(x_center);
		center.push_back(y_center);
		if (((z_center <= 0.1f && z_center >= -0.1f) || z_min >= 0.0f) && (x_sum >= 2.0f || y_sum >= 2.0f))
			z_center += z_sum / 2 + y_sum / 2.0 * tan(60.0*AngleToRadion);
		center.push_back(z_center);
		delete[]x;
		delete[]y;
		delete[]z;

		size = std::max(std::max((one0fcatercorner[0] - other0fcatercorner[0]), one0fcatercorner[1] - other0fcatercorner[1]), one0fcatercorner[2] - other0fcatercorner[2]);
		cout << name << endl;
		//cout << size << endl;
		return center;
	}

};

#endif