#include "game/game_data.hpp"

#include "utils/class/logger.hpp"
#include "../pathfind.hpp"

ConstructionData::ConstructionData()
	: Variant((size_t)SERIALIZABLE_CONSTRUCTION,
			  0)
{
}

ConstructionData::ConstructionData(const BuildingQueueData &data, size_t id)
	: Variant((size_t)SERIALIZABLE_CONSTRUCTION,
			  id)
{
	this->queueData = data;
}

int ConstructionData::work_needed() const
{
	return (int)queueData
		.step
		->buildWork;
}

int ConstructionData::work_left() const
{
	return work_needed() - work;
}

void to_json(json &j, const ConstructionData &v)
{
	WARNING("%s ConstructionData to_json doe's not save \"step\"");
	j["work"] = v.work;
	j["pos"] = v.queueData.pos;
	j["uses"] = v.queueData.useResources;
	j["is"] = v.queueData.isConstruction;
	j["type"] = v.queueData.buildType;
}

void from_json(const json &j, ConstructionData &v)
{
	WARNING("%s ConstructionData to_json doe's not load \"step\"");
	try
	{
		j.at("work").get_to(v.work);
		j.at("pos").get_to(v.queueData.pos);
		j.at("uses").get_to(v.queueData.useResources);
		j.at("is").get_to(v.queueData.isConstruction);
		j.at("type").get_to(v.queueData.buildType);
		v.queueData.isConstruction = true;
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize ConstructionData %s", e.what());
	}
}

void Constructions::clear()
{
	keysTmp.clear();
	container.clear();
	keys.clear();
	counter = 0;
}

Constructions::iterator Constructions::begin()
{
	return keys.begin();
}

Constructions::iterator Constructions::end()
{
	return keys.end();
}

Constructions::iterator Constructions::find(BuildingBase *b)
{
	return keys.find(b);
}

size_t Constructions::count(BuildingBase *b)
{
	return keys.count(b);
}

ConstructionData &Constructions::at(size_t i)
{
	return container.at(i);
}

void Constructions::insert(BuildingBase *b, const ConstructionData &d)
{
	assert(b);
	keys.insert({b, counter});
	container[counter++] = d;
}

ConstructionData &Constructions::at(BuildingBase *b)
{
	return container.at(keys.at(b));
}

bool Constructions::erase_value(BuildingBase *b)
{
	auto itr = keys.find(b);
	if (itr == keys.end())
		return false;
	size_t index = (*itr).second;
	auto itr2 = container.find(index);
	if (itr2 == container.end())
		return false;
	container.erase(index);
	keys.erase(b);
	return true;
}

json Constructions::to_json() const
{
	json j;
	j["keys"] = keys;
	j["values"] = container;
	j["counter"] = counter;
	return j;
}

void Constructions::from_json(const json &j)
{
	// j.at("keys").get_to(keys);
	j.at("values").get_to(container);
	j.at("counter").get_to(counter);

	for (const auto &key : j.at("keys"))
	{
		VariantPtr<BuildingBase> buildBase;
		size_t id;
		key.at(0).get_to(buildBase);
		key.at(1).get_to(id);
		keysTmp.push_back({buildBase, id});
	}
	if (0)
		for (auto& x : container)
		{
			DEBUG("%d %s",
				  (int)x.first,
				  (json{x.second}).dump().c_str());
		}
}

void Constructions::serialize_publish(const SerializeMap &map)
{
	// Inialize BuildingBase per construction object
	for (auto &v : keysTmp)
	{
		auto &b = const_cast<
			VariantPtr<BuildingBase> &>(v.first);
		map.apply(b);
		keys[v.first] = v.second;
	}
	if (0)
		for (const auto &x : keys)
		{
			DEBUG("Test: %s %d",
				  x.first.to_json_ptr(0).dump().c_str(),
				  (int)x.second);
		}
}

void Constructions::serialize_initialize(const SerializeMap &map)
{
	// Get upgrade tree
	GameData *g = map.get<GameData>(
		SERIALIZABLE_DATA, 0);
	for (auto &v : container)
	{
		ConstructionData &b = const_cast<
			ConstructionData &>(v.second);
		auto &x = b.queueData;
		x.step = dynamic_cast<UpgradeTree*>(
			g->bodyConfigMap.at(
			(t_id)ENUM_BUILDING_TYPE, (t_id)x.buildType)[0]);
	}
}

GameData::GameData(Chunks *chunks) : chunks(chunks)
{
	threadPath = new QueryPoolPath(this, 4, 16);
}

GameData::GameData()
	: Variant((size_t)SERIALIZABLE_DATA, 0)
{
	// 馃嵒
	// ↑wtf? i dont remember writing that
}

GameData::~GameData()
{
	clean_world();

	WARNING("todo implement nodyConfigMap deleter");

	for (auto& x : bodyConfigMap.data)
		for (auto& y : x.second)
			for (auto& z : y.second)
	  			delete z.second;
	delete threadPath;
}

void GameData::clean_world()
{
	resources.clear();

	for (auto& x : mapEnumTree.groups)
	{
		for (auto& y : x.idTMap)
		{
			y.second.clear();
			y.second.root = nullptr;
		}
	}

	for (GameBody* gb : bodies)
	{
		delete gb;
	}
	bodies.clear();

	LOG("Cleared bodies successfully");

	for (BuildingBase* bb : buildingBases)
	{
		delete bb;
	}
	buildingBases.clear();

	LOG("Cleared building bases successfully");

	for (VariantPtr<PowerNetwork>& vptrNetwork : networks)
	{
		delete vptrNetwork.get();
	}
	networks.clear();
	LOG("Cleared networks successfully");

	chunks->clear();
	buildings.clear();
	entityCitizens.clear();

	lpowerTotal = 0;
	powerTotal = 0;

	frameCount = 0;
	constructions.clear();

	infoBuild = nullptr;
	infoEntity = nullptr;
	entityQueue.clear();
	buildingQueue.clear();
	bodyQueue.clear();
	addedTiles.clear();
	removedTiles.clear();

	entityRemoveQueue.clear();

	genIndex = 0;
	objectIdCounter.clear();
	variantFactory.clear_counters();
}

json GameData::to_json() const
{
	json j;
	return j;
}

void GameData::from_json(const json &j)
{
}

void GameData::serialize_publish(const SerializeMap &map)
{
}

void GameData::serialize_initialize(const SerializeMap &map)
{
}

t_tiletree *GameData::get_tree(
	const t_group group,
	const t_id id)
{
	bool has;
	t_tiletree *ret = &mapEnumTree.get(group, id, &has);
	if (!has)
	{
		WARNING("Can't find tree of group: %d and id %d",
				(int)group,
				(int)id);
		return nullptr;
	}
	return ret;
}

t_idwtree GameData::get_pair(const t_group group, const t_id id)
{
	t_idwtree ret;
	ret.first = t_idpair{group, id};
	ret.second = get_tree(group, id);
	return ret;
}

