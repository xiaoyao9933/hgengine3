#pragma once

#include <math/matrix.h>

class HgCamera;
class RenderQueue;

class IRenderTarget
{
public:
	~IRenderTarget()
	{}

	virtual void Init() = 0;
	virtual void Render(HgCamera* camera, RenderQueue* queue) = 0;
};