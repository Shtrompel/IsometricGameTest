#include "game/game_buildings.hpp"

#include "game/game_data.hpp"

PowerNetwork::PowerNetwork(size_t id)
	: Variant(SERIALIZABLE_NETWORK, id)
{
}

json PowerNetwork::to_json() const
{
	json j;
	j["builds"] = {output, input, station};
	j["vals"] = {powerOut, powerIn, powerStore,
				 powerValue, storeCount};
	return j;
}

void PowerNetwork::from_json(const json &j)
{
	try
	{
		j.at("builds").at(0).get_to(output);
		j.at("builds").at(1).get_to(input);
		j.at("builds").at(2).get_to(station);

		j.at("vals").at(0).get_to(powerOut);
		j.at("vals").at(1).get_to(powerIn);
		j.at("vals").at(2).get_to(powerStore);
		j.at("vals").at(3).get_to(powerValue);
		j.at("vals").at(4).get_to(storeCount);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize PowerNetwork: %s", e.what());
	}
}

void PowerNetwork::serialize_post(const SerializeMap &map)
{
	for (auto &v : output)
		map.apply(v);
	for (auto &v : input)
		map.apply(v);
	for (auto &v : station)
		map.apply(v);
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);
	this->objectId = g->objectIdCounter.advance<PowerNetwork>();
}

std::string PowerNetwork::to_string()
{
	std::stringstream ss("");
	ss << "{ ";
	for (int i = 0; i < 3; ++i)
	{
		static const std::string NAMES[3] = {"In", "Out", "Store"};
		ss << NAMES[i] << ": { ";
		ss << "Count: " << itr((Use)i).size();
		ss << ", Value: " << value((Use)i);
		ss << " }" << ((i == 2) ? ("") : (", "));
	}
	ss << " }";
	return ss.str();
}

PowerNetwork::t_builds &PowerNetwork::itr(Use use)
{
	switch (use)
	{
	case IN:
		return input;
	case OUT:
		return output;
	case STATION:
		return station;
	default:
		return input;
	}
}

int &PowerNetwork::value(Use use)
{
	switch (use)
	{
	case IN:
		return powerIn;
	case OUT:
		return powerOut;
	case STATION:
		return powerStore;
	default:
		return powerIn;
	}
}

void PowerNetwork::add(BuildingBase *base, Use use, bool changeValue)
{
	base->network = this;
	if (changeValue)
		value(use) += resource(base, use);
	auto &vec = itr(use);
	if (std::find(vec.begin(), vec.end(), base) == vec.end())
		vec.insert(
			std::upper_bound(vec.begin(), vec.end(), base),
			base);
}

void PowerNetwork::add(const t_builds &builds, Use use)
{
	for (auto &b : builds)
		add(b, use);
}

void PowerNetwork::add(BuildingBase *base)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		if (resource(base, u))
			add(base, u);
	}
}

void PowerNetwork::remove(BuildingBase *base, Use use)
{
	base->network = nullptr;
	value(use) -= resource(base, use);
	auto &vec = itr(use);
	vec.erase(
		std::remove(
			std::begin(vec),
			std::end(vec),
			base),
		std::end(vec));
}

void PowerNetwork::remove(BuildingBase *base)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		if (resource(base, u))
			remove(base, u);
	}
}

void PowerNetwork::clear()
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		itr(u).clear();
	}
}

int PowerNetwork::resource(BuildingBase *base, Use use)
{
	switch (use)
	{
	case IN:
		return base->powerIn;
	case OUT:
		return base->powerOut;
	case STATION:
		return base->powerStore;
	default:
		return base->powerIn;
	}
}

void PowerNetwork::merge(PowerNetwork &a, PowerNetwork &b)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		a.add(b.itr(u), u);
	}
	b.clear();
}

void PowerNetwork::merge(PowerNetwork *a, PowerNetwork *b)
{
	for (int i = 0; i < 3; ++i)
	{
		auto u = (PowerNetwork::Use)i;
		a->add(b->itr(u), u);
	}
	b->clear();
}

static std::vector<UpgradeTree*> recursive_get_available_tree(
	UpgradeTree *step,
	const std::vector<int> &upgrades)
{
	std::vector<UpgradeTree *> ret;

	if (!step)
		return ret;

	auto itr = std::find(
		upgrades.begin(),
		upgrades.end(),
		step->upgradeId);

	if (itr == upgrades.end())
	{
		ret.push_back(step);
	}
	else
	{
		auto &childs = step->children;
		for (auto itr = childs.begin(); itr != childs.end(); ++itr)
		{
			UpgradeTree *s = &*itr;
			merge_vectors(
				ret,
				recursive_get_available_tree(s, upgrades));
		}
	}

	return ret;
}

t_sprite UpgradeTree::get_sprite(
	const sf::Vector2i &pos, 
	bool) const
{
	auto& sprites = this->spriteHolders;
	auto itr = sprites.find(pos);
	if (itr == sprites.end())
	{
		LOG_ERROR(
			"Sprite at index %s wasn't found in tree with id of %d",
			VEC_CSTR(pos),
			type.id);
		return -1;
	}
	else
		return (*itr).second;
}