std::list<t_idwtree> GameData::get_trees(GameBody *body)
{
	std::list<t_idwtree> ret;
	const auto checkValid = [](const std::list<t_idwtree> &ret, const char *type) {
		return;
		if (ret.empty())
			return;
		return;

		LOG_ERROR("Failed search for type \"%s\"", type);
		assert(0);
	};

	ret.push_back(get_pair(ENUM_BODY_TYPE, (t_id)BodyType::NONE));

	ret.push_back(get_pair(ENUM_BODY_TYPE, (t_id)body->type));
	checkValid(ret, "Body Type");

	ret.push_back(get_pair(ENUM_ALIGNMENT, (t_id)body->alignment));
	checkValid(ret, "Aligment");

	switch (body->type)
	{
	case BodyType::ENTITY:
	{
		EntityBody *entity = dynamic_cast<EntityBody *>(body);

		ret.push_back(get_pair(ENUM_ENTITY_TYPE, (t_id)entity->entityType));
		checkValid(ret, "Entity Type");
		/*
		for (size_t i = (size_t)EntityUse::NONE + 1; i < (size_t)EntityUse::LAST; ++i)
		{
			if (build->props.bool_is((PropertyBool)i))
			{
				// DEBUG("%d\n", (int)i);
				ret.push_back(get_pair(ENUM_PROPERTY_BOOL, i));
				checkValid(ret, "Enum Use");
			}
		}*/

		switch (entity->entityType)
		{
		case EntityType::CITIZEN:
		{
			EntityCitizen *citizen = dynamic_cast<EntityCitizen *>(entity);
			if (citizen->job != CitizenJob::NONE)
			{
				ret.push_back(get_pair(ENUM_CITIZEN_JOB, (t_id)citizen->job));
				checkValid(ret, "Citizen Job");
			}
			break;
		}
		break;
		case EntityType::ENEMY:
		{
			EntityEnemy *enemy =
				dynamic_cast<EntityEnemy *>(entity);
			if (enemy->enemyType != EntityEnemyType::NONE)
			{
				ret.push_back(
					get_pair(ENUM_ENEMY_TYPE,
							 (t_id)enemy->enemyType));
				checkValid(ret, "Enemy Type");
			}
			break;
		}
		break;
		default:
			WARNING("Unknown entity type: ", (int)entity->entityType);
			assert(0);
			break;
		};
	}
	break;
	case BodyType::BUILDING:
	{
		BuildingBase *build = dynamic_cast<BuildingBody *>(body)->base;
		assert(build);
		ret.push_back(get_pair(ENUM_BUILDING_TYPE, (t_id)build->buildType));

		for (size_t i = (size_t)PropertyBool::NONE + 1; i < (size_t)PropertyBool::LAST; ++i)
		{
			if (build->props.bool_is((PropertyBool)i))
			{
				// DEBUG("%d\n", (int)i);
				ret.push_back(
					get_pair(ENUM_PROPERTY_BOOL, i));
				checkValid(ret, "Enum Use");
			}
		}

		// Specified unique enums
		if (build->props.bool_is(PropertyBool::WORKPLACE) &&
			build->entities.size() < build->entityLimit)
		{
			ret.push_back(get_pair(ENUM_INGAME_PROPERTIES,
								   (t_id)IngameProperties::NEEDS_WORKERS));
			checkValid(ret, "Ingame Proeprties");
		}
	}
	break;
	default:
		WARNING("Unknown body type: %d", (int)body->type);
		assert(0);
		break;
	}

	return ret;
}

static std::string make_shorter(const std::string& str)
{
	std::vector<std::string> vec;
	vec = str_split(str, "::");
	if (vec.size() < 2)
		return str;
	return vec.at(1);
}

t_body_timer GameData::create_timer(t_seconds start)
{
	return timerManager.make_new(start);
}

void GameData::add_timer(t_body_timer& timer)
{
	timerManager.add_new(timer);
}

std::string GameData::enum_get_str(const t_idpair& pair, bool outShort)
{
	std::string str = mapEnumTree.get_str(pair);;
	if (outShort)
		str = make_shorter(str);
	return str;
}

std::string GameData::enum_get_str(t_group group, t_id id, bool outShort)
{
	std::string str = mapEnumTree.get_str(group, id);
	if (outShort)
		str = make_shorter(str);
	return str;
}

std::string GameData::enum_get_str(const GameBody* g, bool outShort)
{
	std::string str = mapEnumTree.get_str(ENUM_BODY_TYPE, (t_id) (g ? g->type : BodyType::NONE));
	if (outShort)
		str = make_shorter(str);
	return str;
}

int GameData::alignment_compare(const size_t alignmentA, const size_t alignmentB)
{
	if (alignmentA == ALIGNMENT_NONE || alignmentB == ALIGNMENT_NONE)
		return ALIGNMENTS_NONE;
	if (alignmentA == ALIGNMENT_NEUTRAL || alignmentB == ALIGNMENT_NEUTRAL)
		return ALIGNMENT_NEUTRAL;
	if (alignmentA != alignmentB)
		return ALIGNMENTS_ENEMIES;
	return ALIGNMENTS_FRIENDLY;
}

int GameData::alignment_opposite(const size_t alignmentA, const size_t alignmentB)
{
	return alignment_compare(alignmentA, alignmentB) == ALIGNMENTS_ENEMIES;
}

size_t GameData::alignment_opposite(size_t alignment)
{
	constexpr size_t RET [] = { 
		ALIGNMENT_NONE , ALIGNMENT_NONE, 
		ALIGNMENT_ENEMY, ALIGNMENT_FRIENDLY };
	if (alignment < (sizeof(RET) / sizeof(*RET)))
		return RET[alignment];
	return ALIGNMENT_NONE;
}


void GameData::depth_sort(const int rot)
{
	/*
	struct Sorter
	{
		int rot;

		float calcDepth(const sf::Vector2f& v)
		{
			return v.x + v.y;
		}

		bool operator()(GameBody* e1, GameBody* e2)
		{
			float a = calcDepth(vec_90_rotate(e1->pos, -rot));
			float b = calcDepth(vec_90_rotate(e2->pos, -rot));
			// a += e1->z;
			// b += e2->z;
			return a < b;
		}
	};
	
	Sorter sorter{rot};
	std::sort(
		bodies.begin(),
		bodies.end(),
		sorter
	);
	*/
	
	constexpr auto calcDepth = [](const sf::Vector2f &v) {
		return v.x + v.y;
	};

	std::sort(bodies.begin(), bodies.end(),
		[rot, calcDepth]( GameBody* e1, GameBody* e2) {
			float a = calcDepth(vec_90_rotate(e1->pos, -rot));
			float b = calcDepth(vec_90_rotate(e2->pos, -rot));
			// a += e1->z;
			// b += e2->z;
			return a < b;
		}
	);
	
	/*
	this->bodies.sort([rot, calcDepth](const GameBody *e1, const GameBody *e2) {
		float a = calcDepth(vec_90_rotate(e1->pos, -rot));
		float b = calcDepth(vec_90_rotate(e2->pos, -rot));
		// a += e1->z;
		// b += e2->z;
		return a < b;
	});*/
}

