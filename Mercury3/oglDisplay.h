#pragma once

#include <stdlib.h>
#include <glew.h>
#include <vertex.h>

#include <RenderData.h>
//#include <HgEntity.h>

#include <HgTypes.h>

//#include <HgVbo.h>
#include <memory>
#include <HgTexture.h>



enum UniformLocations {
	U_ROTATION=0,
	U_POSITION=1,
	U_VIEW=2,
	U_PROJECTION=3,
	U_CAMERA_ROT=4,
	U_CAMERA_POS=5,
	U_ORIGIN=6,
	U_DIFFUSE_TEXTURE=7,
	U_SPECULAR_TEXTURE = 8,
	U_NORMAL_TEXTURE = 9,
	U_BUFFER_OBJECT1 = 10,
	U_MODEL_MATRIX = 11,
	U_MATRICES = 12,
	U_TIME_REMAIN = 13,
	U_UNIFORM_COUNT = 14
};

//struct UniformStringEnumPair
//{
//	const char* name;
//	UniformLocations idx;
//};
//
//extern UniformStringEnumPair UniformStringMap[];

//extern char *UniformString[];
//extern float* _projection;

//class OGLRenderData : public RenderData {
//public:
//	inline static std::shared_ptr<RenderData> Create() { return RenderData::Create(); }
//	OGLRenderData();
////	virtual void render();
//
//	virtual void clearTextureIDs();
//	virtual void setTexture(const HgTexture* t);
//
//	GLuint textureID[HgTexture::TEXTURE_TYPE_COUNT]; //zero ID to stop rendering
//};

//inline std::shared_ptr<RenderData> new_renderData_ogl() { return std::make_shared<OGLRenderData>(); }

uint32_t ogl_updateTextureData(HgTexture* tex);