std::string UpgradeTree::to_string(GameData *context, bool verbose) const
{
	std::stringstream ret;
	ret << "{ ";
	if (id != -1 && verbose)
		ret << "ID : " << id << ", ";
	if (type != IDPAIR_NONE)
		ret << "Type : " << type.to_string() << ", ";
	if (strcmp(name, ""))
		ret << "Name : " << name << ", ";
	if (size != sf::Vector2i{-1, -1})
		ret << "Size x: " << size.x << " - " << size.y << ", ";
	if (strcmp(description, ""))
		ret << "Description : \"" << description << "\", ";
	if (hp >= 0)
		ret << "Hit Points : " << hp << ", ";
	if (entityLimit > 0)
		ret << "Entity Limit : " << entityLimit << ", ";
	if (strcmp(image, ""))
		ret << "Image : " << image << ", ";
	if (framesStart != NULL_IVEC && verbose)
		ret << "Frames Start: " << framesStart.x << "-" << framesStart.y << ", ";
	if (framesEnd != sf::Vector2i{-1, -1} && verbose)
		ret << "Frames End: " << framesEnd.x << "-" << framesEnd.y << ", ";
	if (delayAction >= 0.0f)
		ret << "Action timer delay: " << delayAction << ", ";
	if (delayCost >= 0.0f)
		ret << "Resource timer delay: " << delayCost << ", ";
	if (!build.empty())
		ret << "Build cost: " << build.to_string() << ", ";
	if (!input.empty())
		ret << "Resource Input: " << input.to_string() << ", ";
	if (!output.empty())
		ret << "Resource Output: " << output.to_string() << ", ";
	if (powerIn != NULL_INT)
		ret << "Power In: " << powerIn << ", ";
	if (powerOut != NULL_INT)
		ret << "Power Out: " << powerOut << ", ";
	if (powerStore != NULL_INT)
		ret << "Power Store: " << powerStore << ", ";
	if (!storageCap.empty())
		ret << "Resource Storage Cap: " << storageCap.to_string() << ", ";
	if (weightCap != -1)
		ret << "Resource Storage Weight Cap: " << weightCap << "\n";
	if (upgradeId != -1 && verbose)
		ret << "Upgrade ID " << upgradeId << ", ";
	if (upgradeParent != -1 && verbose)
		ret << "Upgrade Parent : " << upgradeParent << ", ";
	if (upgradePath != -1 && verbose)
		ret << "Upgrade Path : " << upgradePath << ", ";
	DEBUG("todo: die");
	if (job != CitizenJob::NONE)
	{
		std::string strJob = (context
								  ? context->mapEnumTree.get_str(
										ENUM_CITIZEN_JOB,
										(t_id)job)
								  : std::to_string((t_id)this->job));
		ret << "Job : " << strJob << ", ";
	}

	if (props.bool_size())
	{
		ret << "Uses: ";
		for (size_t i = 0; i < props.bool_size(); ++i)
		{
			std::string str = (context
								   ? context->mapEnumTree.get_str(
										 ENUM_PROPERTY_BOOL,
										 (t_id)props.bool_get(i))
								   : std::to_string((t_id)props.bool_get(i)));
			ret << str << ((i == props.bool_size() - 1) ? "" : ", ");
		}
		ret << ", ";
	}

	// todo impement PropertySet iterator
	if (props.num_size())
	{
		ret << "Properties: ";
		auto numsItr = props.nums_citr();
		for (auto itr = numsItr.begin(); itr != numsItr.end(); ++itr)
		{
			if (itr != numsItr.begin())
				ret << ", ";
			std::string str = (context
								   ? context->mapEnumTree.get_str(
										 ENUM_PROPERTY_BOOL,
										 (t_id)itr->first)
								   : std::to_string((t_id)itr->first));
			ret << str << ": " << itr->second;
		}
		ret << ", ";
	}

	// Horrible!
	if (ret.str().size() > 2)
		ret.seekp(-2, ret.cur);
	ret << " }";

	return ret.str();
}

// Building base

BuildingBase::BuildingBase()
{
}

BuildingBase::BuildingBase(
	GameData *context,
	const sf::Vector2i &pos,
	UpgradeTree *step,
	const t_build_enum &type,
	size_t id)
	: Variant(
		  SERIALIZABLE_BUILD_BASE,
		  id)
{

	// todo, change timer length to stats value
	this->context = context;
	this->tilePos = pos;
	costTimer = context->create_timer(context->get_time());
	costTimer->set_length(COST_TIMER);
	costTimer->set_length(1.f);


	actionTimer = context->create_timer(context->get_time());

	this->tree = step;
	this->buildType = type;
	this->actionTimer->set_length(1.f);
	load_upgrade_step(step);
}

UpgradeTree* BuildingBase::get_tree(t_id id)
{
	GameBodyConfig* tree = context->bodyConfigMap.at(
			ENUM_BUILDING_TYPE, (t_id)info->buildType)[id];
	return dynamic_cast<UpgradeTree*>(tree);
}

void BuildingBase::variant_initialize()
{
	ASSERT_ERROR("%s \"variant_initialize\" should not be called.", __FUNCTION__);
}