std::string GameData::resources_to_str(Resources& res)
{
	std::stringstream ss;
	bool any = false;
	for (auto& x : res.resCount)
	{
		if (x.second == 0)
			continue;
		ss << resourceWeights.get_str(x.first);
		ss << ":";
		ss << x.second;
		ss << " ";
		any = true;
	}
	if (!any)
		ss << "Free";
	return ss.str();
}

void GameData::calculate_resources()
{
	static const bool onlyStorage = 1;
	resources = Resources::res_empty(&resourceWeights);
	power = 0;
	for (auto itr = buildingBases.begin(); itr != buildingBases.end(); ++itr)
	{
		BuildingBase *b = (*itr);
		assert(b);
		if (b->is_any_storage() || !onlyStorage)
		{
			resources += b->rStorage;
		}
		if (b->props.bool_is(PropertyBool::POWER_NETWORK) || !onlyStorage)
		{
			power += b->powerValue;
		}
	}
}

float GameData::bodies_distance(GameBody *a, GameBody *b)
{
	static const auto calc = [](ShapeType a, ShapeType b) {
		return (int)a + ((int)b << 2);
	};
	typedef ShapeType t_sp;
	constexpr int PP = calc(t_sp::POINT, t_sp::POINT);
	constexpr int PR = calc(t_sp::POINT, t_sp::RECT);
	constexpr int PC = calc(t_sp::POINT, t_sp::CIRCLE);
	constexpr int RP = calc(t_sp::RECT, t_sp::POINT);
	constexpr int RR = calc(t_sp::RECT, t_sp::RECT);
	constexpr int RC = calc(t_sp::RECT, t_sp::CIRCLE);
	constexpr int CP = calc(t_sp::CIRCLE, t_sp::POINT);
	constexpr int CR = calc(t_sp::CIRCLE, t_sp::RECT);
	constexpr int CC = calc(t_sp::CIRCLE, t_sp::CIRCLE);

	sf::Vector2f posA = a->pos, posB = b->pos;
	float radiusA = a->radius, radiusB = b->radius;
	sf::Vector2f rectA = a->rectSize, rectB = b->rectSize;

	int distType = calc(a->shape, b->shape);
	switch (distType)
	{
	case PR:
		return aabb_distance_point(posA, posB, rectB);
	case RP:
		return aabb_distance_point(posB, posA, rectA);
	case PC:
		return vec_dist_circles(posA, 0.0f, posB, radiusB);
	case CP:
		return vec_dist_circles(posA, radiusA, posB, 0.f);
	case CR:
		return aabb_distance_circle(posA, radiusA, posB, rectB);
	case RC:
		return aabb_distance_circle(posB, radiusB, posA, rectA);
	case PP:
		return vec_dist(posA, posB);
	case CC:
		return vec_dist_circles(posA, radiusA, posB, radiusB);
	case RR:
		return aabb_distance_rectangle(posA, rectA, posB, rectB);
	default:
		return -1.f;
	}
}

void GameData::damage_body(GameBody *a, float damage)
{
	if (!a)
	{
		WARNING("Trying to damage a null body");
		return;
	}

	a->get_hp() -= (int)damage;
	/*
	switch (a->type)
	{
	case BodyType::BUILDING:
	{
		BuildingBody *b =
			dynamic_cast<BuildingBody *>(a);
		b->base->hp -= damage;
	}
	break;
	case BodyType::ENTITY:
	{
		EntityBody *b =
			dynamic_cast<EntityBody *>(a);
		b->hp -= damage;
	}
	break;
	default:
		WARNING("Damage switch case for body type %d is unimplemented", (int)a->type);
	}
	*/
}

bool GameData::body_is_collision(
	const sf::Vector2f &pos,
	const sf::Vector2f &size,
	sf::Vector2i *tilePosPtr)
{
	int x1 = (int)floorf(pos.x - size.x / 2.f), x2 = (int)ceilf(pos.x + size.x);
	int y1 = (int)floorf(pos.y - size.y / 2.f), y2 = (int)ceilf(pos.y + size.y);

	float bestDist = FLT_MAX;
	sf::Vector2i bestPos;

	bool ret = false;

	for (int i = x1; i <= x2; ++i)
	{
		for (int j = y1; j <= y2; ++j)
		{
			sf::Vector2f tilePos = {(float)i, (float)j};
			tilePos += sf::Vector2f{0.5f, 0.5f};
			sf::Vector2f tileSize = {1.f, 1.f};

			if (!chunks->has_tile(i, j))
			{
				bestPos = {i, j};
				ret = true;
				continue;
			}

			if ((chunks->get_tile(i, j).is_barrier() &&
				 aabb_collision_center(pos, size, tilePos, tileSize)))
			{
				float dist = vec_distsq(tilePos, pos);
				if (dist < bestDist)
				{
					bestDist = dist;
					bestPos = {i, j};
					ret = true;
				}
			}
		}
	}

	if (ret && tilePosPtr)
		*tilePosPtr = bestPos;

	return ret;
}


GameBody* GameData::add_game_body(
	const size_t serializableType,
	const t_group configMapGroup,
	const t_id configMapType,
	const FVec& pos)
{
	if (!bodyConfigMap.count(configMapGroup, configMapType))
	{
		WARNING("Config of {%d, %d} was not found, aborting adding the body.", 
			(int)configMapGroup,
			(int)configMapType);
		assert(false);
		return nullptr;
	}
	GameBodyConfig* config = bodyConfigMap.at(
		configMapGroup, configMapType)[0];
	return add_game_body(serializableType, config, pos);
}

GameBody* GameData::add_game_body(
	const size_t serializableType,
	GameBodyConfig* config,
	const FVec& pos)
{
	// Create a local disposable SerializeMap so it could create the object
	// Since a new object should have no relations to other, SerializeMap can refrence only this one object.
	SerializeMap map;
	Variant* t = nullptr;
	map.add(
		SERIALIZABLE_DATA,
		0,
		dynamic_cast<Variant*>(this));
	t = variantFactory.create(map, serializableType);
	GameBody* gb = dynamic_cast<GameBody*>(t);

	gb->pos = pos;

	// Must be called before initializations
	gb->apply_data(config);

	// The map is not needed because the object does not need to initialize
	// any other Variant objects
	/*
	serialize_publish ->
	serialize_publish_derived.
	*/
	t->serialize_publish(map);

	/*
	serialize_initialize ->
	serialize_initialize_derived ->
	confirm_body ->
	confirm_body_derived.
	*/

	t->serialize_initialize(map);

	// Should be called in  serialize_initialize
	// gb->confirm_body(this);

	return gb;
}


void GameData::queue_citizen(BuildingBase* build)
{
	if (build->buildType != (t_id)BuildingType::HOME)
	{
		WARNING("Only homes can append citizens.");
	}

	entityQueue.push_front(build);
}

