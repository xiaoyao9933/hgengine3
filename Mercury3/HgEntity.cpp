#include <HgEntity.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <assert.h>

#include <HgMath.h>
#include <unordered_map>

#include <HgTimer.h>

#include <UpdatableCollection.h>

#include <EventSystem.h>
#include <HgEngine.h>

EntityTable EntityTable::Manager;
EntityNameTable HgEntity::EntityNames;


void RegisterEntityType(const char* c, factory_clbk factory) {
	Engine::entity_factories[c] = factory;
}

void EntityLocator::RegisterEntity(HgEntity* entity)
{
	std::lock_guard<std::mutex> m_lock(m_mutex);

	const auto id = entity->getEntityId();
	if (!EntityTable::Manager.exists(id)) return;

	m_entities[id] = entity;
}

void EntityLocator::RemoveEntity(EntityIdType id)
{
	if (!EntityTable::Manager.exists(id)) return;

	std::lock_guard<std::mutex> m_lock(m_mutex);

	auto itr = m_entities.find(id);
	if (itr != m_entities.end())
	{
		m_entities.erase(itr);
	}
}

EntityLocator::SearchResult EntityLocator::Find(EntityIdType id) const
{
	SearchResult result;
	if (EntityTable::Manager.exists(id))
	{
		std::lock_guard<std::mutex> m_lock(m_mutex);

		auto itr = m_entities.find(id);
		if (itr != m_entities.end())
		{
			result.entity = itr->second;
		}
	}
	return result;
}

EntityLocator& HgEntity::Locator()
{
	static EntityLocator locator;
	return locator;
}

EntityLocator::SearchResult HgEntity::Find(EntityIdType id)
{
	return Locator().Find(id);
}

namespace Events
{

	void UpdateSPIData::execute(SPI& spi) const
	{
		if (validFlags.orientation)
			spi.orientation = spiValues.orientation;

		if (validFlags.position)
			spi.position = spiValues.position;

		if (validFlags.scale)
			spi.scale = spiValues.scale;

		if (validFlags.origin)
			spi.origin = spiValues.origin;
	}

}

void EntityNameTable::setName(EntityIdType id, const std::string name)
{
	const auto idx = findName(id);
	storage newRecord{ id, name };
	if (idx == notFound())
	{
		int32_t idx = (int32_t)m_names.size();
		m_names.emplace_back(newRecord);
		m_indices.insert({ id, idx });
	}
	else
	{
		m_names[idx] = newRecord;
	}
}

int32_t EntityNameTable::findName(EntityIdType id)
{
	int32_t idx = notFound();
	const auto itr = m_indices.find(id);
	if (itr != m_indices.end())
	{
		idx = itr->second;
	}
	return idx;
}

EntityIdType EntityTable::create()
{
	uint32_t idx = 0;
	if (m_freeIndices.size() > 1024)
	{
		idx = m_freeIndices.front();
		m_freeIndices.pop_front();
	}
	else
	{
		idx = (uint32_t)m_generation.size();
		assert(idx < MAX_ENTRIES);
		m_generation.push_back(0); //what happens when we reach 2^22 ?
	}

	m_entityTotal++;
	m_entityActive++;

	return combine_bytes(idx, m_generation[idx]);
}

void EntityTable::destroy(EntityIdType id)
{
	const auto idx = id.index();

	if (exists(id))
	{
		//increment generation to invalidate current generation in the wild.
		m_generation[idx]++;
		m_freeIndices.push_back(idx);
		m_entityActive--;
	}
}

const std::string& EntityNameTable::getName(EntityIdType id)
{
	const auto idx = findName(id);
	if (idx == notFound())
	{
		return m_blankName;
	}
	return m_names[idx].name;
}

void HgEntity::init()
{
	EntityTable::Manager.destroy(m_entityId);

	m_renderData = nullptr;
	//m_logic = nullptr;
	m_renderData = nullptr;

	auto tmp = EntityIdType();
	m_parentId = tmp;
	//m_updateNumber = 0;

	m_drawOrder = 0;

	m_entityId = EntityTable::Manager.create();

	Locator().RegisterEntity(this);
	EventSystem::PublishEvent(Events::EntityCreated(this, m_entityId));
}