json BuildingBase::to_json() const
{
	json ret;

#define j ret
	j["id"] = id;
	j["tilePos"] = tilePos;
	j["tilesSize"] = tilesSize;
	j["name"] = name;
	j["buildType"] = buildType;
	j["job"] = job;
	j["entitiesSpawned"] = entitiesSpawned;
	j["spawn"] = spawn;
	j["alignment"] = alignment;

	j["effectRadius"] = effectRadius;
	j["weightCap"] = weightCap;
	j["entityLimit"] = entityLimit;
	j["active"] = active;
	j["sufficient"] = sufficient;
	j["rIn"] = rIn;
	j["rOut"] = rOut;
	j["rStoreCap"] = rStoreCap;
	j["powerIn"] = powerIn;
	j["powerOut"] = powerOut;
	j["powerStore"] = powerStore;
	j["props"] = props;
	j["sprites"] = sprites;

	j["hp"] = hp;
	j["lastHp"] = lastHp;
	j["operational"] = operational;
	j["flagDelete"] = flagDelete;
	j["entities"] = entities;
	j["storedEntities"] = storedEntities;
	j["costTimer"] = costTimer;
	j["actionTimer"] = actionTimer;
	j["animFrame"] = animFrame;
	j["rStorage"] = rStorage;
	j["rPending"] = rPending;
	j["powerValue"] = powerValue;
	j["lastPowerOut"] = lastPowerOut;
	j["finalPowerOut"] = finalPowerOut;
	j["binded"] = binded;
	if (network)
	{
		j["network"] = network->to_ptr_json();
	}
	j["updateInfo"] = updateInfo;
	j["followers"] = followers;
	j["upgrades"] = upgrades;

	
#undef j

	return ret;
}

void BuildingBase::from_json(const json &j)
{
	try
	{
#define _GET(str) j.find(str).value()

		_GET("id").get_to(info->id);
		_GET("tilePos").get_to(info->tilePos);
		_GET("tilesSize").get_to(info->tilesSize);
		std::string strName;
		_GET("name").get_to(strName);
		strcpy(info->name, strName.c_str());
		_GET("buildType").get_to(info->buildType);
		_GET("job").get_to(info->job);
		_GET("entitiesSpawned").get_to(info->entitiesSpawned);
		_GET("spawn").get_to(info->spawn);
		_GET("alignment").get_to(info->alignment);

		_GET("effectRadius").get_to(info->effectRadius);
		_GET("weightCap").get_to(info->weightCap);
		_GET("entityLimit").get_to(info->entityLimit);
		_GET("active").get_to(info->active);
		_GET("sufficient").get_to(info->sufficient);
		_GET("rIn").get_to(info->rIn);
		_GET("rOut").get_to(info->rOut);
		_GET("rStoreCap").get_to(info->rStoreCap);
		_GET("powerIn").get_to(info->powerIn);
		_GET("powerOut").get_to(info->powerOut);
		_GET("powerStore").get_to(info->powerStore);
		_GET("props").get_to(info->props);
		_GET("sprites").get_to(info->sprites);

		_GET("hp").get_to(data->hp);
		_GET("lastHp").get_to(data->lastHp);
		_GET("operational").get_to(data->operational);
		_GET("flagDelete").get_to(data->flagDelete);
		_GET("entities").get_to(data->entities);
		_GET("storedEntities").get_to(data->storedEntities);
		if (!costTimer.get())
			costTimer = t_time_manager::make_independent_timer();
		_GET("costTimer").get_to(data->costTimer);
		if (!actionTimer.get())
			actionTimer = t_time_manager::make_independent_timer();
		_GET("actionTimer").get_to(data->actionTimer);
		_GET("animFrame").get_to(data->animFrame);
		_GET("rStorage").get_to(data->rStorage);
		_GET("rPending").get_to(data->rPending);
		_GET("powerValue").get_to(data->powerValue);
		_GET("lastPowerOut").get_to(data->lastPowerOut);
		_GET("finalPowerOut").get_to(data->finalPowerOut);
		if (j.contains("binded"))
		{
			_GET("binded").get_to(data->binded);
		}
		if (j.contains("network"))
		{
			_GET("network").get_to(data->network);
		}
		_GET("updateInfo").get_to(data->updateInfo);
		if (j.contains("followers"))
		{
			_GET("followers").get_to(data->followers);
		}
		if (j.contains("upgrades"))
		{
			_GET("upgrades").get_to(data->upgrades);
		}

#undef _GET
	}
	catch (json::exception &e)
	{
		DEBUG("%s", j.dump().c_str());
		LOG_ERROR("Can't desirialize BuildingBase: %s", e.what());
	}
}

void BuildingBase::serialize_publish(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	g->add_timer(costTimer);
	g->add_timer(actionTimer);

	this->tree = dynamic_cast<UpgradeTree *>(
		g->bodyConfigMap.at(
			ENUM_BUILDING_TYPE,
			(t_id)info->buildType)[0]);
	assert(this->tree);
	for (VariantPtr<GameBody> b : followers)
		if (b.is_valid())
			map.apply(b);
	for (auto &v : binded)
		map.apply(v);
	if (network)
		map.apply(network);
}