bool GameData::queue_add_build(
	const sf::Vector2i &pos,
	UpgradeTree *step,
	bool construction,
	bool useResources)
{
	BuildingQueueData data;
	data.pos = pos;
	data.step = step;
	data.buildType = (t_build_enum)step->type.id;
	data.useResources = useResources;
	data.isConstruction = construction;

	if (useResources)
	{
		Resources costTmp = step->build;
		calculate_resources();
		if (costTmp.affordable(resources))
			return false;
	}

	buildingQueue.push_front(std::move(data));
	return true;
}

bool GameData::queue_add_build(
	const sf::Vector2i &pos,
	const std::string &key,
	bool construction,
	bool useResources)
{
//	UpgradeTree *step = nullptr;
	WARNING("todo");
	return false;
/*
	if (!mapEnumUpgrade.get_group(0).has(key))
	{
		return false;
	}
	step = &mapEnumUpgrade.get_group(0).get(key);
	
	
		x.step = dynamic_cast<UpgradeTree*>(
			g->bodyConfigMap.at(
			{(t_id)ENUM_BUILDING_TYPE, (t_id)x.buildType}));
			
	queue_add_build(pos, step, construction, useResources);
	return true;
	*/
}

bool GameData::queue_add_build(
	const sf::Vector2i &pos,
	BuildingType type,
	bool construction,
	bool useResources)
{
	UpgradeTree *step = nullptr;
	
	if (!bodyConfigMap.count(
		ENUM_BUILDING_TYPE, (t_id)type))
	{
		return false;
	}
	step = dynamic_cast<UpgradeTree*>(
		bodyConfigMap.at(ENUM_BUILDING_TYPE, (t_id)type)[0]);
			
	return queue_add_build(pos, step, construction, useResources);
}

bool GameData::queue_add_body(BodyQueueData& data)
{
	bodyQueue.push_back(data);
	return true;
}

void GameData::delete_game_body_generic(GameBody* gb)
{
	gb->set_target(nullptr);

	// Clear all targets from this object
	if (gb->followers.size())
	{
		// Since set_target affects gb->followers, 
		// Make a copy to avoid undefined behaviour
		std::vector<VariantPtr<GameBody>> followers =
			gb->followers;

		for (VariantPtr<GameBody> followerBody : followers)
		{
			assert(followerBody.get());
			// followerBody->target = nullptr;
			followerBody->set_target(nullptr, false);
		}

		// Should be cleared by now
		//gb->followers.clear();
	}

	delete gb;
}

std::string GameData::spawn_info_to_string(const SpawnInfo& spawnInfo)
{
		std::string ret = "";
		ret += std::to_string(spawnInfo.serializableID);
		ret += ", ";
		ret += mapEnumTree.get_str(spawnInfo.id.group, spawnInfo.id.id);
		return ret;
	
}

SpawnInfo GameData::spawn_info_get(size_t serializableId, const t_idpair& pair)
{
	SpawnInfo out;
	out.serializableID = serializableId;
	out.id = pair;
	return out;
}

bool GameData::can_build(const BuildingQueueData &data)
{
	auto &step = data.step;
	auto &pos = data.pos;
	auto &useResources = data.useResources;

	// Check if all needed tile are available
	for (int x = 0; x < step->size.x; ++x)
		for (int y = 0; y < step->size.y; ++y)
			if (!this->chunks->has_tile(pos.x + x, pos.y + y) ||
				this->chunks->get_tile(pos.x + x, pos.y + y).building)
				return false;

	if (useResources)
	{
		Resources cost = step->build;
		calculate_resources();

		calculate_resources();
		if (!this->resources.affordable(cost))
		{
			// DEBUG("%s %s", CSTR(resources), CSTR(cost));
			return false;
		}
	}

	return true;
}

std::vector<BuildingBase *>::iterator GameData::delete_building(BuildingBase *build)
{

	assert(build);
	assert(build->get_body());

	auto pos = build->get_body()->tilePos;
	LOG("Deleting build \"%s\" on: %d %d", build->name, pos.x, pos.y);

	for (VariantPtr<GameBody> other : build->followers)
	{
		other->set_target(nullptr);
		if (other->type == BodyType::ENTITY)
		{
			EntityBody *oe = dynamic_cast<EntityBody *>(other.get());
			assert(oe);
			oe->reset();
		}
	}

	// Reset all workers
	// MUST HAPPEN BEFORE REMOVING BODIES
	AAAA;
	for (auto itr = build->entities.begin();
		itr != build->entities.end();)
	{
		EntityBody* entity = itr->get();

		entity->force_stop();
		AAAA;
		if (entity->entityType == EntityType::CITIZEN)
		{
			auto citizen = dynamic_cast<EntityCitizen*>(itr->get());
			AAAA;
			if (citizen->workplace == build)
			{
				AAAA;
				if (build->props.bool_is(PropertyBool::INSIDE))
				{
					AAAA;
					bool hasInside = false;
					for (auto& e : build->storedEntities)
					{
						AAAA;
						if (e.get() == (EntityBody*)citizen)
						{
							hasInside = true;
							break;
						}
					}
					DEBUG("%d", hasInside);
					if (hasInside)
						show_entity(citizen);
					AAAA;
				}

				if (citizen->insideWorkplace)
					build->exit_entity(citizen);
				itr = build->remove_entity(citizen);
			}
			else
				++itr;
		}
	}

	for (auto& e : build->storedEntities)
	{
		e->visible = true;
	}

	build->storedEntities.clear();

	// If the building is a home, make the entities homeless
	if (build->props.bool_is(PropertyBool::HOME))
	{
		for (auto& e : build->entities)
		{
			dynamic_cast<EntityCitizen*>(e.get())->home = nullptr;
		}
	}
	// Remove all workers from building
	build->entities.clear();

	// Remove all bodies
	for (BuildingBody *body : build->get_bodies())
	{
		sf::Vector2i &tilePos = body->tilePos;
		removedTiles.push_back(tilePos);
		body->dead = true;
		this->buildings.erase(
			std::find(this->buildings.begin(),
			this->buildings.end(),
			body));
		Tile &tile = chunks->get_tile(tilePos.x, tilePos.y);
		tile.building = nullptr;

		// Erase that buiding pointer from the list and the quadTree
		for (auto& pair : get_trees(body))
		{
			t_tiletree *tree = pair.second;

			//std::lock_guard<std::recursive_mutex> glock(quadTreeMutex);
			bool b = tree->remove(tilePos, body);
			if (!b)
			{
				LOG_ERROR("Can't delete Building %s in %s", build->name, vec_str(tilePos).c_str());
				t_tiletree::printQuadTree(tree->root);
				ASSERT_ERROR(b, "Can't delete");
			}
		}
		
		body->dead = true;
		// delete body;
	}

	/*
	for (auto& x : mapEnumTree.groups)
	{
		for (auto& y : x.idTMap)
		{
			t_tiletree& tree = y.second;
			for (GameBody* x : tree)
			{
				DEBUG("a");
				assert(x);
			}
		}
	}
	*/

	constructions.erase_value(build);

	auto itrBuildBase = std::find(
		this->buildingBases.begin(),
		this->buildingBases.end(),
		build);
	ASSERT_ERROR(itrBuildBase != buildingBases.end(),
				 "Deleted building base cannot be found in the buildings list.");
	auto itr = buildingBases.erase(itrBuildBase);
	delete build;

	return itr;
}

