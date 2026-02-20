#ifndef _GAME_BODY
#define _GAME_BODY

#include <SFML/System/Vector2.hpp>

#include "../utils/globals.hpp"
#include "../libs/FastNoiseLite.h"
#include "../file/serialization.hpp"

#include <unordered_set>

struct GameData;

typedef typename std::shared_ptr<GameTimer<t_seconds>> t_body_timer;
typedef typename GameTimerManager<t_seconds> t_time_manager;

template <typename t_use, typename t_property>
struct PropertySet
{
	typedef typename std::unordered_set<t_use> t_bools;
	typedef typename std::map<t_property, double> t_nums;

	static_assert(
		!std::is_same<
			t_use,
			t_property>::value,
		"PropertySet types can be the same");

	t_bools uses;
	t_nums nums;

	struct BoolItr
	{
		typedef typename t_bools::iterator iterator;
		typedef typename t_bools::const_iterator const_iterator;

		t_bools *const bools = nullptr;

		iterator begin() { return bools->begin(); }

		iterator end() { return bools->end(); }

		const_iterator cbegin() const { return bools->cbegin(); }

		const_iterator cend() const { return bools->cend(); }
	};

	template <bool IsConst = false>
	struct NumItr
	{
		using Container = std::conditional_t<
			IsConst,
			t_nums const,
			t_nums>;

		using iterator = std::conditional_t<
			IsConst,
			typename Container::const_iterator,
			typename Container::iterator>;

		Container *nums = nullptr;

		iterator begin() { return nums->begin(); }

		iterator end() { return nums->end(); }
	};

	PropertySet()
	{
	}
	/*
	PropertySet(const PropertySet<t_use, t_property> &other)
	{
		this->uses = other.uses;
		this->nums = other.nums;
	}

	PropertySet(PropertySet<t_use, t_property> &&other)
	{
		this->uses = std::move(other.uses);
		this->nums = std::move(other.nums);
	}*/

	void append(const PropertySet<t_use, t_property> &other)
	{
		for (const auto &v : other.nums)
			this->nums.insert(v);
		for (const auto &v : other.uses)
			this->uses.insert(v);
	}

	// Uses

	inline void bool_reset()
	{
		uses.clear();
	}

	bool bool_is(const t_use &x) const
	{
		return uses.count(x);
	}

	bool bool_get(const t_use &x) const
	{
		return uses.count(x);
	}

	bool bool_is(size_t i) const
	{
		return bool_is((t_use)i);
	}

	bool bool_get(size_t i) const
	{
		return bool_get((t_use)i);
	}

	inline void bool_set(t_use x, bool b)
	{
		uses.insert(x);
	}

	inline size_t bool_size() const
	{
		return uses.size();
	}

	inline BoolItr bool_itr()
	{
		return BoolItr{&uses};
	}

	// Properties

	inline void num_reset()
	{
		nums.clear();
	}

	bool num_is(const t_property x) const
	{
		return nums.count(x);
	}

	inline double num_set(t_property x, double d)
	{
		return (nums[x] = d);
	}

	double num_get(t_property x)
	{
		return nums.at(x);
	}

	bool num_is(size_t i) const
	{
		return num_is((t_property)i);
	}

	bool num_get(size_t i) const
	{
		return bool_get((t_property)i);
	}

	size_t num_size() const
	{
		return nums.size();
	}

	inline NumItr<false> nums_itr()
	{
		return NumItr<false>{&nums};
	}

	inline NumItr<true> nums_citr() const
	{
		NumItr<true> ret;
		ret.nums = &this->nums;
		return ret;
	}
};

template <typename t_use, typename t_property>
static void to_json(
	json &j,
	const PropertySet<t_use, t_property> &v)
{
	j = {v.uses, v.nums};
}

template <typename t_use, typename t_property>
static void from_json(
	const json &j,
	PropertySet<t_use, t_property> &v)
{
	j.at(0).get_to(v.uses);
	j.at(1).get_to(v.nums);
}

struct GameBodyConfig
{
	virtual ~GameBodyConfig() = default;

	virtual std::string to_string()
	{
		return "to_string() unimplemented";
	}
};

struct BodyConfigManager
{
	// Unreadable mess lol
	typedef GameBodyConfig *t_type;
	typedef std::unordered_map<t_id, t_type> t_idt;
	typedef std::unordered_map<t_idpair, t_idt> t_pair_idt;

