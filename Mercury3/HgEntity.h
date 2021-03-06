#pragma once

#include <stdint.h>
#include <quaternion.h>

#include <HgTypes.h>
#include <HgShader.h>

#include <memory>
#include <HgTexture.h>
#include <vector>

#include <HgTimer.h>
//#include <HgVbo.h>
#include <RenderData.h>

#include <unordered_map>
//#include <ServiceLocator.h>
#include <GuardedType.h>
#include <deque>
#include <stdint.h>
#include <unordered_set>
#include <EntityIdType.h>
#include <TransformManager.h>

//#include <core/HgScene2.h>
#include <HgScene2.h> //REGISTER_LINKTIME2 needs this
class HgEntity;
class model_data;
class HgScene;

struct PositionalData {
	//position, and rotation are in global coordinate system
	point position; float scale; //16
	point origin; //origin (0,0,0) in local space
	quaternion rotation; //16
	uint8_t flags; //1
};

struct EntityFlags {
	EntityFlags() :
		used(false), active(false), hidden(false), updated(false),
		destroy(false), /*update_textures(false),*/
		inheritParentScale(true), inheritParentRotation(true),
		inheritParentTranslation(true)
	{}
	bool used : 1; //used in scene graph
	bool active : 1;
	bool hidden : 1;
	bool updated : 1;
	bool destroy : 1;
	//bool update_textures : 1;
	bool inheritParentScale : 1;
	bool inheritParentRotation : 1;
	bool inheritParentTranslation : 1;
};

struct SPI
{
	SPI()
		:scale(1.0)
	{}

	quaternion orientation; //16 bytes
	vertex3f position; float scale; //16 bytes
	vertex3f origin; //12 bytes
};

class SpacialData {
private:
	SPI m_spi;

	bool m_hasPrevious;
	vertex3f m_prevPosition;
public:
	SpacialData() : m_hasPrevious(false)
	{}

	inline const vertex3f origin() const { return m_spi.origin; }
	inline void origin(const vertex3f& p) { m_spi.origin = p; }

	inline bool hasPrevious() const { return m_hasPrevious; }
	inline const vertex3f& PreviousPosition() const { return m_prevPosition; }
	inline void PreviousPosition(const vertex3f& p) { m_hasPrevious = true; m_prevPosition = p; }

	//inline vertex3f& position() { return m_position; }
	inline const vertex3f& position() const { return m_spi.position; }
	inline void position(const vertex3f& p) { m_spi.position = p; }

	//inline quaternion& orientation() { return m_orientation; }
	inline const quaternion& orientation() const { return m_spi.orientation; }
	inline void orientation(const quaternion& q) { m_spi.orientation = q; }

	inline float scale() const { return m_spi.scale; }
	inline void scale(float s) { m_spi.scale = s; }

	inline const SPI getSPI() const { return m_spi; }
	inline SPI& getSPI() { return m_spi; }
};

//typedef uint32_t EntityIdType;

namespace Events
{
	class UpdateSPIData
	{
	public:
		void execute(SPI& spi) const;

		struct ValidityFlags {
			ValidityFlags() :
				orientation(false), position(false), scale(false), origin(false)
			{}

			bool orientation : 1;
			bool position : 1;
			bool scale : 1;
			bool origin : 1;
		};

		void setOrientation(quaternion o) { spiValues.orientation = o; validFlags.orientation = true; }
		void setPosition(vertex3f p) { spiValues.position = p; validFlags.position = true; }
		void setScale(float s) { spiValues.scale = s; validFlags.scale = true; }
		void setOrigin(vertex3f o) { spiValues.origin = o; validFlags.origin = true; }

		EntityIdType entityId;
		ValidityFlags validFlags;
		SPI spiValues;
	};
}

class EntityLocator
{
public:
	class SearchResult
	{
	public:
		SearchResult() : entity(nullptr)
		{}

		bool isValid() const { return entity != nullptr; }

		HgEntity* entity;
		HgEntity* operator->() { return entity; }
	};

	void RegisterEntity(HgEntity* entity);
	void RemoveEntity(EntityIdType id);

	SearchResult Find(EntityIdType id) const;
private:
	std::unordered_map<EntityIdType, HgEntity*> m_entities;
	mutable std::mutex m_mutex;
};

class EntityNameTable
{
	static inline auto notFound() { return std::numeric_limits<int32_t>::max(); }

public:
	void setName(EntityIdType id, const std::string name);
	const std::string& getName(EntityIdType id);

private:
	inline int32_t findName(EntityIdType id);

	struct storage
	{
		EntityIdType id;
		std::string name;

		//inline bool operator<(const storage& rhs) const { return id < rhs.id; }
	};

