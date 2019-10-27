#include <OGLBackend.h>

//#define GL_GLEXT_PROTOTYPES
//#include <glcorearb.h>
//#include <glext.h>
#include <glew.h>
#include <stdio.h>

#include <oglShaders.h>
#include <oglDisplay.h>

#include <HgVbo.h>
#include <OGLFramebuffer.h>

//OGLBackend oglRenderer;

static void GLAPIENTRY ogl_error_clbk(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	//fprintf(stderr, "%s\n", message);
}

RenderBackend* OGLBackend::Create() {
	static OGLBackend openglRenderer;

	HgShader::Create = HgOglShader::Create;
	//RenderData::Create = new_renderData_ogl;

	HgTexture::GraphicsCallbacks gc;
	gc.updateTexture = ogl_updateTextureData;
	gc.deleteTexture = [](uint32_t id) { GLuint x = id; glDeleteTextures(1, &x); };
	HgTexture::gpuCallbacks = gc;

	return &openglRenderer;
}

void OGLBackend::Init() {

	GLenum err = glewInit();

	printf("%s\n", glGetString(GL_VERSION));
	printf("%s\n", glGetString(GL_VENDOR));

	GLint d;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &d);
	printf("GL_MAX_VERTEX_ATTRIBS %d\n", d);

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &d);
	printf("GL_MAX_VERTEX_UNIFORM_COMPONENTS %d\n", d);

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &d);
	printf("GL_MAX_VERTEX_UNIFORM_VECTORS %d\n", d);

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &d);
	printf("GL_MAX_VERTEX_UNIFORM_BLOCKS %d\n", d);

	glDebugMessageCallback(ogl_error_clbk, NULL);
	glEnable(GL_DEBUG_OUTPUT);

	//	glEnable(GL_MULTISAMPLE);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void OGLBackend::Clear() {
	//Write masking state glColorMask, glStencilMask and glDepthMask can affect Framebuffer Clearing functionality.
	glDepthMask(GL_TRUE); //glClear requires depth mask
	glDisable(GL_SCISSOR_TEST);
	glClearColor(.1f, .1f, .1f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OGLBackend::BeginFrame() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void OGLBackend::EndFrame() {

}

void OGLBackend::setViewport(const Viewport& vp) {
	if (vp == m_currentViewport) return;
	m_currentViewport = vp;

	glViewport(vp.x, vp.y, vp.width, vp.height);
	glScissor(vp.x, vp.y, vp.width, vp.height);
	glEnable(GL_SCISSOR_TEST);
}

std::unique_ptr<IFramebuffer> OGLBackend::CreateFrameBuffer()
{
	auto framebuffer = std::make_unique<OGLFramebuffer>();
	return std::move(framebuffer);
}

void OGLBackend::setRenderAttributes(BlendMode blendMode, RenderData::Flags flags) {
	//if (_currentBlendMode == blendMode) return;
	//_currentBlendMode = blendMode;

	switch (blendMode) {
	case BLEND_NORMAL:
		//glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		//		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_ADDITIVE:
		//		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BLEND_ALPHA:
		//		glDepthMask(GL_TRUE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}

	if (flags.FACE_CULLING) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}

	if (flags.DEPTH_WRITE) {
		glDepthMask(GL_TRUE);
	}
	else {
		glDepthMask(GL_FALSE);
	}
}

static GLenum convertTextureLocation(uint16_t location)
{
	switch (location)
	{
	case 0:
		return GL_TEXTURE0;
	case 1:
		return GL_TEXTURE1;
	case 2:
		return GL_TEXTURE2;
	//case 3:
	//	return GL_TEXTURE3;
	case 4:
		return GL_TEXTURE4;
	case 5:
		return GL_TEXTURE5;
	case 6:
		return GL_TEXTURE6;
	case 7:
		return GL_TEXTURE7;
	case 8:
		return GL_TEXTURE8;
	case 9:
		return GL_TEXTURE9;
	default:
		return GL_TEXTURE10;
	}
}

void OGLBackend::ActiveTexture(GLenum t)
{
	static GLenum activeTexture = 0;
	if (activeTexture != t)
	{
		glActiveTexture(t);
		activeTexture = t;
	}
}

void OGLBackend::BindTexture(uint16_t textureLocation, HgTexture::Handle textureHandle)
{
	static GLuint boundTexture[11] = { 0 };
	const auto texture = convertTextureLocation(textureLocation);

	ActiveTexture(texture);

	if (textureHandle == 0) return;

	if (boundTexture[textureLocation] != textureHandle)
	{
		glBindTexture(GL_TEXTURE_2D, textureHandle);
		boundTexture[textureLocation] = textureHandle;
	}
}