std::vector<BuildingBase *>::iterator GameData::delete_building(const sf::Vector2i &pos)
{
	// Find tile
	Tile &tile = this->chunks->get_tile(pos.x, pos.y);
	assert(tile.building);
	BuildingBase *build = tile.building->base;

	return delete_building(build);
}

bool GameData::confirm_building_base(BuildingBase *build)
{
	Vec<int> pos = build->tilePos;

	LOG("Building build \"%s\" on: %d %d", build->name, pos.x, pos.y);

	// Add to bases list
	this->buildingBases.push_back(build);

	build->network = nullptr;
	build->context = this;

	// Assign and update power network
	bool isNetwork = build->props.bool_is(PropertyBool::POWER_NETWORK);
	if (!isNetwork)
		return true;

	// Search for nearest network
	auto nearest = nearest_buildings_radius_quad(
		(sf::Vector2f)pos + sf::Vector2f{0.5f, 0.5f},
		1,
		(int)get_const(t_constflt::MAX_NETWORK_RADIUS),
		t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::POWER_NETWORK},
		[build](const sf::Vector2i &, const GameBody *gb, const int dist) {
			const BuildingBody *other = dynamic_cast<const BuildingBody *>(gb);
			const BuildingBase *base = other->base;
			if (base->network)
			{
				return dist < powf(math_min(
									   base->effectRadius,
									   build->effectRadius),
								   2);
			}
			return false;
		});

	constexpr bool ONE_NETWORK = false;
	// If there's a nearby network
	if (nearest.size())
	{
		build->network = nearest.front()->base->network;
		LOG("%s", nearest.front()->base->name);
	}
	else if (networks.size() && ONE_NETWORK)
	{
		// Use an existing network
		build->network = networks.back();
	}
	else
	{
		// Create new network
		networks.push_back(new PowerNetwork(objectIdCounter.advance<PowerNetwork>()));
		build->network = networks.back();
	}

	// If the building doesn't generates electricity
	if (build->powerOut == 0 || build->entityLimit == 0)
	{
		// Add to the network
		build->network->add(build);
	}
	else
	{
		// Enumurate through the 3 types of power building:
		// IN, OUT and STORE
		for (int i = 0; i < 3; ++i)
		{
			PowerNetwork::Use u = (PowerNetwork::Use)i;
			assert(build->network);
			if (!PowerNetwork::resource(
					build,
					(PowerNetwork::Use)i))
				continue;
			// Add the building to it's corresponding use'
			build->network->add(build, u, false);
		}
	}

	// Search and combine networks
	// todo: there's must be a more efficient way
	// two combine two networks
	nearest = nearest_buildings_radius_quad(
		(sf::Vector2f)pos + sf::Vector2f{0.5f, 0.5f},
		SMALL_INFINITY,
		(int)get_const(t_constflt::MAX_NETWORK_RADIUS),
		t_idpair{ENUM_PROPERTY_BOOL, (t_id)PropertyBool::POWER_NETWORK},
		[build](const sf::Vector2i &, const GameBody *gb, const int dist) {
			const BuildingBody *other = dynamic_cast<const BuildingBody *>(gb);
			const BuildingBase *base = other->base;
			float maxD = math_max(base->effectRadius, build->effectRadius);
			return dist < maxD;
		});

	for (auto &bodyBuild : nearest)
	{
		// Merge first network with the second
		if ((intptr_t)bodyBuild->base->network != (intptr_t)build->network)
			PowerNetwork::merge(bodyBuild->base->network, build->network);
		// todo better performance?

		// Replace old network with new one for all buildings
		for (auto &treeBody : *get_tree(ENUM_PROPERTY_BOOL, (t_id)PropertyBool::POWER_NETWORK))
		{
			BuildingBase *base = dynamic_cast<BuildingBody *>(treeBody)->base;
			if (base->network == build->network)
				base->network = bodyBuild->base->network;
		}

		// Remove deleted network from the container
		*std::find(
			networks.begin(),
			networks.end(),
			build->network) = bodyBuild->base->network;
	}

	return true;
}

BuildingBase* GameData::add_building(const BuildingQueueData& queue, bool updatePath)
//const sf::Vector2i &pos, UpgradeTree *step, bool useResources)
{
	// Check if building is possible
	if (!can_build(queue))
	{
		WARNING("Can't build on tile: %s ",
			vec_str(queue.pos).c_str());
		return nullptr;
	}

	auto& pos = queue.pos;
	UpgradeTree* step = queue.step;
	assert(step);
	auto& useResources = queue.useResources;

	sf::Vector2i size = step->size;

	// Subtruct build cost from storage buildings
	if (useResources)
	{
		Resources costTmp = step->build;

		// Use the specialized quad tree for more efficiency.
		t_tiletree* tree = get_tree(ENUM_PROPERTY_BOOL, (t_id)PropertyBool::ANY_STORAGE);

		for (auto itr = tree->begin(); itr != tree->end(); ++itr)
		{
			BuildingBody* body = dynamic_cast<BuildingBody*>(*itr);
			BuildingBase* storageBuild = body->base;
			if (Resources::use(storageBuild->rStorage, costTmp))
				break;
		}
		ASSERT_ERROR(costTmp.empty(), "");
	}

	// If building is for construction, replace is with construction building
	if (queue.isConstruction &&
		step->buildWork > 0)
	{
		const t_id id = (t_id)BuildingType::CONSTRUCTION;
		IdPair idPair{ ENUM_BUILDING_TYPE, id };
		if (!bodyConfigMap.count(idPair.group, idPair.id))
			assert(0);
		UpgradeTree* newStep = dynamic_cast<UpgradeTree*>(
			bodyConfigMap.at(idPair.group, idPair.id)[0]);
		step = newStep;
	}

	// Initialized build
	BuildingBase* build = new BuildingBase(
		this,
		pos,
		step,
		step->type.id,
		objectIdCounter.advance<BuildingBase>());
	if (!build)
	{
		return nullptr;
	}

	// For construction buildingings
	build->tilesSize = size;
	//DEBUG("%s", VEC_CSTR(build->tilesSize));

	LOG("Building build \"%s\" on: %d %d", build->name, pos.x, pos.y);

	// Confirm that the path hs been changed
	if (updatePath)
		for (int x = 0; x < build->tilesSize.x; ++x)
			for (int y = 0; y < build->tilesSize.y; ++y)
				addedTiles.push_back({ x, y });
	//DEBUG("%s", VEC_CSTR(build->tilesSize));

	// Add construction data
	if (queue.isConstruction)
	{
		constructions.insert(
			build,
			ConstructionData(
				queue,
				objectIdCounter.advance<ConstructionData>()));
	}
	confirm_building_base(build);

	// Initialize every tile
	for (int x = 0; x < build->tilesSize.x; ++x)
	{
		for (int y = 0; y < build->tilesSize.y; ++y)
		{
#ifndef NDEBUG
			assert(this->chunks->has_tile(pos.x + x, pos.y + y));
#endif
			sf::Vector2i tileIndex{x, y};

			t_sprite sprite = -1;
			auto itrSprite = step->spriteHolders.find(tileIndex);
			if (itrSprite != step->spriteHolders.end())
				sprite = itrSprite->second;
			else if (
				(itrSprite =
					 step->spriteHolders.find({0, 0})) !=
				step->spriteHolders.end())
			{
				sprite = itrSprite->second;
			}

			BuildingBody *body = new BuildingBody(
				this,
				pos + tileIndex,
				build,
				sprite,
				tileIndex,
				objectIdCounter.advance<BuildingBody>());

			body->confirm_body(this);
		}
	}

	return build;
}

