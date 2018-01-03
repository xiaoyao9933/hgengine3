#include <stdlib.h>
#include <assert.h>

#include <Windows.h>

#include <HgScene.h>

#include <stdio.h>

//Must be a multiple of 8
//#define CHUNK_SIZE		128

//static uint16_t SceneChunk::CHUNK_SIZE = 512;
SceneChunk::SceneChunk() {
	memset(used, 0, sizeof(used));
}

bool SceneChunk::isUsed(uint16_t i)
{
	uint32_t q = i / 8;
	uint32_t r = i % 8;
	return 0<(used[q] & (1 << r));
}

void SceneChunk::set_used(uint16_t i) {
	uint32_t q = i / 8;
	uint32_t r = i % 8;
	used[q] |= (1 << r);
}

void SceneChunk::clear_used(uint16_t idx) {
	uint32_t q = idx / 8;
	uint32_t r = idx % 8;
	used[q] &= ~(1 << r);
}

static void decode_index(uint32_t index, uint16_t* h, uint16_t* l) { *l = index & 0x1FF; *h = (index >> 9) & 0x7F; }

HgScene::HgScene()
	:used_count(0) 
{

}

void HgScene::init()
{
	allocate_chunk();
}

uint32_t HgScene::getNewElement(HgElement** element) {
	uint16_t h, l;
	for (uint32_t i = 0;; ++i) {
		decode_index(i, &h, &l);
		if (h >= chunks.size()) break;
		if (!chunks[h]->isUsed(l)) {
			SceneChunk* chunk = chunks[h].get();
			HgElement* e = &chunk->elements[l];
			chunk->set_used(l);
			e->init();
			used_count++;
			*element = e;
			return i;
		}
	}

	allocate_chunk();
	return getNewElement(element);
}

void HgScene::removeElement(uint32_t i) {
	//	printf("do destroy\n");
	uint16_t h, l;
	decode_index(i, &h, &l);
	if (chunks[h]->isUsed(l)) {
		HgElement* e = get_element(i);
//		printf("Destroying element of type: %s\n", hgelement_get_type_str(e));
		e->destroy();
		chunks[h]->clear_used(l);
		used_count--;
	}
}


void HgScene::allocate_chunk() {
	chunks.push_back(std::make_unique<SceneChunk>());
}

bool HgScene::isUsed(uint32_t index)
{
	return chunks[(index >> 9) & 0x7F]->isUsed(index & 0x1FF);
}

#include <map>
extern std::map<std::string, factory_clbk> element_factories;

uint8_t create_element(char* type, HgScene* scene, HgElement** element) {
//	uint32_t idx = hgelement_get_type_index(type);

	auto factory = element_factories.find(type);

	if (factory == element_factories.end()) {
		fprintf(stderr, "Unable to find element type \"%s\"\n", type);
		return 0;
	}
	factory_clbk clbk = factory->second;
	scene->getNewElement(element);
	clbk(*element);

	//find element type creator
//	(*element)->vptr_idx = idx;
//	VCALL_IDX((*element), create);

	return 1;
}
/*
void scene_clearUpdate(HgScene* scene) {
	uint32_t i = 0;
	for (i = 0; i < scene->_size; ++i) {
		HgElement* e = get_element(scene, i);
		CLEAR_FLAG(e, HGE_UPDATED);
	}
}
*/
/* using this iterator is just as fast as manually using a for loop */
/*
HgElement* scene_next_element(HgScene_iterator* itr) {
	uint32_t x;
	for (x = itr->_current; x < itr->s->_size; ++x) {
		if (IS_USED(itr->s, HGE_USED) > 0) {
			itr->_current=x;
			return &itr->s->elements[x];
		}
	}
	return NULL;
}
*/
