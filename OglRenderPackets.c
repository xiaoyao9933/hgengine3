#include <HgRenderQueue.h>

#include <oglDisplay.h>

int8_t draw_render_packet(const render_packet* p) {
	if (p->element == NULL) return 1;

	glUniform4f(U_CAMERA_ROT, p->camera.rotation.x, p->camera.rotation.y, p->camera.rotation.z, p->camera.rotation.w);
	glUniform3f(U_CAMERA_POS, p->camera.position.components.x, p->camera.position.components.y, p->camera.position.components.z);

	hgViewport(p->viewport_idx);

	p->element->m_renderData->renderFunc(p->element);
	return 0;
}