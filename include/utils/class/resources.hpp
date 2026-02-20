#ifndef _GAME_RESOURCES
#define _GAME_RESOURCES

#include <array>
#include <cassert>
#include <iterator>

#include "utils/class/logger.hpp"
#include "../utils/globals.hpp"
#include "../utils/math.hpp"
#include "../utils/utils.hpp"

typedef IdStrTMap<int, std::map> t_weights;

struct Resources
{
	static const int ORE = 0;
	static const int GEMS = 1;
	static const int BODIES = 2;
	static const int COUNT = 3;

	static const int CONST_INF = -1;

	std::map<size_t, int> resCount;

#define INF(res, indx) (res.get(indx) == -1)

	decltype(resCount)::iterator begin()
	{
		return resCount.begin();
	}

	decltype(resCount)::iterator end()
	{
		return resCount.end();
	}

	decltype(resCount)::const_iterator begin() const
	{
		return resCount.begin();
	}

	decltype(resCount)::const_iterator end() const
	{
		return resCount.end();
	}

	Resources(const Resources &x, t_weights *resWeight)
	{
		resCount[ORE] = 0;
		resCount[GEMS] = 0;
		resCount[BODIES] = 0;
	}
	
	Resources(std::initializer_list<int> lst)
	{
		auto i = 0;
		auto j = lst.begin();
		for (; j != lst.end(); j++)
			resCount[i++] = *j;
	}

	Resources() = default;

	static Resources all(int x, t_weights *resWeight)
	{
		Resources ret;
		if (!resWeight)
			return ret;
		t_weights &weights = *resWeight;
		for (auto &w : weights)
		{
			auto &id = w.first;
			ret.resCount[id] = x;
		}
		return ret;
	}

	static Resources res_inf(t_weights *resWeight)
	{
		return all(-1, resWeight);
	}

	static Resources res_empty(t_weights *resWeight)
	{
		return all(0, resWeight);
	}
	
	static Resources res_empty()
	{
		return Resources({});
	}

	static Resources res_true(t_weights *resWeight)
	{
		return all(1, resWeight);
	}

	int &operator[](const size_t i)
	{
		return resCount[i];
	}

	int get(const size_t i) const
	{
		return resCount.count(i) ? resCount.at(i) : 0;
	}

	bool operator==(const Resources& other) const
	{
		// First check if all resources in this object are 
		// available in the second
		for (const std::pair<const size_t, int>& resource : resCount)
		{
			if (resource.second == 0)
				continue;
			if (!other.resCount.count(resource.first))
				return false;
			if (other.resCount.at(resource.first) !=
				resource.second)
				return false;
			// continue
		}

		// Then do the opposite
		for (const std::pair<const size_t, int>& resource : other.resCount)
		{
			if (resource.second == 0)
				continue;
			if (!this->resCount.count(resource.first))
				return false;
			if (this->resCount.at(resource.first) !=
				resource.second)
				return false;
			// continue
		}

		return true;
	}

	bool operator!=(const Resources& other) const
	{
		return ! ( *this == other );
	}

	Resources &operator=(const Resources &other)
	{
		this->resCount = other.resCount;
		return *this;
	}

	inline bool operator<(const Resources &other) const
	{
		for (auto o : other)
		{
			if (get(o.first) >= o.second)
				return false;
		}
		for (auto o : *this)
		{
			if (o.second >= other.get(o.first))
				return false;
		}
		return true;
	}

	inline bool operator<=(const Resources &other) const
	{
		for (auto o : other)
		{
			if (get(o.first) > o.second)
				return false;
		}
		for (auto o : *this)
		{
			if (o.second > other.get(o.first))
				return false;
		}
		return true;
	}

	Resources operator+(const Resources &c) const
	{
		Resources ret = *this;
		for (auto i : c)
		{
			ret[i.first] += i.second;
		}
		return ret;
	}

	Resources operator-(const Resources &c) const
	{
		Resources ret = *this;
		for (auto i : c)
		{
			ret[i.first] -= i.second;
		}
		return ret;
	}