void BuildingBase::serialize_initialize(const SerializeMap &map)
{
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	g->confirm_building_base(this);
	g->objectId = g->objectIdCounter.advance<BuildingBase>();
}

std::string BuildingBase::to_string()
{
	std::stringstream s("");
	s << "\t\t\t" << get_format_name() << "\n";

	if (context->bodyConfigMap.count(
			ENUM_BUILDING_TYPE, (t_id)(info->buildType)))
	{
		auto tree = get_tree();
		s << "Description: " << tree->description << "\n";
	}

	s << "HP: " << hp << "\n";
	if (entityLimit > 0)
	{
		s << "Entities: " << entities.size();
		s << " / " << entityLimit;
		s << "\n";
	}
	if (!rIn.empty())
		s << "Input: " << rIn.to_string(&context->resourceWeights) << "\n";
	if (!rOut.empty())
		s << "Output: " << rOut.to_string(&context->resourceWeights) << "\n";

	bool a = !rStoreCap.is_infinity();
	bool b;
	b = weightCap > 0;
	if ((a || b) && !rStoreCap.empty())
	{
		s << "Storage: ";
		s << rStorage.to_string(&context->resourceWeights) << " ";
		s << "W: " << rStorage.weight(&context->resourceWeights);
		s << "/ ";
		if (a)
			s << rStoreCap.to_string(&context->resourceWeights);
		if (b)
			s << "W: " << weightCap;
		s << "\n";
	}

	if (network)
	{
		if (powerIn > 0)
			s << "Power In: " << powerIn << "\n";
		if (powerOut > 0)
			s << "Power Out: " << powerOut << "\n";
		if (powerStore > 0)
			s << "Power Out: " << powerStore << "\n";

		auto strs = str_split(network->to_string(), "}");
		std::string str = iterator_to_string(strs.begin(), strs.end(), "}\n");
		s << "Network: " << str;
	}

	return s.str();
}

std::string BuildingBase::get_format_str(const std::string& name)
{
	std::string str = string_format("", name);
	str_replace(str, "_", " ");
	return str;
}

std::string BuildingBase::get_format_name()
{
	std::string str = string_format("", name);
	str_replace(str, "_", " ");
	return str;
}

FVec BuildingBase::get_center_pos()
{
	return (FVec)tilePos + (FVec)tilesSize / 2.f;
}

void BuildingBase::load_upgrade_step(UpgradeTree *step, void *tree)
{
	if (!step)
		return;

	if (std::find(upgrades.begin(), upgrades.end(), step->upgradeId) != upgrades.end())
	{
		WARNING("Upgrade \"%d\" name: \"%s\" has already applied", step->upgradeId, step->name);
		return;
	}

	this->id = step->type.id;
	this->upgrades.push_back(step->upgradeId);
	if (step->size != sf::Vector2i{-1, -1})
		this->tilesSize = step->size;

	if (strlen(step->name))
	{
		if (strlen(this->name))
		{
			DEBUG("Before %s %s", step->name, this->name);
			strcpy(
				this->name,
				string_format(this->name, step->name).c_str());
			DEBUG("After %s %s", step->name, this->name);
		}
		else
		{
			strcpy(
				this->name,
				step->name);
		}
	}

	if (step->hp != NULL_INT)
		this->lastHp = this->hp = step->hp;
	if (step->alignment != NULL_INT)
		this->alignment = step->alignment;
	// "isObstacle", "isBarrier"
	// Different names for the same variables?
	// Too bad!

	if (step->entityLimit != NULL_INT)
		this->entityLimit = step->entityLimit;
	if (step->delayCost != NULL_FLOAT)
		this->costTimer->set_length(step->delayCost);
	if (step->delayAction != NULL_FLOAT)
		this->actionTimer->set_length(step->delayAction);

	if (step->effectRadius != NULL_FLOAT)
		this->effectRadius = step->effectRadius;
	if (step->weightCap != NULL_INT)
		this->weightCap = step->weightCap;

	if (step->job != CitizenJob::NONE)
		this->job = (size_t)step->job;
		/*
	if (strlen(step->job))
	{
		if (str_is_num(step->job))
			this->job = (CitizenJob)(std::stoi(step->job) + (int)CitizenJob::NONE);
		else if (JOB_STRING.count(step->job))
			this->job = JOB_STRING.at(str_uppercase(std::string(step->job)));
		else
		{
			char buf[MAX_SHORT_STR];
			sprintf(buf, "Unknown job: %s", step->job);
			ASSERT_ERROR(false, buf);
		}
	}*/

#define SET_R(a, b) \
	if (!b.empty()) \
		a = b;

#define SET_I(a, b)    \
	if (b != NULL_INT) \
		a = b;

	SET_I(this->powerOut, step->powerOut);
	SET_I(this->powerIn, step->powerIn);
	SET_I(this->powerStore, step->powerStore);

	SET_R(this->rIn, step->input);
	SET_R(this->rOut, step->output);
	SET_R(this->rStoreCap, step->storageCap);

	if (step->spawn.serializableID != 0 &&
		step->spawn.id != IDPAIR_NONE)
		this->spawn = step->spawn;

	if (step->upgradeId == 0)
	{
		for (auto &[key, value] : step->spriteHolders)
			this->sprites[key] = value;
	}
	else
	{
		WARNING("Sprites upgrade uninplemented");
	}

	props.append(step->props);
}

