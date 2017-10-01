#include <oglDisplay.h>

#include <stdlib.h>
#include <HgElement.h>
#include <oglShaders.h>

#include <string.h>

viewport view_port[3];

static uint8_t _currenViewPort_idx =  0xFF;
static uint8_t _currentBlendMode = 0xFF;

HgCamera* _camera;
float* _projection;

char *UniformString[] = {
	"rotation",
	"translation",
	"view",
	"projection",
	"cam_rot",
	"cam_position",
	"origin",
	NULL
};

void hgViewport(uint8_t idx) {
	if (idx == _currenViewPort_idx) return;
	_currenViewPort_idx = idx;

	const viewport vp = view_port[idx];
	glViewport(vp.x, vp.y, vp.width, vp.height);
	glScissor(vp.x, vp.y, vp.width, vp.height);
	glEnable(GL_SCISSOR_TEST);
}

GLuint hgOglVbo(vertices v) {
	GLuint vbo = 0;
	GLuint vao = 0;

//	glGenVertexArrays(1, &vao);
//	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, v.size * sizeof(*(v.points.v)), v.points.array, GL_STATIC_DRAW);
	return vbo;
}

void setup_viewports(uint16_t width, uint16_t height) {
	uint8_t i = 0;

	view_port[i].x = view_port[i].y = 0;
	view_port[i].width = width;
	view_port[i].height = height;
	++i;

	view_port[i].x = view_port[i].y = 0;
	view_port[i].width = width/2;
	view_port[i].height = height;
	++i;

	view_port[i].x = width/2;
	view_port[i].y = 0;
	view_port[i].width = width/2;
	view_port[i].height = height;
}

GLuint new_index_buffer8(uint8_t* indices, uint32_t count) {
	GLuint buf_id;
	glGenBuffers(1, &buf_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(*indices), indices, GL_STATIC_DRAW);

	return buf_id;
}

GLuint new_index_buffer16(uint16_t* indices, uint32_t count) {
	GLuint buf_id;
	glGenBuffers(1, &buf_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(*indices), indices, GL_STATIC_DRAW);

	return buf_id;
}

void setBlendMode(BlendMode blendMode) {
	if (_currentBlendMode == blendMode) return;
	_currentBlendMode = blendMode;

	if (blendMode == BLEND_NORMAL) {
//		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
//		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (blendMode == BLEND_ADDITIVE) {
//		glDepthMask(GL_FALSE);
		glEnable (GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else if (blendMode == BLEND_ALPHA) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void draw_index_vbo(HgVboMemory* vbo, uint32_t offset) {
	if (vbo->type == VBO_INDEX8 || vbo->type == VBO_INDEX16) {
		uint32_t glType = GL_UNSIGNED_BYTE;
		if (vbo->type == VBO_INDEX16) glType = GL_UNSIGNED_SHORT;
		glDrawElementsBaseVertex(GL_TRIANGLES, vbo->count, glType, 0, offset);
	}
}

static void NormalIndiceRender(OGLRenderData* rd) {
	if (rd->idx_id == 0) {
		rd->idx_id = new_index_buffer8((uint8_t*)rd->indices.data, rd->index_count);
		free_arbitrary(&rd->indices);
	}

	setBlendMode((BlendMode)rd->blendMode);
	hgvbo_use(rd->hgVbo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd->idx_id);
	glDrawElementsBaseVertex(GL_TRIANGLES, rd->index_count, GL_UNSIGNED_BYTE, 0, rd->vbo_offset);
}

OGLRenderData::OGLRenderData()
	:hgVbo(nullptr),/* indexVbo(nullptr), colorVbo(nullptr),*/ vbo_offset(0), vertex_count(0), idx_id(0), index_count(0), renderFunction(NormalIndiceRender)
{
	memset(&indices, 0, sizeof(indices));
	init();
}

void OGLRenderData::init() {
	shader = HgShader::acquire("test_vertex.glsl", "test_frag.glsl");
}

void OGLRenderData::destroy() {
//	OGLRenderData* oglrd = (OGLRenderData*)rd;
	free_arbitrary(&indices);
//	if (idx_id>0) glDeleteBuffers(1, &idx_id);

	//FIXME: Do something to clean up hgVbo
	//hgvbo_remove(d->hgvbo, d->vbo_offset, d->vertex_count)

	if (idx_id > 0) {
		glDeleteBuffers(1, &idx_id);
		idx_id = 0;
	}

	RenderData::destroy();
}

void OGLRenderData::render() {
	renderFunction(this);
}