BuildingBase *GameData::add_building(const sf::Vector2i &pos, UpgradeTree *step, bool useResources)
{
	return add_building(BuildingQueueData{pos, step, useResources}, false);
}

BuildingBase *GameData::add_building(const sf::Vector2i &pos, const std::string &key, bool useResources)
{
	bool success = false;
	t_idpair pair = mapEnumTree.get_id(key, &success);
	if (!success || pair == IDPAIR_NONE)
	{
		WARNING("Unable to find building %s in the enum tree\n", key.c_str());
		assert(0);
		return nullptr;
	}
	if (!bodyConfigMap.count(pair.group, pair.id))
	{
		WARNING("Unable to find building %s in the enum tree\n", key.c_str());
		assert(0);
		return nullptr;
	}
	UpgradeTree *step = nullptr;
	step = dynamic_cast<UpgradeTree*>(
		bodyConfigMap.at(pair.group, pair.id)[0]);
	assert(step);
	return add_building(pos, step, useResources);
}

BuildingBase *GameData::add_building(const sf::Vector2i &pos, BuildingType type, bool useResources)
{
	IdPair idPair{ENUM_BUILDING_TYPE, (t_id)type};
	if (!bodyConfigMap.count(idPair.group, idPair.id))
	{
		WARNING("Unable to find building %d\n", (int)type);
		//assert(0);
		return nullptr;
	}
	UpgradeTree *step = nullptr;
	step = dynamic_cast<UpgradeTree*>(
			bodyConfigMap.at(idPair.group, idPair.id)[0]);
	assert(step);
	return add_building(pos, step, useResources);
}

bool GameData::is_barrier(const sf::Vector2i &pos)
{
	return !chunks->has_tile(pos.x, pos.y) ||
		   chunks->get_tile(pos.x, pos.y).is_barrier();
};

bool GameData::get_free_neighbor(sf::Vector2f& outPos, const FVec& pos)
{
	sf::Vector2i ipos = vec_pos_to_tile(pos);
	Tile& tile = chunks->get_tile(ipos.x, ipos.y);

	sf::Vector2i tileSize = { 1, 1 };
	sf::Vector2i tilePos = ipos;

	if (!tile.is_barrier())
	{
		outPos = pos;
		return true;
	}

	if (tile.building != nullptr)
	{
		tilePos = tile.building->base->tilePos;
		tileSize = tile.building->base->tilesSize;
	}
	
	std::vector<sf::Vector2i> pendingPositions;

	for (const auto& x : DIRECTIONS)
	{
		pendingPositions.push_back( x);
	}

	for (int i = 0; i < pendingPositions.size(); ++i)
	{
		sf::Vector2i pos = pendingPositions[(i + genIndex++) % pendingPositions.size()];
		pos += tilePos;
		
		if (chunks->has_tile(pos.x, pos.y) &&
			!chunks->get_tile(pos.x, pos.y).is_barrier())
		{
			outPos = (sf::Vector2f)pos + sf::Vector2f{ .5f, .5f };
			return true;
		}
	}

	return false;
}

EntityCitizen *GameData::add_entity_citizen(
	const sf::Vector2f &pos,
	EntityStats *stats)
{
	EntityCitizen *e = new EntityCitizen(
		this,
		pos, 
		-1, 
		objectIdCounter.advance<EntityCitizen>());
	e->apply_data(dynamic_cast<GameBodyConfig *>(stats));
	// todo
	e->spriteHolder = e->spriteWalk;
	return (e->confirm_body(this) ? e : nullptr);
}

EntityCitizen* GameData::add_entity_citizen(
	BuildingBase* build)
{
	assert(build);
	EntityCitizen* e = nullptr;

	if (build->entities.size() >= build->entityLimit)
		return e;

	++build->entitiesSpawned;
	sf::Vector2i& tileSize = build->tilesSize;
	sf::Vector2i& tilePos = build->tilePos;

	std::vector<sf::Vector2i> pendingPositions;

	const auto itrTiles = [this, &pendingPositions](
		int start, int end,
		int mx, int my,
		int px, int py) {
			for (int i = (start); i <= (end); ++i)
			{
				sf::Vector2i pos = sf::Vector2i{
					i * mx + px,
					i * my + py };
				if (chunks->has_tile(pos.x, pos.y) &&
					!chunks->get_tile(pos.x, pos.y).is_barrier())
					pendingPositions.push_back(pos);
			}
	};

	const auto checkFreeSpace =
		[this, &e](
			const sf::Vector2i& posi,
			BuildingBase* build)
	{
		if (!chunks->get_tile(posi.x, posi.y).is_barrier())
		{
			DEBUG("Build job: %d", (int)build->job);
			EntityStats* stats = get_entity_stats(
				ENUM_CITIZEN_JOB,
				(t_id)build->job);
			e = add_entity_citizen(
				(sf::Vector2f)posi + sf::Vector2f{ .5f, .5f },
				stats);
			e->home = build;
			build->entities.push_back(e);
			return true;
		}
		return false;
	};

	itrTiles(-1, tileSize.x + 1, 1, 0, 0, -1);
	itrTiles(-1, tileSize.x + 1, 1, 0, 0, tileSize.y);
	itrTiles(0, tileSize.y, 0, 1, -1, 0);
	itrTiles(0, tileSize.y, 0, 1, tileSize.x, 0);

	for (int i = 0; i < pendingPositions.size(); ++i)
	{
		sf::Vector2i pos = pendingPositions[(i + genIndex++) % pendingPositions.size()];
		pos += tilePos;
		if (checkFreeSpace(pos, build))
			break;
	}

	return e;
}

BulletBody* GameData::add_bullet(
	const FVec& pos,
	const FVec& dir,
	EntityStats* stats)
{
	BulletBody* e = new BulletBody(
		this, objectIdCounter.advance<BulletBody>(),
		pos, dir, 
		-1, (t_id)EnumAlignment::NEUTRAL);
	if (stats)
		e->apply_data(dynamic_cast<GameBodyConfig*>(stats));
	return (e->confirm_body(this) ? e : nullptr);
}