	template <typename U>
	Resources operator*(const U x) const
	{
		Resources ret;
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (itr.second >= 0)
				ret[i] = (int)((U)itr.second * x);
			else
				ret[i] = -1;
		}
		return ret;
	}

	Resources &operator+=(const Resources &c)
	{
		for (auto &itr : c)
		{
			size_t i = itr.first;
			if (get(i) == -1 || c.get(i) == -1)
				continue;
			operator[](i) += c.get(i);
		}
		return *this;
	}

	template <typename U>
	Resources &operator*=(const U x)
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) == -1)
				continue;
			operator[](i) = (int)((U)get(i) * x);
		}
		return *this;
	}

	Resources &operator-=(const Resources &c)
	{
		for (auto &itr : c)
		{
			size_t i = itr.first;
			if (get(i) == -1 || c.get(i) == -1)
				continue;
			(*this)[i] -= c.get(i);
		}
		return *this;
	}

	float get_presentage_of(const Resources &c) const
	{
		float sum = 0.0f;
		int counter = 0;
		for (auto &itr : c)
		{
			size_t i = itr.first;
			if (c.get(i) == 0)
				continue;
			++counter;
			sum += (float)get(i) / c.get(i);
		}
		return counter ? (sum / (float)counter) : 0.f;
	}

	float get_presentage_of(int cap, t_weights* weights) const
	{
		assert(weights);
		
		int weightSum = 0;
		for (auto& itr : this->resCount)
		{
			size_t i = itr.first;
			if (get(i) == 0)
				continue;
			if (get(i) == -1)
				return -1.f;
			weightSum += weights->get(i) * get(i);
		}
		return (float)weightSum / cap;
	}

	bool capped(const Resources &cap) const
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (cap.get(i) != -1 && get(i) == -1 && get(i) > cap.get(i))
				return false;
		}
		return true;
	}

	inline bool affordable(const Resources &cost) const
	{
		for (auto &itr : cost)
		{
			size_t i = itr.first;
			if ((cost.get(i) == CONST_INF) || get(i) < cost.get(i))
				return false;
		}
		return true;
	}

	inline bool empty() const
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) != 0)
				return false;
		}
		return true;
	}

	inline bool is_infinity() const
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) != -1)
				return false;
		}
		return true;
	}

	inline void clear()
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			(*this)[i] = 0;
		}
	}

	inline Resources to_bool() const
	{
		Resources ret;
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			ret[i] = get(i) ? 1 : 0;
		}
		return ret;
	}

	inline Resources reverse_bool(t_weights* weights) const
	{
		Resources ret;
		for (auto &itr : *weights)
		{
			size_t i = itr.first;
			ret[i] = get(i) ? false : true;
		}
		return ret;
	}

	inline Resources reverse_bool(t_weights& weights) const
	{
		return reverse_bool(&weights);
	}

	inline std::string to_string() const
	{
		// return "todo resources.hpp"; 
		std::string str = "{";
		for (auto &itr : *this)
		{
			str += std::to_string(itr.first);
			str += ": ";
			str += std::to_string(itr.second);
			str += ", ";
		}
		str += "}";
		return str;
	}
	
	inline std::string to_string(t_weights* weights) const
	{
		// return "todo resources.hpp"; 
		std::string str = "{";
		for (auto &itr : *this)
		{
			str += weights->get_str(itr.first);
			str += ":";
			str += std::to_string(itr.second);
			str += " ";
		}
		str += "}";
		return str;
	}

	inline int weight(t_weights *resWeight) const
	{
		int ret = 0;
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) > 0)
				ret += resWeight->get(i) * get(i);
		}
		return ret;
	}

	inline bool is_full(t_weights *resWeight, const int weightLimit) const
	{
		if (weightLimit == -1)
			return false;
		int sum = 0;
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) > 0)
				sum += get(i) * resWeight->get(i);
		}
		return sum >= weightLimit;
	}

	inline bool is_full(const Resources &cap) const
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) < cap.get(i) || cap.get(i) == -1)
				return false;
		}
		return true;
	}

	inline bool has_bool(const Resources &b) const
	{
		for (auto &itr : *this)
		{
			size_t i = itr.first;
			if (get(i) && b.get(i))
				return true;
		}
		return false;
	}

	inline bool has_bool_all(const Resources& b) const
	{
		if (b.empty())
			return true;
		for (auto& itr : *this)
		{
			size_t i = itr.first;
			if (b.get(i))
			{
				if (!get(i))
					return false;
			}
		}
		return true;
	}

	/**
    Remove "cost" from resources,
    Returns true if src becomes empty, else returns false.
    */
	static inline bool use(Resources &src, Resources &cost)
	{
		for (auto &itr : cost)
		{
			size_t i = itr.first;
			if (src.get(i) == 0 || cost.get(i) == 0)
				continue;
			ASSERT_ERROR(src.get(i) != -1 || cost.get(i) != -1, "Cant remove infinity from infinity");

			int dif = cost.get(i);
			dif = math_min(dif, src[i]);
			dif = INF(src, i) ? 0 : dif;
			if (INF(cost, i))
			{
				src[i] = 0;
			}
			else
			{
				src[i] -= dif;
				cost[i] -= dif;
			}
		}
		return cost.empty();
	}

	/**
    For storage buildings.
    Fills this with in, limited by limit.
    Move resources from
    in to here. If the limit of the storage "limit" is
    reached don't empty "in", instead move the
    required amount to fill the storage.
    If the limit is reached return false, else true
    */
	bool fill(Resources &in, const Resources &limit)
	{
		bool ret = true;
		for (auto &itr : *this)
		{
			size_t i = itr.first;

			(*this)[i] += in.get(i);
			if (get(i) > limit.get(i))
			{
				in[i] = get(i) - limit.get(i);
				(*this)[i] = limit.get(i);
				ret = false;
			}
			else
			{
				in[i] = 0u;
			}
		}

		return ret;
	}

	static bool transfer(
		Resources &from,
		Resources &to,
		t_weights *weights,
		const int weightTransfer)
	{
		return transfer(
			from, to, weights, weightTransfer,
			res_inf(weights),
			-1,
			res_true(weights));
	}

	static bool transfer(
		Resources &from,
		Resources &to,
		t_weights *weights,
		const int weightTransfer,
		const Resources &toLimit,
		const int toLimitWeight)
	{
		return transfer(
			from, to, weights, weightTransfer,
			toLimit,
			toLimitWeight,
			res_true(weights));
	}

	static bool transfer(
		Resources &from,
		Resources &to,
		t_weights *weights,
		const int weightTransfer,
		const Resources &toLimit,
		const int toLimitWeight,
		const Resources &boolResource)
	{
		int weightMin = (weightTransfer == -1 ? INT32_MAX : weightTransfer);
		if (toLimitWeight >= 0)
			weightMin = math_min(weightMin, toLimitWeight - to.weight(weights));

		bool anything = false;

		// Go throught every input resource
		for (auto &itr : from)
		{
			// Get id of resource
			size_t i = itr.first;

			// Skip is bool resource is false for current
			if (!boolResource.get(i))
				continue;
			// Try to transfer all from source
			int amount = from[i];
			// If there's is a weight limit, transfer the minimum possible
			amount = weightMin / weights->get(i);
			amount = math_min(amount, from[i]);
			// If there's a hard limit, transfer the minimum
			if (toLimit.get(i) >= 0)
				amount = math_min(amount, toLimit.get(i) - to[i]);
			// If there's anything to transfer, do it
			if (amount > 0)
			{
				to[i] += amount;
				from[i] -= amount;
				anything = true;
				int sub = amount * weights->get(i);
				if (weightMin > 0 && weightMin - sub <= 0)
					break;
				weightMin -= sub;
			}
		}

		return !from.empty() && anything;
	}

	Resources bool_pass(const Resources &other) const
	{
		Resources ret;
		for (auto &itr : other)
		{
			size_t i = itr.first;
			ret[i] = (other.get(i) ? this->get(i) : 0);
		}
		return ret;
	}

	/**
    Check if there is any space for transferring any
    resources from "from" to "to", limited by weight of resource
    count.
   */

	static bool can_transfer_weight(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight)
	{
		return can_transfer_weight(
			from, to, weights, toLimitWeight,
			res_true(weights));
	}

	static bool can_transfer_weight(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight,
		const Resources &boolResource)
	{
		if (toLimitWeight == -1)
			return true;
		int delta = toLimitWeight - to.bool_pass(boolResource).weight(weights);
		if (delta <= 0)
			return false;

		for (auto &itr : from)
		{
			size_t i = itr.first;

			if (boolResource.get(i))
			{
				assert(from.get(i) != -1 && to.get(i) != -1);
				if (from.get(i) > 0 &&
					math_min(1, from.get(i)) * weights->get(i) <= delta)
					return true;
			}
		}

		return false;
	}

	static bool can_transfer_limit(
		const Resources &from,
		const Resources &to,
		const Resources &toLimit,
		t_weights *weights)
	{
		return can_transfer_limit(
			from, to, toLimit,
			res_true(weights));
	}

	static bool can_transfer_limit(
		const Resources &from,
		const Resources &to,
		const Resources &toLimit,
		const Resources &boolResource)
	{
		Resources dif = toLimit - to;
		for (auto &itr : from)
		{
			size_t i = itr.first;

			assert(from.get(i) != -1 && to.get(i) != -1);
			if (!boolResource.get(i))
				continue;
			if (from.get(i) > 0 && (dif.get(i) > 0 || INF(toLimit, i)))
				return true;
		}
		return false;
	}

	// can_transfer

	static bool can_transfer(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight,
		const Resources &toLimit)
	{
		return can_transfer(
			from, to, weights, toLimitWeight,
			toLimit,
			Resources::res_true(weights));
	}

	static bool can_transfer(
		const Resources &from,
		const Resources &to,
		t_weights *weights,
		const int toLimitWeight,
		const Resources &toLimit,
		const Resources &boolResource)
	{
		for (auto &itr : from)
		{
			size_t i = itr.first;
			ASSERT_ERROR(from.get(i) != -1, "Can't transfer from infinite storage storage");
		}
		return can_transfer_weight(from, to, weights, toLimitWeight, boolResource) &&
			   can_transfer_limit(from, to, toLimit, boolResource);
	}
};