bool BuildingBase::tree_similar(BuildingBase *other)
{
	if (!other)
		return false;
	if (this == other)
		return true;
	if (buildType != other->buildType)
		return false;
	const auto& ua = this->upgrades;
	const auto& ub = other->upgrades;
	if (ua.size() != ub.size())
		return false;
	for (auto &a : ua)
	{
		bool found = false;
		for (auto &b : ub)
		{
			if (a != b)
				continue;
			found = true;
		}
		if (!found)
			return false;
	}
	return true;
}

void BuildingBase::damage(int attack)
{
	hp -= attack;
}
/*
inline void BuildingBase::bool_reset()
{
	std::fill(
		std::begin(arrPropertyBools),
		std::end(arrPropertyBools),
		false);
}

bool BuildingBase::bool_is(PropertyBool x) const
{
	return arrPropertyBools[(size_t)x - (size_t)PropertyBool::NONE];
}

inline t_use BuildingBase::bool_set(PropertyBool x, bool b)
{
	return arrPropertyBools[(size_t)x - (size_t)PropertyBool::NONE] = b;
}

inline void BuildingBase::num_reset()
{
	propertyVals.clear();
}

bool BuildingBase::num_is(PropertyNum x) const
{
	return propertyVals.count(x);
}

double BuildingBase::num_set(PropertyNum x, double d)
{
	// printf("%s %f ", global_enum_to_str(x).c_str(), d);
	return propertyVals[x] = d;
}

double BuildingBase::num_get(PropertyNum x)
{
	return propertyVals[x];
}
*/

BuildingBody *BuildingBase::get_body() const
{
	Tile &t = context->chunks->get_tile(tilePos.x, tilePos.y);
	return t.building;
}

inline std::list<BuildingBody *> BuildingBase::get_bodies()
{
	std::list<BuildingBody *> ret;
	for (int x = 0; x < tilesSize.x; ++x)
	{
		for (int y = 0; y < tilesSize.y; ++y)
		{
			Tile &t = context->chunks->get_tile(tilePos.x + x, tilePos.y + y);
			ret.push_back(t.building);
		}
	}
	return ret;
}

void BuildingBase::update_sprites(int rotation)
{	/*
	for (BuildingBody* body : get_bodies())
	{
		body->spriteHolder = rand()%20;//body->get_sprite(rotation);
	}*/
	// uneeded
}

bool BuildingBase::is_any_storage() const
{
	bool out;
	out = props.bool_is((t_id)PropertyBool::ANY_STORAGE);
	assert(out == (props.bool_is((t_id)PropertyBool::STORAGE) ||
		props.bool_is((t_id)PropertyBool::TMP_STORAGE)));
	return out;
}

inline bool BuildingBase::is_operational() const
{
	if (props.bool_is(PropertyBool::WORKPLACE) &&
		entityLimit > 0 &&
		storedEntities.size() == 0)
		return false;

	return active && sufficient;
}

inline void BuildingBase::flag_deletion()
{
	this->flagDelete = true;
}

std::vector<UpgradeTree *> BuildingBase::get_upgrades()
{
	std::vector<UpgradeTree *> ret;
	if (tree)
		ret = recursive_get_available_tree(tree, upgrades);
	return ret;
}

void BuildingBase::accept_entity(EntityCitizen *e)
{
	e->workplace = this;
	this->entities.push_back(e);

	// Can not accept an entity that already has a job
	assert(e->job == CitizenJob::NONE);
	
	// If workplace's full, remove it from the "NEEDS_WORKERS" tree
	if (entities.size() >= entityLimit)
	{
		const static auto nw = (t_id)IngameProperties::NEEDS_WORKERS;

		const auto &bodies = get_bodies();
		std::for_each(bodies.cbegin(), bodies.cend(), [this](GameBody *body) {
			context
				->get_tree(ENUM_INGAME_PROPERTIES, nw)
				->remove(tilePos, body);
		});
	}
}

void BuildingBase::accept_entity_job(EntityCitizen* e)
{
	context
		->get_tree(ENUM_CITIZEN_JOB, (t_id)e->job)
		->remove(vec_pos_to_tile(e->pos), e);
	
	e->job = (CitizenJob)this->job;
	bool success;
	EntityStats* stats = context->get_entity_stats(
		ENUM_CITIZEN_JOB,
		this->job,
		&success);
	if (success)
	{
		e->apply_data(dynamic_cast<GameBodyConfig*>(stats));
		e->spriteHolder = e->spriteWalk;
	}
	
	// Insert the worker to his quad tree
	context
		->get_tree(ENUM_CITIZEN_JOB, (t_id)e->job)
		->insert(vec_pos_to_tile(e->pos), e);
}

