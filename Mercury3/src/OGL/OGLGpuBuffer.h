#pragma once

#include <glew.h>
#include <HgVboMemory.h>
#include <HgGPUBuffer.h>

//Contains opengl buffer and texture id for texture buffers
struct GLBufferId
{
	GLBufferId() : buffId(0), texId(0), m_bufferBound(false)
	{}

	~GLBufferId();

	void AllocateOnGPU();

	//Only allow move operators.
	//We don't want copies because we don't want the destructor to deallocate the opengl buffer ids.
	GLBufferId(const GLBufferId& rhs) = delete;
	GLBufferId(GLBufferId&& rhs);

	const GLBufferId& operator=(const GLBufferId& rhs) = delete;
	const GLBufferId& operator=(GLBufferId&& rhs);

	void AssociateBuffers();

	bool isValid() const { return buffId != 0; }

	GLuint buffId;
	GLuint texId;
	bool m_bufferBound;
};

class OGLHgGPUBuffer : public IGPUBufferImpl {
public:
	OGLHgGPUBuffer() : m_lastSize(0) {}
	~OGLHgGPUBuffer();

	virtual void SendToGPU(const IHgGPUBuffer* bufferObject) final;
	virtual void Bind(uint16_t textureLocation) final;

	//inline GLuint TextureId() const { return m_bufferIds.texId; }
private:
	static GLBufferId getGLBufferId();

	//return GLBufferId to pool for reuse
	static void releaseGLBufferId(GLBufferId& rhs);
	static std::vector<GLBufferId> m_useableBufferIds; //a pool of open glbuffer resources

	GLBufferId m_bufferIds;

	size_t m_lastSize;
};