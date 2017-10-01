#pragma once

#include <stdint.h>
#include <quaternion.h>

#include <HgTypes.h>
#include <HgShader.h>

#include <HgCamera.h>
#include <str_utils.h>
#include <memory>

	enum HgElementFlag {
		HGE_USED = 0x01, //used in scene graph
		HGE_ACTIVE = 0x02,
		HGE_HIDDEN = 0x04,
		HGE_UPDATED = 0x08,
		HGE_DESTROY = 0x10
	};

extern viewport view_port[];
extern HgCamera* _camera;
extern float* _projection;

typedef enum BlendMode {
	BLEND_NORMAL = 0,
	BLEND_ADDITIVE,
	BLEND_ALPHA
} BlendMode;

class RenderData {
	public:
		typedef RenderData*(*newRenderDataCallback)();
		static newRenderDataCallback Create;

		RenderData();
		~RenderData();

		virtual void render() = 0;
		virtual void destroy();

		HgShader* shader;
		uint8_t blendMode;
};


//typedef uint8_t vtable_index;

class HgElement;

class HgElementLogic {
public:
	virtual void update(uint32_t tdelta) = 0;
	HgElement* element;
};

//#define MAX_ELEMENT_TYPES 255
/*
extern HgElement_vtable HGELEMT_VTABLES[MAX_ELEMENT_TYPES];
extern hgstring HGELEMENT_TYPE_NAMES;
extern uint32_t HGELEMENT_TYPE_NAME_OFFSETS[MAX_ELEMENT_TYPES];
*/
/* NOTES: Try to avoid pointers, especially on 64 bit.
The entity that allocates memory for render data should
be responsible for destroying it. This is more clear than
having HgElement free render data by default and then
handling special cases.
*/
class HgElement {
public:
		point position; float scale; //16
		point origin; //origin (0,0,0) in local space
		quaternion rotation; //16
		uint8_t flags; //1

//		RenderData* m_renderData; //can be shared //4, whoever whoever populates this must clean it up.
//		void* extraData; //whoever whoever populates this must clean it up.

		void init();
		void destroy();

		inline bool isRenderable() const { return m_renderData != nullptr; }
		void render();

		inline void update(uint32_t dtime) { if (m_logic != nullptr) m_logic->update(dtime); }

		void setLogic(HgElementLogic* logic) { m_logic = logic; m_logic->element = this; }
		HgElementLogic* logic() { return m_logic; }

		RenderData* m_renderData; //can be shared //4, whoever whoever populates this must clean it up.
private:
	HgElementLogic* m_logic;
};

//typedef void(*SignalHandler)(int signum);
//typedef void(*hgelement_function)(class HgElement* e);
//typedef void (*hgelement_update_function)(struct HgElement* e, uint32_t tdelta); //strange warnings with this....
/*
typedef struct HgElement_vtable {
	hgelement_function create;
	hgelement_function destroy;
	void(*updateFunc)(class HgElement* e, uint32_t tdelta);
} HgElement_vtable;
*/
extern RenderData* (*new_RenderData)();

#define CHECK_FLAG(e,x) ((e)->flags&(x))
#define CLEAR_FLAG(e,x) ((e)->flags &= ~(x))
#define SET_FLAG(e,x) ((e)->flags |= (x))

//#define VCALL(e,function,...) if (e && e->vptr && e->vptr->function) e->vptr->function(e,__VA_ARGS__)

/*
#define VCALL(e,function,...) if (e->vptr->function) e->vptr->function(e,__VA_ARGS__)
#define VCALL_F(e,function,...) e->vptr->function(e,__VA_ARGS__)
#define SCALL(x,function,...) x->function(x,__VA_ARGS__)
#define VCALL_IDX(e,function,...) if (HGELEMT_VTABLES[e->vptr_idx].function) HGELEMT_VTABLES[e->vptr_idx].function(e,__VA_ARGS__)
*/

typedef void(*factory_clbk)(HgElement* e);

//vtable_index RegisterElementType(const char* c);
void RegisterElementType(const char* c, factory_clbk);

#define REGISTER_ELEMENT_TYPE(str) TestRegistration(str);

#ifdef _MSC_VER
#define REGISTER_LINKTIME( func, factory ) \
	__pragma(comment(linker,"/export:_REGISTER_ELEMENT"#func)); \
	extern "C" { void REGISTER_ELEMENT##func() { RegisterElementType(#func,factory); } }
//	void REGISTER_ELEMENT##func() { VTABLE_INDEX = RegisterElementType(#func); HGELEMT_VTABLES[VTABLE_INDEX] = vtable; }
//	__pragma(comment(linker, "/export:_GLOBAL_DESTROY"#func)); \
//	void GLOBAL_DESTROY##func() { }
#else
#define REGISTER_LINKTIME( func ) \
	void __attribute__((constructor)) REGISTER##func() { TestRegistration(#func, &func); }
#endif

#ifdef _MSC_VER
#define REGISTER_GLOBAL_DESTROY( func ) \
	__pragma(comment(linker, "/export:_GLOBAL_DESTROY"#func)); \
	void GLOBAL_DESTROY##func() { ##func(); }
#else
#define REGISTER_LINKTIME( func ) \
	void __attribute__((constructor)) REGISTER##func() { TestRegistration(#func, &func); }
#endif

#define SAFE_FREE(ptr) if (NULL != (ptr)) { free(ptr); ptr=NULL; }
#define SAFE_DESTROY(func,ptr) if (NULL != (ptr)) { func(ptr); free(ptr); ptr=NULL; }