void BuildingBase::enter_entity(EntityCitizen *e)
{
	if (this->props.bool_is(PropertyBool::INSIDE))
	{
		e->reset();
		e->insideWorkplace = true;
		this->storedEntities.push_back(e);
		auto v = vec_pos_to_tile(e->pos);
		ASSERT_ERROR(context->chunks->get_tile(v.x, v.y).remove_entity(e), "Failed attempt to remove entity from tile");
		context->hide_entity(e);
		e->action = EntityBody::Action::WORK;
		e->timerPath->reset(context->get_time());
		if (props.num_is(PropertyNum::ACTION_TIME))
		{
			double x;
			x = props.num_get(PropertyNum::ACTION_TIME);
			e->timerAction->set_length((float)x);
		}
	}
}

void BuildingBase::exit_entity(EntityCitizen *e)
{
	assert(e->workplace == this);
	if (this->props.bool_is(PropertyBool::INSIDE) &&
		e->workplace == this &&
		e->insideWorkplace)
	{
		e->reset();
		e->insideWorkplace = false;
		auto itr = std::find(
			storedEntities.begin(),
			storedEntities.end(),
			dynamic_cast<EntityBody *>(e));
		assert(itr != storedEntities.end());
		storedEntities.erase(itr);
		auto v = vec_pos_to_tile(e->pos);
		context->chunks->get_tile(v.x, v.y).entities.push_back(e);
		e->action = EntityBody::Action::IDLE;
		context->show_entity(e);
	}
}

std::vector<VariantPtr<EntityBody>>::iterator
BuildingBase::remove_entity(EntityCitizen *e)
{
	assert(e->workplace == this);

	// Like causes false assetion, building can get destroyed before 
	// entty get the job.
	// assert((size_t)e->job == this->job);

	// If work space was created, add to the "NEEDS_WORKERS" tree
	if (entities.size() <= entityLimit)
	{
		static const auto nw = (t_id)IngameProperties::NEEDS_WORKERS;
		const auto &bodies = get_bodies();
		std::for_each(bodies.cbegin(), bodies.cend(), [this](GameBody *body) {
			assert(body);
			// Don't add a building if it has a null body
			// The quad trees should never hold null values
			if (!body)
				return;

			context->get_tree(
					   ENUM_INGAME_PROPERTIES,
					   nw)
				->insert(tilePos, body);
				
		});
	}

	e->workplace = nullptr;
	e->job = CitizenJob::NONE;

	auto itr = std::find(entities.begin(), entities.end(), e);
	assert(itr != entities.end());
	auto ret = this->entities.erase(itr);
	/*
	for (t_idwtree& gb : context->get_trees(e))
	{
		gb.second->remove(vec_pos_to_tile(e->pos), e);
	}
	*/
	context
		->get_tree(ENUM_CITIZEN_JOB,
				   (t_id)e->job)
		->remove(
			vec_pos_to_tile(e->pos), e);
	
	return ret;
}

