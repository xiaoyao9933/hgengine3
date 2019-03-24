#include <HgModel.h>
#include <stdio.h>
#include <stdlib.h>
#include <HgVbo.h>

#include "oglDisplay.h"

#include <ini.h>
#include <IniLoader.h>
#include <StringConversions.h>

typedef struct header {
	uint32_t vertex_count, index_count;
} header;

std::unique_ptr<FILE, decltype(&fclose)> open(const char* filename) {
	std::unique_ptr<FILE, decltype(&fclose)> ret(nullptr,fclose);

	FILE* f = NULL;
	errno_t err = fopen_s(&f, filename, "rb");
	if (err != 0) {
		fprintf(stderr, "Unable to open file \"%s\"\n", filename);
	}
	else {
		ret = std::unique_ptr<FILE, decltype(&fclose)>(f,fclose);
	}

	return ret;
}

static model_data LoadModel(const char* filename) {
	header head;
	model_data r;

	auto file = open(filename);
	if (file == nullptr) return r;

	auto f = file.get();
	size_t read = fread(&head, sizeof(head), 1, f);
	if (read != 1) {
		fprintf(stderr, "Unable to read file header for \"%s\"\n", filename);
		return r;
	}

	//if (head.vertex_count > 65535) {
	//	fprintf(stderr, "Too many vertices for \"%s\"\n", filename);
	//	fclose(f);
	//	return r;
	//}

	int sizeOfIndices = 0;
	void* indexBuffer = nullptr;

	std::shared_ptr<uint32_t[]> indices32;
	std::shared_ptr<uint16_t[]> indices16;

	if (head.index_count > 50000000) {
		fprintf(stderr, "Too many indices for \"%s\"\n", filename);
		return r;
	}

	if (head.vertex_count > 65535) {
		indices32 = std::shared_ptr<uint32_t[]>(new uint32_t[head.index_count]);
		indexBuffer = indices32.get();
		sizeOfIndices = sizeof(uint32_t);
	}
	else {
		indices16 = std::shared_ptr<uint16_t[]>(new uint16_t[head.index_count]);
		indexBuffer = indices16.get();
		sizeOfIndices = sizeof(uint16_t);
	}

	auto vertices = std::shared_ptr<vbo_layout_vnut[]>(new vbo_layout_vnut[head.vertex_count]);

	read = fread(vertices.get(), sizeof(*vertices.get()), head.vertex_count, f);
	if (read != head.vertex_count) {
		fprintf(stderr, "Error, %d vertices expected, read %zd", head.vertex_count, read);
		return r;
	}

	read = fread(indexBuffer, sizeOfIndices, head.index_count, f);
	if (read != head.index_count) {
		fprintf(stderr, "Error, %d indices expected, read %zd", head.index_count, read);
		return r;
	}

	r.indices16 = indices16;
	r.indices32 = indices32;
	r.vertices = vertices;
	r.vertex_count = head.vertex_count;
	r.index_count = head.index_count;

	return std::move(r);
}

static std::shared_ptr<RenderData> init_render_data() {
	auto rd = OGLRenderData::Create();
	return rd;
}

static void updateClbk(HgEntity* e, uint32_t tdelta) {
	//	printf("cube\n");
}

static void destroy(HgEntity* e) {
	e->destroy();
}

static void* change_to_model(HgEntity* element) {
	//create an instance of the render data for all triangles to share
	element->setRenderData( init_render_data() ); //this needs to be per model instance if the model is animated
	return nullptr;
}

int8_t model_data::load(HgEntity* element, const char* filename) {
	change_to_model(element);

	OGLRenderData* rd = (OGLRenderData*)element->renderData();

	element->flags.destroy = true;

	model_data mdl( LoadModel(filename) );
	if (mdl.vertices == nullptr ||
		(mdl.indices16 == nullptr)&&(mdl.indices32 == nullptr)) return -1;

	/*
	for (int i = 0; i < mdl.vertex_count; i++) {
		float x = mdl.vertices[i].tan.x;
		float y = mdl.vertices[i].tan.y;
		float z = mdl.vertices[i].tan.z;
		float w = mdl.vertices[i].tan.w;
		printf("%f %f %f %f\n",x, y, z, w);
	}
*/
	auto record = HgVbo::GenerateFrom(mdl.vertices.get(), mdl.vertex_count);
	rd->VertexVboRecord(record);

	if (mdl.indices16 != nullptr) {
		auto iRec = HgVbo::GenerateUniqueFrom(mdl.indices16.get(), mdl.index_count);
		rd->indexVboRecord(iRec);
	}
	else if (mdl.indices32 != nullptr) {
		auto iRec = HgVbo::GenerateUniqueFrom(mdl.indices32.get(), mdl.index_count);
		rd->indexVboRecord(iRec);
	}

	element->flags.destroy = false;

	return 0;
}

bool model_data::load_ini(HgEntity* element, std::string filename) {
	IniLoader::Contents contents = IniLoader::parse(filename);
	return load_ini(element, contents);
}

bool model_data::load_ini(HgEntity* element, const IniLoader::Contents& contents) {
	change_to_model(element);

	float scale = 1;
	vector3 origin;
	vector3 position;
	quaternion orientation;

	StringConverters::convertValue(contents.getValue("model","scale"), scale);
	StringConverters::convertValue(contents.getValue("model","origin"), origin);
	StringConverters::convertValue(contents.getValue("model","position"), position);
	StringConverters::convertValue(contents.getValue("model","orientation"), orientation);

	const std::string& modelFilename = contents.getValue("model", "file");

	const std::string& vertexShader = contents.getValue("model", "vertexshader");
	const std::string& fragmentShader = contents.getValue("model", "fragmentshader");

	const std::string& diffuseTexture = contents.getValue("material", "diffusetexture");
	const std::string& specularTexture = contents.getValue("material", "speculartexture");
	const std::string& normalTexture = contents.getValue("material", "normaltexture");

	if (model_data::load(element, modelFilename.c_str()) < 0) return false;

	element->scale(scale);
	element->origin(origin.scale(1.0f/scale));
	element->position(position);
	element->orientation(orientation);

	auto renderData = element->renderData();

	if (!vertexShader.empty() && !fragmentShader.empty())
		renderData->shader = HgShader::acquire(vertexShader.c_str(), fragmentShader.c_str());

	if (!diffuseTexture.empty())
	{
		auto tmp = HgTexture::acquire(diffuseTexture, HgTexture::DIFFUSE);
		if (tmp != nullptr) {

			renderData->textures.push_back(std::move(tmp));
			renderData->updateTextures(true);
		}
	}

	if (!specularTexture.empty())
	{
		auto tmp = HgTexture::acquire(specularTexture, HgTexture::SPECULAR);
		if (tmp != nullptr) {
			renderData->textures.push_back(std::move(tmp));
			renderData->updateTextures(true);
		}
	}

	if (!normalTexture.empty())
	{
		auto tmp = HgTexture::acquire(normalTexture, HgTexture::NORMAL);
		if (tmp != nullptr) {
			renderData->textures.push_back(std::move(tmp));
			renderData->updateTextures(true);
		}
	}

	return true;
}

REGISTER_LINKTIME(hgmodel,change_to_model);