	std::unordered_map<EntityIdType, int32_t> m_indices;
	std::vector<storage> m_names;
	std::string m_blankName;
};

class EntityTable
{
	//Based on http://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html
public:
	static const uint32_t MAX_ENTRIES = (1 << EntityIdType::INDEX_BITS);
	//static const uint32_t MAX_ENTRIES = 5; //for testing create() assert
	EntityTable()
	{
		m_generation.reserve(MAX_ENTRIES);
		m_entityTotal = 0;
		m_entityActive = 0;
	}

	//create a new entity id
	EntityIdType create();

	//check if an entity exists
	bool exists(EntityIdType id) const
	{
		const auto idx = id.index();
		if (idx < m_generation.size())
		{
			return (m_generation[idx] == id.generation());
		}
		return false; //this should never happen
	}

	//destroy an entity
	void destroy(EntityIdType id);

	//total number of entites ever created
	uint32_t totalEntities() const { return m_entityTotal; }

	//current number of entities in existance
	uint32_t numberOfEntitiesExisting() const { return m_entityActive; }


	static EntityTable Manager;

private:
	inline EntityIdType combine_bytes(uint32_t idx, uint8_t generation)
	{
		const uint32_t id = (generation << EntityIdType::INDEX_BITS) | idx;
		return id;
	}

	std::vector<uint8_t> m_generation;

	/* I think a deque is better than a list here because there
	would be fewer heap allocaitons/fragmentation. */
	std::deque<uint32_t> m_freeIndices;

	uint32_t m_entityTotal; //total number of entities that have ever existed
	uint32_t m_entityActive; //number of entities currently in existance
};


//Compute local transformation matrix
HgMath::mat4f computeTransformMatrix(const SPI& sd, const bool applyScale = true, bool applyRotation = true, bool applyTranslation = true);

/* NOTES: Try to avoid pointers, especially on 64 bit.
The entity that allocates memory for render data should
be responsible for destroying it. This is more clear than
having HgEntity free render data by default and then
handling special cases.
*/
class HgEntity {
public:
		HgEntity()
			:/* m_updateNumber(0), */m_renderData(nullptr)
		{}

		~HgEntity();

		HgEntity(const HgEntity &other) = delete;
		HgEntity(HgEntity &&); //move operator

		void init();
		void destroy();

		inline EntityIdType getEntityId() const { return m_entityId; }

		inline const point origin() const { return m_spacialData.origin(); }
		inline void origin(const point& p) { m_spacialData.origin(p); }

		//inline point& position() { return m_spacialData.position(); }
		inline const point& position() const { return m_spacialData.position(); }
		inline void position(const point& p) { m_spacialData.position(p); }

		//inline quaternion& orientation() { return m_spacialData.orientation(); }
		inline const quaternion& orientation() const { return m_spacialData.orientation(); }
		inline void orientation(const quaternion& q) { m_spacialData.orientation(q); }

		inline float scale() const { return m_spacialData.scale(); }
		inline void scale(float s) { m_spacialData.scale(s); }

		const SpacialData& getSpacialData() const { return m_spacialData; }
		SpacialData& getSpacialData() { return m_spacialData; }
		void setSpacialData(const SpacialData& x) { m_spacialData = x; }

		inline bool isRenderable() const { return m_renderData != nullptr; }
		//inline void render() { if (isRenderable()) m_renderData->render();  }

		//inline bool needsUpdate(uint32_t updateNumber) const { return ((m_updateNumber != updateNumber)); }
		//inline void update(HgTime dtime, uint32_t updateNumber) {
		//	m_updateNumber = updateNumber;
		//	//require parents to be updated first
		//	auto parent = getParent();
		//	if ((parent.isValid()) && parent->needsUpdate(updateNumber)) parent->update(dtime, updateNumber);
		//	//m_logic->update(dtime);
		//}

		//Send texture data to GPU. I don't like this here and it pulls in extended data.
		//void updateGpuTextures();

		inline void setParent(HgEntity* parent) { m_parentId = parent->getEntityId(); }
		inline void setParent(EntityIdType id) { m_parentId = id; }
		
		inline EntityLocator::SearchResult getParent() const
		{
			EntityLocator::SearchResult r;

			if (EntityTable::Manager.exists(m_parentId)) r = Find(m_parentId);
			return r;
		}

		inline void setChild(HgEntity* child) { child->setParent(this); }

		inline void setRenderData(std::shared_ptr<RenderData>& rd) { m_renderData = rd; }

		RenderData* renderData() { return m_renderData.get(); }
		const RenderData* renderData() const { return m_renderData.get(); }

		RenderDataPtr& getRenderDataPtr() { return m_renderData; }
		const RenderDataPtr& getRenderDataPtr() const { return m_renderData; }

