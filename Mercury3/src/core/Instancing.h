#pragma once

#include <memory>
#include <stdint.h>
#include <RenderData.h>
#include <HgGPUBuffer.h>
#include <RenderData.h>

namespace Instancing
{

	/*
	Structure containing the instancing data required for instance rendering.
	This can indicate a subset of the instanceData buffer.
	*/
	struct InstancingMetaData
	{
		InstancingMetaData()
			:instanceCount(0), byteOffset(0)
		{
		}

		InstancingMetaData(std::shared_ptr< IHgGPUBuffer >& dataBuffer)
			:instanceCount(0), byteOffset(0), instanceData(dataBuffer)
		{
		}

		~InstancingMetaData()
		{
		}

		inline bool isValid() const { return instanceCount > 0 && instanceData; }

		//number of instances to render
		uint32_t instanceCount;

		//byte offset into the instanceData buffer where the instancing data begins
		uint32_t byteOffset;

		RenderDataPtr renderData;

		//pointer to the data buffer
		std::shared_ptr< IHgGPUBuffer > instanceData;
	};

}