EntityStats *GameData::get_entity_stats(
	const std::string &name,
	bool *success)
{
	t_group group;
	t_id id;
	bool success2;
	// Convert json string represention of an enum to idpair
	mapEnumTree.get_id(
		group,
		id,
		name,
		&success2);
	if (!success)
	{
		if (success)
			*success = false;
		if (bodyConfigMap.count(ENUM_ENTITY_TYPE, 0))
		 return get_entity_stats(
		 	ENUM_ENTITY_TYPE, 
		 	0, 
		 	success);
	}
	return get_entity_stats(group, id, success);
}

EntityStats *GameData::get_entity_stats(
	t_group group,
	t_id id,
	bool *success)
{
	if (!bodyConfigMap.count(group, id))
	{
		if (success)
			*success = false;
		return nullptr;
	}
	if (success)
		*success = true;
	return dynamic_cast<EntityStats*>(
		bodyConfigMap.at(group, id)[0]);
}

EntityEnemy *GameData::add_entity_enemy(
	const sf::Vector2f &pos,
	t_sprite sprite)
{
	EntityEnemy *e = new EntityEnemy(
		this, pos, sprite, objectIdCounter.advance<EntityEnemy>());
	return (e->confirm_body(this) ? e : nullptr);
}

EntityEnemy *GameData::add_entity_enemy(
	const FVec &pos,
	EntityStats *stats)
{
	if (!stats)
		return nullptr;
	assert(stats);
	EntityEnemy *e = new EntityEnemy(
		this, pos, stats->spriteWalk, objectIdCounter.advance<EntityEnemy>());
	e->apply_data(stats);
	return (e->confirm_body(this) ? e : nullptr);
}

constexpr bool IGNORE_ASSERT = 1;

void GameData::move_entity(EntityBody *entity, const sf::Vector2f &posA, const sf::Vector2f &posB)
{
	assert(entity);

	sf::Vector2i iposA = vec_pos_to_tile(posA);
	sf::Vector2i iposB = vec_pos_to_tile(posB);

	for (auto& pair : get_trees(entity))
	{
		t_tiletree* tree = pair.second;
		//std::lock_guard<std::recursive_mutex> glock(quadTreeMutex);
		bool bol = tree->move(iposA, iposB, dynamic_cast<GameBody*>(entity));
		if (!bol)
		{
			t_tiletree::printQuadTree(tree->root);

			WARNING("%s -> %s", vec_str(iposA).c_str(), vec_str(iposB).c_str());

			if (tree->get_pair(iposA).first)
			{
				DEBUG("Found entity: %s %p",
					vec_str(tree->get_pair(iposA).second->pos).c_str(),
					(void*)tree->get_pair(iposA).second->type);
				DEBUG("Expected: %s %p",
					vec_str(iposA).c_str(),
					(void*)entity);
			}

			tree->_tmpDebug = 1;
			tree->get_pair(iposA);
			tree->_tmpDebug = 0;

			assert(IGNORE_ASSERT | (bool)tree->get_pair(iposA).first);
			assert(IGNORE_ASSERT | (bool)tree->get_pair(iposA, entity).first);
			assert(IGNORE_ASSERT | (bool)tree->get_pair(iposA, entity).second->type);
			assert(IGNORE_ASSERT | bol);
		}
	}

	if (iposA != iposB)
	{
		assert(chunks);
		Grid* gridA = chunks->get_grid(iposA.x, iposA.y);
		Grid* gridB = chunks->get_grid(iposB.x, iposB.y);
		assert(gridA);
		if (gridA && !gridB)
			return;
		if (gridA != gridB)
		{
			gridA->remove_entity(entity);
			gridB->insert_entity(entity);
		}
		//assert(chunks->has_tile(iposA.x, iposA.y));
		Tile& a = chunks->get_tile(iposA.x, iposA.y);
		//assert(chunks->has_tile(iposB.x, iposB.y));
		Tile& b = chunks->get_tile(iposB.x, iposB.y);
		assert(a.remove_entity(entity));
		b.entities.push_back(entity);
	}

	entity->pos = posB;
}

void GameData::move_body(GameBody* body, const sf::Vector2f& posA, const sf::Vector2f& posB)
{
	assert(body);

	sf::Vector2i iposA = vec_pos_to_tile(posA);
	sf::Vector2i iposB = vec_pos_to_tile(posB);

	for (auto& pair : get_trees(body))
	{
		t_tiletree* tree = pair.second;
		//std::lock_guard<std::recursive_mutex> glock(quadTreeMutex);
		bool bol = tree->move(iposA, iposB, body);
		if (!bol)
		{
			t_tiletree::printQuadTree(tree->root);

			WARNING("%s -> %s",
				vec_str(iposA).c_str(),
				vec_str(iposB).c_str());

			if (tree->get_pair(iposA).first)
			{
				DEBUG("Found entity: %s %p",
					vec_str(tree->get_pair(iposA).second->pos).c_str(),
					(void*)tree->get_pair(iposA).second->type);
				DEBUG("Expected: %s %p",
					vec_str(iposA).c_str(),
					(void*)body);
			}

			tree->_tmpDebug = 1;
			tree->get_pair(iposA);
			tree->_tmpDebug = 0;

			assert(IGNORE_ASSERT | (bool)tree->get_pair(iposA).first);
			assert(IGNORE_ASSERT | (bool)tree->get_pair(iposA, body).first);
			assert(IGNORE_ASSERT | (bool)tree->get_pair(iposA, body).second->type);
			assert(IGNORE_ASSERT | bol);
		}
	}

	if (iposA != iposB)
	{
		assert(chunks);
		Grid* gridA = chunks->get_grid(iposA.x, iposA.y);
		Grid* gridB = chunks->get_grid(iposB.x, iposB.y);
		assert(gridA);
		if (gridA && !gridB)
			return;
		if (gridA != gridB)
		{
			if (body->type == BodyType::ENTITY)
			{
				EntityBody* entity = dynamic_cast<EntityBody*>(body);
				gridA->remove_entity(entity);
				gridB->insert_entity(entity);
			}
		}
		//assert(chunks->has_tile(iposA.x, iposA.y));
		Tile& a = chunks->get_tile(iposA.x, iposA.y);
		//assert(chunks->has_tile(iposB.x, iposB.y));
		Tile& b = chunks->get_tile(iposB.x, iposB.y);

		if (body->type == BodyType::ENTITY)
		{
			EntityBody* entity = dynamic_cast<EntityBody*>(body);
			assert(a.remove_entity(entity));
			b.entities.push_back(entity);
		}
	}
	
	body->pos = posB;
}