		//inline void setScene(HgScene* s) { m_extendedData->m_scene = s; }

		HgMath::mat4f computeWorldSpaceMatrix(bool scale = true, bool rotation = true, bool translation = true) const;
		HgMath::mat4f computeWorldSpaceMatrixIncludeParent(bool scale = true, bool rotation = true, bool translation = true) const;
		point computeWorldSpacePosition() const;

		inline void setInheritParentScale(bool x) { flags.inheritParentScale = x; }
		inline void setInheritParentRotation(bool x) { flags.inheritParentRotation = x; }
		inline void setInheritParentTranslation(bool x) { flags.inheritParentTranslation = x; }

		inline void setDestroy(bool x) { flags.destroy = x; }
		inline void setHidden(bool x) { flags.hidden = x; }

		//Lower numbers draw first. default draw order is 0
		inline void setDrawOrder(int8_t order) { m_drawOrder = order; }
		inline int8_t getDrawOrder() const { return m_drawOrder; }

		inline void setName(const std::string& name) { EntityNames.setName(m_entityId, name); }
		inline std::string& getName() const { EntityNames.getName(m_entityId); }

		inline EntityFlags getFlags() const { return flags; }

		/*	Find an existing entity by id. Returned pointer is managed, do not delete.
			Return nullptr if the entity does not exist.
		*/
		static EntityLocator::SearchResult Find(EntityIdType id);
private:

	static EntityNameTable EntityNames;

	//static EntityIdType m_nextEntityId;
	static EntityLocator& Locator();

	SpacialData m_spacialData; //local transormations
	EntityIdType m_entityId;

	RenderDataPtr m_renderData;

	EntityIdType m_parentId;
	//uint32_t m_updateNumber;
	int8_t m_drawOrder;

	EntityFlags flags;
public:
};

namespace Events
{
	class EntityCreated
	{
	public:
		EntityCreated(HgEntity* e, EntityIdType id)
			:entity(e)
		{}
		HgEntity* entity;
		EntityIdType entityId;
	};

	class EntityDestroyed
	{
	public:
		EntityDestroyed(HgEntity* e, EntityIdType id)
			:entity(e), entityId(id)
		{}
		HgEntity* entity;
		EntityIdType entityId;
	};

}
//Transform point p into world space of HgEntity e
//point toWorldSpace(const HgEntity* e, const point* p);

class IUpdatableCollection;
class RenderQueue;


extern RenderData* (*new_RenderData)();

typedef void*(*factory_clbk)(HgEntity* e);

void RegisterEntityType(const char* c, factory_clbk);

#define REGISTER_ENTITY_TYPE(str) TestRegistration(str);

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define LINKER_PREFIX
#else
#define LINKER_PREFIX _
#endif
#endif

// Check GCC
#if __GNUC__
//#if __x86_64__ || __ppc64__
//#define ENVIRONMENT64
//#else
//#define ENVIRONMENT32
//#endif
#define LINKER_PREFIX _
#endif

#ifdef _MSC_VER
#define REGISTER_LINKTIME( func, factory ) \
	__pragma(comment(linker,"/export:"##LINKER_PREFIX##"REGISTER_ENTITY"#func)); \
	extern "C" { void REGISTER_ENTITY##func() { RegisterEntityType(#func,factory); } }

#define REGISTER_LINKTIME2( func, type ) \
	__pragma(comment(linker,"/export:"##LINKER_PREFIX##"REGISTER_ENTITY2"#func)); \
	extern "C" { void REGISTER_ENTITY2##func() { Engine::HgScene::RegisterEntityFactory(#func, Engine::HgScene::generate_entity<type>); } }

#define REGISTER_LINKTIME3( func, type, CollectionType ) \
	__pragma(comment(linker,"/export:"##LINKER_PREFIX##"REGISTER_ENTITY3"#func)); \
	extern "C" { void REGISTER_ENTITY3##func() { Engine::HgScene::RegisterEntityFactory(#func, Engine::HgScene::generate_entity2<type, CollectionType>); } }

#else
#define REGISTER_LINKTIME( func ) \
	void __attribute__((constructor)) REGISTER##func() { TestRegistration(#func, &func); }
#endif

#ifdef _MSC_VER
//#define REGISTER_GLOBAL_DESTROY( func ) \
	//__pragma(comment(linker, "/export:_GLOBAL_DESTROY"#func)); \
	//void GLOBAL_DESTROY##func() { ##func(); }
#else
#define REGISTER_LINKTIME( func ) \
	void __attribute__((constructor)) REGISTER##func() { TestRegistration(#func, &func); }
#endif

#define SAFE_DESTROY(func,ptr) if (NULL != (ptr)) { func(ptr); free(ptr); ptr=NULL; }