void BuildingBase::update()
{
	// Here's goes the stuff the can happen even if
	// the building is inactive.

	// If the building is a depleted raw resource, hide it and mark it
	// for later deletion .
	if (props.bool_is(PropertyBool::HARVESTABLE) &&
		rStorage.empty())
	{
		this->flagDelete = true;
		this->hp = 0;
		auto bodies = get_bodies();
		std::for_each(bodies.begin(), bodies.end(),
			[](GameBody* body) { body->visible = false; });
	}

	if (this->hp != this->lastHp)
	{
		this->updateInfo = true;
		this->lastHp = this->hp;
	}

	if (this->hp <= 0)
	{
		this->flagDelete = true;
	}

	// Go through every propertyVals
	for (const auto& pPair : props.nums_itr())
	{
		switch (pPair.first)
		{
			// Removes a percentage amount of resources
			// BAD FEATURE, REMOVE
		case PropertyNum::DECAY_AMOUNT:
		{
			break;
			while (actionTimer->next_surplus(context->get_time()))
			{
				Resources x = this->rStorage;
				x *= (float)pPair.second;
				this->rStorage = x;
			}
			break;
		}
		// Change frames according to the storage capacity
		case PropertyNum::STORAGE_FRAMES:
		case PropertyNum::POWER_FRAMES:
		case PropertyNum::ENTITY_FRAMES:
		{
			auto bodies = get_bodies();
			double storageFrames = pPair.second;
			std::for_each(
				bodies.begin(),
				bodies.end(),
				[this, pPair, storageFrames](BuildingBody* body) {
					float frame = 0;
					if (pPair.first == PropertyNum::POWER_FRAMES)
					{
						frame = (float)powerValue / powerStore;
					}
					else if (pPair.first == PropertyNum::ENTITY_FRAMES)
					{
						frame = (float)storedEntities.size() / (entityLimit + 1);
					}
					else if (weightCap > 0)
						frame = (float)rStorage.weight(&context->resourceWeights) / weightCap;
					else
					{
						frame = rStorage.get_presentage_of(rStoreCap);
					}

					frame *= (float)storageFrames;
					frame = math_min<float>(frame, (float)storageFrames);

					body->animFrame = (int)frame;

					if (0 && context->frameCount % 50 == 0)
					{
						DEBUG("%s %s %f %f %f %d",
							CSTR(rStorage),
							CSTR(rStoreCap),
							weightCap,
							rStorage.get_presentage_of(rStoreCap),
							(float)storageFrames,
							(int)body->animFrame);
					}
				});
		}
		break;
		default:
			continue;
		}
	}

	// From here, everything can only work if the building active
	if (!active)
		return;

	// If the building is a workplace, multiply every resource
	// related logic by the workers count.
	int storedCount = props.bool_is(PropertyBool::WORKPLACE)
		? (int)storedEntities.size()
		: 1;

	bool wasSufficient = sufficient;
	unsigned ioTime = 0;

	// Resource io goes here
	while (costTimer->next_surplus(context->get_time()))
		++ioTime;

	// If it's time to gemerateqconsume resources
	for (unsigned i = 0; i < ioTime; ++i)
	{
		// Check if storage can afford the input
		sufficient = rStorage.affordable(rIn * storedCount);

		// If it's part of a network
		if (network &&
			props.bool_is(PropertyBool::POWER_NETWORK) &&
			this->powerOut)
		{
			// If the building had the ability to geenrate power
			if (wasSufficient && !sufficient)
			{
				// Slowly dampen the power it's generate until it's 0s
				network->powerOut -= this->lastPowerOut;
				this->finalPowerOut = 0;
				this->updateInfo = true;
			}
			else if (!wasSufficient && sufficient)
			{
			}
		}
	}

	// Unable to operate, stop
	if (!sufficient)
		return;

	// Reset pending resources, remove it for keeping them
	// todo remove?
	rPending = Resources::res_empty(&context->resourceWeights);

	for (unsigned i = 0; i < ioTime; ++i)
	{
		Resources out = this->rPending;

		if (buildType == (int)BuildingType::GENRATOR && 0)
			DEBUG("%s %s %d %s", CSTR(rStorage), CSTR(this->rIn), (int)storedCount, CSTR((this->rIn * storedCount)));
		// Remove from storage the nessesey resources
		rStorage -= this->rIn * storedCount;

		// If the building had the ability to geenrate power
		if (network &&
			props.bool_is(PropertyBool::POWER_NETWORK) &&
			powerOut)
		{
			// Slowly accelerate the power output until the desired amoint
			int powerOut2 = this->powerOut * storedCount;
			int newOut = this->finalPowerOut;
			int dif = math_sign(powerOut2 - finalPowerOut);
			newOut += dif * POWER_SPEED;
			newOut = dif > 0
				? math_min(newOut, powerOut2)
				: math_max(newOut, powerOut2);

			network->powerOut += newOut;
			network->powerOut -= this->finalPowerOut;

			this->finalPowerOut = newOut;

			this->updateInfo = true;
		}

		// Find how many resources need to be generated
		out += this->rOut * storedCount;

		// If something was generated
		if (!out.empty())
		{
			// Move the resources to the building's storage
			Resources::transfer(
				out,
				rStorage,
				&context->resourceWeights,
				weightCap,	// Max transfer amount
				rStoreCap,	// Max storage capacity
				weightCap); // Transfer count
			this->updateInfo = true;
		}
	}

	bool isOffensive = props.bool_is(PropertyBool::OFFENSIVE);
	bool isHome = props.bool_is(PropertyBool::HOME);

	GameBody* target = nullptr;

	if (isOffensive)
	{
		float effectRadius = this->effectRadius;
		auto entities =
			context->nearest_bodies_quad(
				this->get_center_pos(),
				1ull,
				{ ENUM_ALIGNMENT, GameData::alignment_opposite(this->alignment) },
				[effectRadius](const sf::Vector2i& pos,
					const GameBody* gb,
					const int dist)
				{
					return dist < effectRadius;
				});

		if (entities.size())
		{
			target = dynamic_cast<GameBody*>(entities.front());
		}
	}

	if (!target && !isHome)
		return;

	// Spawning
	while (actionTimer->next_surplus(context->get_time()))
	{
		if (!is_operational())
			break;

		if (isHome && !((entities.size() < entityLimit) ||
			(spawn.id.id != (t_id)BodyType::ENTITY)))
		{
			// break;
		}

		// If the building a home, create new citizens
		if (spawn.id.group != ENUM_NONE &&
			spawn.serializableID != 0)
		{
			sf::Vector2f freePos;
			BodyQueueData data{};

			if (isHome)
			{
				if (!context->get_free_neighbor(
					freePos, this->get_bodies().back()->pos))
					break;
			}
			else
				freePos = get_center_pos();

			data.spawn = this->spawn;
			data.pos = this->get_center_pos();
			data.alignment = this->alignment;

			if (isOffensive && target)
			{
				FVec targetPos = target->pos;
				if (target->type == BodyType::BUILDING)
				{
					BuildingBody* build;
					build = dynamic_cast<BuildingBody*>(target);
					targetPos = build->base->get_center_pos();
				}
				FVec dif = -(this->get_center_pos() - target->pos);
				vec_normalize(dif);

				if (dif != FVec{ 0.f, 0.f })
				{
					data.vel = dif;
					data.target = target;
				}
			}
			else if (isHome)
			{
				data.home = this;
				/*
				GameBody* gb = nullptr;
				
				if (isOffensive && target )
					gb = context->add_game_body(
						spawn.serializableID,
						spawn.id.group,
						spawn.id.id,
						freePos);

				if (!gb || !(gb->type == BodyType::ENTITY))
					continue;

				EntityBody* entity = dynamic_cast<EntityBody*>(gb);

				assert(entity);
				if (entity->entityType == EntityType::CITIZEN)
				{
					EntityCitizen* citizen = dynamic_cast<EntityCitizen*>(gb);
					citizen->home = this;
				}
				this->entities.push_back(entity);
				this->updateInfo = true;

				if (entities.size() >= entityLimit)
				{
					actionTimer->reset(context->get_time());
					break;
				}
				*/
			}
			else
			{
				break;
			}

			context->queue_add_body(data);
		}


	}
	
	
}