HgEntity::~HgEntity() {
	destroy();
}

HgEntity::HgEntity(HgEntity &&rhs)
{
	m_spacialData = std::move(rhs.m_spacialData);
	m_renderData = std::move(rhs.m_renderData);
	//m_logic = std::move(rhs.m_logic);
	m_parentId = std::move(rhs.m_parentId);
	//m_updateNumber = std::move(rhs.m_updateNumber);
	m_drawOrder = std::move(rhs.m_drawOrder);
	flags = std::move(rhs.flags);

	//setLogic(std::move(m_logic)); //reset logic pointer

	std::swap(m_entityId, rhs.m_entityId);
	Locator().RegisterEntity(this);

	rhs.destroy();
}


void HgEntity::destroy()
{
	EventSystem::PublishEvent(Events::EntityDestroyed(this, m_entityId));
	Locator().RemoveEntity(m_entityId);

	m_renderData.reset();
	EntityTable::Manager.destroy(m_entityId);
}

HgMath::mat4f computeTransformMatrix(const SPI& sd, const bool applyScale, bool applyRotation, bool applyTranslation)
{
	//translate to origin, scale, rotate, apply local translation, apply parent transforms
	HgMath::mat4f modelMatrix;
	const auto origin_vec = -vectorial::vec3f(sd.origin.raw());
	//modelMatrix = HgMath::mat4f::translation(origin_vec);

	const float scaleFactor = (sd.scale*applyScale) + (!applyScale * 1.0f); //Integral promotion of bool, avoid branching

	//const auto correct = HgMath::mat4f::scale(scaleFactor) * modelMatrix;
	//I think this is the same result with less math
	modelMatrix = HgMath::mat4f::scale(scaleFactor);
	modelMatrix.value.w = (origin_vec * scaleFactor).xyz1().value;

	if (applyRotation) {
		modelMatrix = sd.orientation.toMatrix4() * modelMatrix;
	}

	const float translationScalar = applyTranslation * 1.0f; //Integral promotion of bool, avoid branching
	//if (applyTranslation) {
		//auto correct = HgMath::mat4f::translation(vectorial::vec3f(position().raw())) * modelMatrix;
		//I think this is the same result with less math
	const auto tmp = vectorial::vec4f(modelMatrix.value.w)
		+ vectorial::vec3f(sd.position.raw()).xyz0() * translationScalar;
	modelMatrix.value.w = tmp.value;
	//}

	return modelMatrix;
}

HgMath::mat4f HgEntity::computeWorldSpaceMatrix(const bool applyScale, bool applyRotation, bool applyTranslation) const {
	if (EntityTable::Manager.exists(m_parentId))
	{
		return computeWorldSpaceMatrixIncludeParent(applyScale, applyRotation, applyTranslation);
	}
	else
	{
		return computeTransformMatrix(m_spacialData.getSPI(), applyScale, applyRotation, applyTranslation);
	}
}

HgMath::mat4f HgEntity::computeWorldSpaceMatrixIncludeParent(const bool applyScale, bool applyRotation, bool applyTranslation) const {
	HgMath::mat4f modelMatrix = computeTransformMatrix(m_spacialData.getSPI(), applyScale, applyRotation, applyTranslation);

	auto parent = getParent();
	if (parent.isValid()) {
		modelMatrix = parent->computeWorldSpaceMatrix(flags.inheritParentScale,
			flags.inheritParentRotation, flags.inheritParentTranslation) * modelMatrix;
	}

	return modelMatrix;
}

point HgEntity::computeWorldSpacePosition() const
{
	const vector3f p;
	const auto matrix = computeWorldSpaceMatrix();
	return matrix * p;
}

//Transform point p into world space of HgEntity e
//I'm not 100% sure this matches the functionality of computeWorldSpaceMatrix so remove for now
//point toWorldSpace(const HgEntity* e, const point* p) {
//	vector3 v1 = (*p - e->origin()).scale(e->scale()).rotate(e->orientation()) + e->position();
//	return v1;
//}

REGISTER_EVENT_TYPE(Events::EntityCreated)
REGISTER_EVENT_TYPE(Events::EntityDestroyed)


REGISTER_EVENT_TYPE(Events::UpdateSPIData)