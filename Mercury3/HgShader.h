#pragma once

#include "quaternion.h"
#include <HgTypes.h>
#include <HgCamera.h>

class HgShader {
	public:
		typedef HgShader*(*createShaderCallback)(const char* vert, const char* frag);
		virtual void load() = 0;
		virtual void destroy() = 0;
		virtual void enable() = 0;

		virtual void setGlobalUniforms(const HgCamera& c) = 0;
		virtual void setLocalUniforms(const quaternion* rotation, const point* position, float scale, const point* origin) = 0;

		static HgShader* acquire(const char* vert, const char* frag);
		static void release(HgShader* shader);

		static createShaderCallback Create;
};