	struct iterator
	{
		t_pair_idt *setA = nullptr;
		t_pair_idt::iterator itrA;
		t_idt *setB = nullptr;
		t_idt::iterator itrB;

		iterator(
			t_pair_idt *setA,
			t_pair_idt::iterator itrA,
			t_idt *setB,
			t_idt::iterator itrB)
			: setA(setA),
			  itrA(itrA),
			  setB(setB),
			  itrB(itrB)
		{
		}

		iterator &operator++()
		{
			itrB++;
			if (itrB == setB->end())
			{
				++itrA;
				if (itrA != setA->end())
					itrB = (*itrA).second.begin();
			}
			return *this;
		}

		iterator operator++(int)
		{
			iterator old = *this;
			operator++();
			return old;
		}

		t_type operator*()
		{
			return (*itrB).second;
		}
	};

	t_pair_idt data;

	BodyConfigManager()
	{
	}

	iterator insert(const t_idpair &pair, const t_type &type)
	{
		t_pair_idt::iterator itrA;
		t_idt::iterator itrB;
		if ((itrA = data.find(pair)) == data.end())
			itrA = data.insert({pair, t_idt{}}).first;
		t_idt &idt = (*itrA).second;
		if ((itrB = idt.find((t_id)0)) == idt.end())
			itrB = idt.insert({0, type}).first;
		return iterator{&data, itrA, &idt, itrB};
	}

	iterator insert(const t_idpair &pair, const t_id id, const t_type &type)
	{
		t_pair_idt::iterator itrA;
		t_idt::iterator itrB;
		if ((itrA = data.find(pair)) == data.end())
			itrA = data.insert({pair, t_idt{}}).first;
		t_idt &idt = (*itrA).second;
		if ((itrB = idt.find(id)) == idt.end())
			itrB = idt.insert({id, type}).first;
		return iterator{&data, itrA, &idt, itrB};
	}

	t_type at(const t_idpair &pair)
	{
		return data.at(pair).at(0);
	}

	t_type at(const t_idpair &pair, const t_id id)
	{
		return data.at(pair).at(id);
	}

	iterator begin()
	{
		return iterator{
			&data,
			data.begin(),
			&(*data.begin()).second,
			(*data.begin()).second.begin()};
	}
	
	iterator end()
	{
		auto last = std::next(data.begin(), data.size()-1);
		return iterator{
			&data,
			last,
			&(*last).second,
			(*last).second.end()};
	}
	
};

struct GameBody;

class AbstractCanTarget
{

	virtual GameBody* get_target() 
	{
		return nullptr;
	}

	virtual void set_target(GameBody*) {};

};

struct GameBody : Variant, AbstractCanTarget
{
	bool isNoiseInit = 0;
	FastNoiseLite noise;

	GameData *context = nullptr;
	[[deprecated]] t_obj_itr<GameBody*> objItr{};

	// Game logic
	FVec pos;
	FVec vel;
	float z = 0.0f;
	// AABB game rectangle width and height
	sf::Vector2f rectSize;
	// Radius for entities
	float radius = 0.0f;
	/* In the list of body types as seen in enums.json(with id of 1)
	 * what type of body is this object?
	 */
	BodyType type = BodyType::NONE;
	// What spawned this object
	GameBody *source = nullptr;

	std::vector<VariantPtr<GameBody>> followers{};

	VariantPtr<GameBody> target{ nullptr };

	ShapeType shape = ShapeType::POINT;
	bool visible = true;
	t_alignment alignment = ALIGNMENT_NONE;

	std::array<bool, COUNT_PROPERTY_BOOL>
		arrUseEnums = {false};
	std::map<PropertyNum, double>
		propertyVals;

	static inline int tmpHp = -1; // Null get_hp placeholder
	bool dead = false; // Add to json copression
	// And remove dead from entity

	// Rendering
	sf::Vector2i start = {-1, -1};
	sf::Vector2i end = {-1, -1};
	t_sprite spriteHolder = -1;
	size_t animFrame = 0u;

	GameBody()
	{
	}

	GameBody(
		GameData *context,
		size_t e,
		size_t id,
		const sf::Vector2f &pos,
		const t_sprite &sprite,
		const BodyType &type,
		const sf::Vector2f &rect)
		: Variant(e, id)
	{
		this->context = context;
		this->pos = pos;
		this->rectSize = rect;
		this->type = type;
		this->spriteHolder = sprite;

		if (!isNoiseInit)
		{
			noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
			noise.SetFrequency(1.f);
			isNoiseInit = true;
		}
	}