// Building Body

BuildingBody::BuildingBody(
	GameData *context,
	const sf::Vector2i &pos,
	BuildingBase *base,
	t_sprite spriteHolder,
	sf::Vector2i spriteIndex,
	size_t id)
	: GameBody(
		  context,
		  SERIALIZABLE_BUILD_BODY,
		  id,
		  (sf::Vector2f)pos + sf::Vector2f{.5f, .5f}, // + (sf::Vector2f)base->tilesSize / 2.f,
		  -1,
		  BodyType::BUILDING,
		  sf::Vector2f{1.f, 1.f}),
	  tilePos(pos),
	  spriteHolder(spriteHolder),
	  spriteIndex(spriteIndex),
	  base(base)
{
	this->shape = ShapeType::RECT;
	this->frame = (int)base->animFrame;
	this->alignment = base->alignment;
}

BuildingBody::~BuildingBody()
{
}

int BuildingBody::get_hp() const
{
	return base.get()->hp;
}

int& BuildingBody::get_hp()
{
	return base.get()->hp;
}

void BuildingBody::apply_data(GameBodyConfig *config)
{
	LOG_ERROR("Can't apply data to building body, should be applied to base");
	assert(0);
}

bool BuildingBody::confirm_body(GameData *context)
{
	GameBody::confirm_body(context);

	BuildingBody *body = this;
	body->context = context;

	Tile &tile = context->chunks->get_tile(body->tilePos.x, body->tilePos.y);
	tile.building = body;
	context->bodies.push_back(body);


	context->buildings.push_back(body);


	// Insert building body to quad trees
	auto trees = context->get_trees(dynamic_cast<GameBody *>(body));
	for (auto& pair : trees)
	{
		GameBody *b = dynamic_cast<GameBody *>(body);
		t_tiletree *tree = pair.second;
		assert(b);
		assert(tree && tile.building);
		if (!tree || !tile.building)
			continue;
		bool bb = tree->insert(body->tilePos, b);
		if (!bb)
		{
			ASSERT_ERROR(
				bb,
				"QuadTree insert failed");
			return false;
		}
	}

	return true;
}

t_sprite BuildingBody::get_sprite(int rotation) const
{
	if (spriteHolder == -1)
		return -1;
	sf::Vector2i axis = vec_90_rotate_axis<int>(-rotation);
	sf::Vector2i index = spriteIndex;
	if (axis.x == -1)
		index.x = base->tilesSize.x - index.x - 1;
	if (axis.y == -1)
		index.y = base->tilesSize.y - index.y - 1;
	auto itr = base->sprites.find(index);
	if (itr == base->sprites.end())
		itr = base->sprites.find({0, 0});
	assert(itr != base->sprites.end());
	return (*itr).second; //base->sprites[index];
}

json BuildingBody::to_json() const
{
	json j = GameBody::to_json();
	j["build"] = {tilePos, start, end, spriteHolder, frame, maxFrame,
				  spriteIndex, base->to_ptr_json()};
	return j;
}

void BuildingBody::from_json(const json &j0)
{
	try
	{
		GameBody::from_json(j0);
		json j = j0.at("build");

		tilePos = j[0].get<sf::Vector2i>();
		start = j[1].get<sf::Vector2i>();
		end = j[2].get<sf::Vector2i>();
		spriteHolder = j[3].get<t_sprite>();
		frame = j[4].get<int>();
		maxFrame = j[5].get<int>();
		spriteIndex = j[6].get<sf::Vector2i>();
		base.from_json_ptr(j[7]);
	}
	catch (json::exception &e)
	{
		LOG_ERROR("Can't desirialize BuildingBody: %s", e.what());
	}
}

void BuildingBody::serialize_publish(const SerializeMap &map)
{
	GameBody::serialize_publish(map);

	map.apply(this->base);
	assert(this->base);
}

void BuildingBody::serialize_initialize(const SerializeMap &map)
{
	GameBody::serialize_initialize(map);
	// Get the context of the game
	GameData *g = map.get<GameData>(SERIALIZABLE_DATA, 0);
	assert(g);

	// Confirm the body
	this->confirm_body(g);
	g->objectId = g->objectIdCounter.advance<BuildingBody>();
}