void GameData::delete_entity(EntityBody*e)
{
	sf::Vector2i ipos = vec_pos_to_tile(e->pos);

	chunks->get_grid(ipos.x, ipos.y)->remove_entity(e);

	// mapEnumTree

	if (this->chunks->has_tile(ipos.x, ipos.y))
	{
		Tile &t = chunks->get_tile(ipos.x, ipos.y);
		t.remove_entity(e);

		// horrible! just bcuase an entity is not visible does not exists it doesn't exists at all
		if (e->visible)
		{
			for (auto& pair : get_trees(e))
			{
				auto tree = pair.second;
				std::lock_guard<std::recursive_mutex> glock(quadTreeMutex);
				ASSERT_ERROR(IGNORE_ASSERT | tree->remove(ipos, e), "QuadTree Citizen remove failed");
			}
		}
	}

	switch (e->entityType)
	{
	case EntityType::ENEMY:
	{
		EntityEnemy *enemy = dynamic_cast<EntityEnemy *>(e);
		break;
	}
	case EntityType::CITIZEN:
	{
		EntityCitizen *citizen = dynamic_cast<EntityCitizen *>(e);
		if (citizen->workplace)
		{
			if (citizen->insideWorkplace)
				citizen->workplace->exit_entity(citizen);
			citizen->workplace->remove_entity(citizen);
		}
		if (citizen->home)
		{
			BuildingBase *home;
			home = citizen->home;
			auto itr = std::find(
				home->entities.begin(),
				home->entities.end(),
				citizen);
			assert(itr != home->entities.end());
			home->entities.erase(itr);
			home->actionTimer->reset(get_time());
		}

		break;
	}
	default:
		break;
	}
}

void GameData::show_entity(EntityBody*e)
{
	e->visible = true;

	sf::Vector2i ipos = vec_pos_to_tile(e->pos);
	for (auto &pair : get_trees(e))
	{
		auto tree = pair.second;
		std::lock_guard<std::recursive_mutex> glock(quadTreeMutex);
		ASSERT_ERROR(IGNORE_ASSERT | tree->insert(ipos, e), "QuadTree insert failed");
		assert(tree->get_pair(ipos, e).second->type);
	}

	Grid* gridA = chunks->get_grid(ipos.x, ipos.y);
	gridA->insert_entity(e);
}

void GameData::hide_entity(EntityBody*e)
{
	e->visible = false;

	IVec ipos = vec_pos_to_tile(e->pos);
	for (auto &pair : get_trees(e))
	{
		auto tree = pair.second;
		std::lock_guard<std::recursive_mutex> glock(quadTreeMutex);
		bool b = tree->remove(ipos, e);
		if (!b)
		{
			t_tiletree::printQuadTree(tree->root);
			ASSERT_ERROR(IGNORE_ASSERT | b, "QuadTree remove failed");
		}
	}

	Grid* gridA = chunks->get_grid(ipos.x, ipos.y);
	gridA->remove_entity(e);

	for (GameBody* follower : e->followers)
	{
		EntityBody* eb = (EntityBody*)(follower);
		follower->set_target(nullptr);
	}
}

std::list<EntityBody*> GameData::nearest_entities_radius(
	const sf::Vector2f &pos,
	float radius,
	EntityType type,
	size_t limit)
{
	return nearest_bodies_radius<EntityBody>(
		pos,
		radius,
		limit,
		[type](GameBody *body, int, int) {
			if (body->type != BodyType::ENTITY)
				return false;
			auto entity = dynamic_cast<EntityBody*>(body);
			return entity->entityType == type;
		});
}

std::list<BuildingBody *>
GameData::nearest_buildings_aabb(const sf::Vector2f &pos, float radius, size_t limit)
{
	const auto pairLess = [pos](const BuildingBody *a, const BuildingBody *b) {
		return aabb_distance_point(pos, a->pos, a->rectSize) <
			   aabb_distance_point(pos, b->pos, b->rectSize);
	};
	std::list<BuildingBody *> ret;

	float radiusSq = radius * radius;
	int x1 = (int)floorf(pos.x - radius), x2 = (int)floorf(pos.x + radius);
	int y1 = (int)floorf(pos.y - radius), y2 = (int)floorf(pos.y + radius);
	for (int i = x1; i <= x2; ++i)
	{
		for (int j = y1; j <= y2; ++j)
		{
			Tile &tile = this->chunks->get_tile(i, j);
			if (!tile.building)
				continue;
			BuildingBody *b = tile.building;
			float dist = aabb_distance_point(pos, b->pos, {1.f, 1.f});
			if (dist >= radiusSq)
				continue;
			ret.insert(std::lower_bound(ret.begin(), ret.end(), b, pairLess), b);
		}
	}

	if (ret.size() > limit)
		ret.erase(std::next(ret.begin(), limit), ret.end());
	return ret;
}

std::list<BuildingBody *> GameData::nearest_buildings_radius(const sf::Vector2f &pos, float radius, size_t limit)
{
	return nearest_bodies_radius<BuildingBody>(
		pos,
		radius,
		limit,
		[](GameBody *body, int, int) {
			return body->type == BodyType::BUILDING;
		});
}

std::list<BuildingBody *> GameData::nearest_buildings_aabb(const sf::Vector2f &pos, const sf::Vector2f &size, const float radius, const size_t limit)
{
	const auto pairLess = [pos, size](const BuildingBody *a, const BuildingBody *b) {
		return aabb_distance_rectangle(pos, size, a->pos, a->rectSize) <
			   aabb_distance_rectangle(pos, size, b->pos, b->rectSize);
	};
	std::list<BuildingBody *> ret;

	float radiusSq = radius * radius;
	int x1 = (int)floorf(pos.x - size.x / 2 - radius), x2 = (int)floorf(pos.x + size.x / 2 + radius);
	int y1 = (int)floorf(pos.y - size.y / 2 - radius), y2 = (int)floorf(pos.y + size.y / 2 + radius);
	for (int i = x1; i <= x2; ++i)
	{
		for (int j = y1; j <= y2; ++j)
		{
			if (!this->chunks->has_tile(i, j))
				continue;

			Tile &tile = this->chunks->get_tile(i, j);
			if (!tile.building)
				continue;
			BuildingBody *b = tile.building;
			float dist = aabb_distance_rectangle(
				pos,
				size,
				b->pos,
				b->rectSize);
			if (dist >= radiusSq)
				continue;
			ret.insert(std::lower_bound(ret.begin(), ret.end(), b, pairLess), b);
		}
	}

	if (ret.size() > limit)
		ret.erase(std::next(ret.begin(), limit), ret.end());
	return ret;
}

std::list<BuildingBody *> GameData::nearest_bodies_quad(const sf::Vector2f &pos, size_t limit)
{
	std::list<BuildingBody *> ret;
	auto inserter = std::back_inserter(ret);

	t_tiletree *tree = get_tree(ENUM_BODY_TYPE, (t_id)BodyType::BUILDING);

	if (tree)
	{
		tree->nearest_k_insertor<decltype(inserter), BuildingBody *>(
			vec_pos_to_tile(pos),
			limit,
			inserter);
	}
	else
	{
		assert(0);
	}

	return ret;
}