	virtual ~GameBody() = default;

	std::string get_ptr_str() const
	{
		std::ostringstream oss;
		oss << (void*)this;
		std::string s(oss.str());
		return s;
	}

	virtual int get_hp() const { assert(0); return -1; }

	virtual int& get_hp() { assert(0); return tmpHp; }

	virtual GameBody* get_target()
	{
		return target;
	}

#define EPIC_TEST3()\
	for (VariantPtr<GameBody>& x : this->followers)\
	assert(x.get() != this);

	std::string test_get_str() const;

	/*
	 Steps of removing target:
		1) Remove from the target myself as a follower
		2) Set my target to null
	 Steps of setting target:
		1) Add myself to a follower to the target
		2) Set target to follower
	*/
	void set_target(GameBody* newTarget, bool skipOld = false);

	virtual FVec get_pos() { return this->pos;  }

	virtual void apply_data(GameBodyConfig *config) {}

	virtual void update() {}

	virtual void update(float delta) {}

	inline float depth() const
	{
		// Isometric world depth
		return pos.x + pos.y;
	}

	bool operator<(const GameBody &other) const
	{
		// Depth sorting for rendering
		return this->depth() < other.depth();
	}

	void change_sprite(t_sprite sprite)
	{
		if (sprite != (t_sprite)-1 && sprite != 0)
			this->spriteHolder = sprite;
	}

	void virtual variant_initialize() override
	{
		isNoiseInit = 0;
		z = 0.0f;
		type = BodyType::NONE;
		source = nullptr;
		shape = ShapeType::POINT;
		visible = true;
		alignment = ALIGNMENT_NONE;
		start = { -1, -1 };
		end = { -1, -1 };
		spriteHolder = -1;
		animFrame = 0u;
	}

	virtual json to_json() const override
	{

		json j;
		j["body"] = {pos, rectSize, type, visible, start, end,
					 spriteHolder, animFrame, radius, alignment,
					 nullptr, nullptr,
					 arrUseEnums, propertyVals,
					 this->dead, this->vel };
		if (target.get())
			j["body"][11] = target;
		if (followers.size())
			j["body"][10] = followers;
		return j;
	}

	virtual void from_json(const json &j0) override
	{
		auto &j = j0.at("body");
		j.at(0).get_to(this->pos);
		j.at(1).get_to(this->rectSize);
		j.at(2).get_to(this->type);
		j.at(3).get_to(this->visible);
		j.at(4).get_to(this->start);
		j.at(5).get_to(this->end);
		j.at(6).get_to(this->spriteHolder);
		j.at(7).get_to(this->animFrame);
		j.at(8).get_to(this->radius);
		j.at(9).get_to(this->alignment);
		if (!j.at(10).is_null())
			j.at(10).get_to(this->followers);
		if (!j.at(11).is_null())
			j.at(11).get_to(this->target);
		j.at(12).get_to(this->arrUseEnums);
		j.at(13).get_to(this->propertyVals);
		j.at(14).get_to(this->dead);
		j.at(15).get_to(this->vel);
		
	}

	/**
	* Used for descompression or generic constructor (like the spawn 
	* mechanic). Initialize all of the stuff that is not based on
	* Other pointers of Variant type.
	* All Variant pointers must be declared here with the 
	* SerializeMap::apply(Variant*) function.
	*/
	virtual void serialize_publish(const SerializeMap &map) override
	{
	}

	/**
	* Used for descompressiof or generic constructor (like the spawn
	* mechanic). All initializations based on Variant pointer
	* are located here. Initializations that are based on GameData
	* will be made in the confirm_body function, which must be
	* called here.
	*/
	virtual void serialize_initialize(const SerializeMap &map) override
	{
	}

	/**
	* Do all of the initialization that is based on GameData.
	* Since GameData is based on Variant, this function must
	* be called within serialize_initialize.
	* Everything that is needed to happend regardless of
	* is the object is genericly spawned of specificly spawned
	* must be located here, since this function is always called.
	*/
	virtual bool confirm_body(GameData* context)
	{
		return true;
	}

};

#endif // _GAME_BODY