#endif // _GAME_RESOURCES

/*
#ifndef _GAME_RESOURCES
#define _GAME_RESOURCES

#include <array>
#include <cassert>
#include <iterator>

#include "utils/globals.hpp"
#include "utils/logger.hpp"
#include "utils/math.hpp"
#include "utils/utils.hpp"

struct Resources
{
	static const int ORE = 0;
	static const int GEMS = 1;
	static const int BODIES = 2;
	static const int COUNT = 3;

	static const int CONST_INF = -1;

	BiMap<std::string, int> resCount;
	BiMap<std::string, int> resWeight;

#define INF(res, indx) (res.get(indx) == -1)

	[[deprecated]]
	static constexpr int WEIGHT[COUNT] = {
		WEIGHT_ORE,
		WEIGHT_GEMS,
		WEIGHT_BODIES};
		
	[[deprecated]]
	int data[3] = {0, 0, 0};

	Resources(const Resources &x)
	{
		for (int i = 0; i < COUNT; ++i)
			data[i] = x.get(i);
	}

	Resources() = default;

	Resources(std::initializer_list<int> lst)
	{
		auto i = std::begin(data);
		auto j = lst.begin();
		for (; i != std::end(data) && j != lst.end(); i++, j++)
			*i = *j;
	}

	static Resources all(int x)
	{
		Resources ret;
		for (int i = 0; i < COUNT; ++i)
			ret[i] = x;
		return ret;
	}

	static Resources inf_resources()
	{
		return Resources({-1, -1, -1});
	}

	int &operator[](const int i)
	{
		assert(0 <= i && i < COUNT);
		return data[i];
	}

	const int &get(const int i) const
	{
		assert(0 <= i && i < COUNT);
		return data[i];
	}

	Resources &operator=(const Resources &other)
	{
		for (int i = 0; i < COUNT; ++i)
			data[i] = other.get(i);
		return *this;
	}

	inline bool operator<(const Resources &other) const
	{
		for (int i = 0; i < COUNT; ++i)
			if (get(i) >= other.get(i))
				return false;
		return true;
	}

	inline bool operator<=(const Resources &other) const
	{
		for (int i = 0; i < COUNT; ++i)
			if (get(i) > other.get(i))
				return false;
		return true;
	}

	Resources operator+(const Resources &c) const
	{
		Resources ret = *this;
		ret += c;
		return ret;
	}

	Resources operator-(const Resources &c) const
	{
		Resources ret = *this;
		ret -= c;
		return ret;
	}

	template <typename U>
	Resources operator*(const U x) const
	{
		Resources ret;
		for (int i = 0; i < COUNT; ++i)
			if (get(i) >= 0)
				ret[i] = (int)((U)get(i) * x);
			else
				ret[i] = -1;
		return ret;
	}

	Resources &operator+=(const Resources &c)
	{
		for (int i = 0; i < COUNT; ++i)
		{
			if (data[i] == -1 || c.get(i) == -1)
				continue;
			data[i] += c.get(i);
		}
		return *this;
	}

	template <typename U>
	Resources &operator*=(const U x)
	{
		for (int i = 0; i < COUNT; ++i)
		{
			if (data[i] == -1)
				continue;
			data[i] = (int)((U)data[i] * x);
		}
		return *this;
	}

	Resources &operator-=(const Resources &c)
	{
		for (int i = 0; i < COUNT; ++i)
		{
			if (data[i] == -1 || c.get(i) == -1)
				continue;

			data[i] -= c.get(i);
		}
		return *this;
	}

	float get_presentage_of(const Resources &c) const
	{
		float sum = 0.0f;
		int counter = COUNT;
		for (int i = 0; i < COUNT; ++i)
		{
			if (c.get(i) == 0 && get(i) == 0)
				--counter;
			else if (c.get(i) == 0 && get(i) != 0)
				return FLT_MAX;
			else if (c.get(i) != 0 && get(i) != 0)
				sum += (float)get(i) / c.get(i);
		}
		return sum / (float)counter;
	}

	bool capped(const Resources &cap) const
	{
		for (int i = 0; i < COUNT; ++i)
			if (cap.get(i) != -1 && data[i] == -1 && data[i] > cap.get(i))
				return false;
		return true;
	}

	inline bool affordable(const Resources &cost) const
	{
		for (int i = 0; i < COUNT; ++i)
			if ((cost.get(i) == CONST_INF) || get(i) < cost.get(i))
				return false;
		return true;
	}

	inline bool empty() const
	{
		for (int i = 0; i < COUNT; ++i)
			if (data[i] != 0)
				return false;
		return true;
	}

	inline bool is_infinity() const
	{
		for (int i = 0; i < COUNT; ++i)
			if (data[i] != -1)
				return false;
		return true;
	}

	inline void clear()
	{
		for (int i = 0; i < COUNT; ++i)
			data[i] = 0;
	}

	inline int weight() const
	{
		int ret = 0;
		for (int i = 0; i < COUNT; ++i)
			if (data[i] > 0)
				ret += WEIGHT[i] * data[i];
		return ret;
	}

	inline Resources to_bool()
	{
		Resources ret;
		for (int i = 0; i < COUNT; ++i)
			ret[i] = get(i) ? 1 : 0;
		return ret;
	}

	inline Resources reverse_bool()
	{
		Resources ret;
		for (int i = 0; i < COUNT; ++i)
			ret[i] = get(i) ? false : true;
		return ret;
	}

	inline std::string to_string() const
	{
		return array_to_string<int>(data, COUNT);
	}

	inline bool is_full(const int weightLimit) const
	{
		if (weightLimit == -1)
			return false;
		int sum = 0;
		for (int i = 0; i < COUNT; ++i)
			if (data[i] > 0)
				sum += data[i] * WEIGHT[i];
		return sum >= weightLimit;
	}

	inline bool is_full(const Resources &cap) const
	{
		for (int i = 0; i < COUNT; ++i)
			if (get(i) < cap.get(i) || cap.get(i) == -1)
				return false;
		return true;
	}

	inline bool is_full(const Resources &cap, int weightLimit) const
	{
		return is_full(weightLimit) || is_full(cap);
	}

	inline bool has_bool(const Resources &b) const
	{
		for (int i = 0; i < COUNT; ++i)
			if (get(i) && b.get(i))
				return true;
		return false;
	}

	static inline bool use(Resources &src, Resources &cost)
	{
		for (int i = 0; i < COUNT; ++i)
		{
			if (src.get(i) == 0 || cost.get(i) == 0)
				continue;
			ASSERT_ERROR(!(INF(src, i) && INF(cost, i)), "Cant remove infinity from infinity");

			int dif = cost.get(i);
			dif = math_min(dif, src[i]);
			dif = INF(src, i) ? 0 : dif;
			if (INF(cost, i))
			{
				src[i] = 0;
			}
			else
			{
				src[i] -= dif;
				cost[i] -= dif;
			}
		}
		return cost.empty();
	}

	bool fill(Resources &in, const Resources &limit)
	{
		bool ret = true;
		for (int i = 0; i < COUNT; ++i)
		{
			data[i] += in[i];
			if (data[i] > limit.get(i))
			{
				in[i] = data[i] - limit.get(i);
				data[i] = limit.get(i);
				ret = false;
			}
			else
			{
				in[i] = 0u;
			}
		}

		return ret;
	}

	static bool transfer(
		Resources &from,
		Resources &to,
		const int weightTransfer = WEIGHT_TRANSFER_CITIZEN,
		const Resources &toLimit = {-1, -1, -1},
		const int toLimitWeight = -1,
		const Resources &boolResource = Resources{1, 1, 1})
	{
		int weightMin = (weightTransfer == -1 ? INT32_MAX : weightTransfer);
		if (toLimitWeight >= 0)
			weightMin = math_min(weightMin, toLimitWeight - to.weight());

		bool anything = false;

		for (int i = 0; i < COUNT; ++i)
		{
			// Skip is bool resource is false for current
			if (!boolResource.get(i))
				continue;
			// Try to transfer all from source
			int amount = from[i];
			// If there's is a weight limit, transfer the minimum possible
			amount = weightMin / WEIGHT[i];
			amount = math_min(amount, from[i]);
			// If there's a hard limit, transfer the minimum
			if (toLimit.get(i) >= 0)
				amount = math_min(amount, toLimit.get(i) - to[i]);
			// If there's anything to transfer, do it
			if (amount > 0)
			{
				to[i] += amount;
				from[i] -= amount;
				anything = true;
				int sub = amount * WEIGHT[i];
				if (weightMin > 0 && weightMin - sub <= 0)
					break;
				weightMin -= sub;
			}
		}

		return !from.empty() && anything;
	}

	Resources bool_pass(const Resources &other) const
	{
		Resources ret;
		for (int i = 0; i < COUNT; ++i)
		{
			ret[i] = (other.get(i) ? this->get(i) : 0);
		}
		return ret;
	}

	static bool can_transfer_weight(
		const Resources &from,
		const Resources &to,
		const int toLimitWeight,
		const Resources &boolResource = {true, true, true, true})
	{
		if (toLimitWeight == -1)
			return true;
		int delta = toLimitWeight - to.bool_pass(boolResource).weight();
		if (delta <= 0)
			return false;

		for (int i = 0; i < COUNT; ++i)
		{
			if (boolResource.get(i))
			{
				assert(from.get(i) != -1 && to.get(i) != -1);
				if (from.get(i) > 0 && math_min(1, from.get(i)) * WEIGHT[i] <= delta)
					return true;
			}
		}

		return false;
	}

	static bool can_transfer_limit(
		const Resources &from,
		const Resources &to,
		const Resources &toLimit,
		const Resources &boolResource = {1, 1, 1, 1})
	{
		Resources dif = toLimit - to;
		for (int i = 0; i < COUNT; ++i)
		{
			assert(from.get(i) != -1 && to.get(i) != -1);
			if (!boolResource.get(i))
				continue;
			if (from.get(i) > 0 && (dif.get(i) > 0 || INF(toLimit, i)))
				return true;
		}
		return false;
	}

	static bool can_transfer(
		const Resources &from,
		const Resources &to,
		const int toLimitWeight,
		const Resources &toLimit,
		const Resources &boolResource = {1, 1, 1})
	{
		for (int i = 0; i < COUNT; ++i)
			ASSERT_ERROR(!INF(from, i), "Can't transfer from infinite storage storage");
		return can_transfer_weight(from, to, toLimitWeight, boolResource) &&
			   can_transfer_limit(from, to, toLimit, boolResource);
	}
};

static const Resources RES_INF = {-1, -1, -1};

#endif // _GAME_RESOURCES
*/