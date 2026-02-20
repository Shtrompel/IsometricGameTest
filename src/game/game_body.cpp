#include "game/game_body.hpp"
#include "game/game_data.hpp"

std::string GameBody::test_get_str() const
{
	std::stringstream ret;
	ret << "{ ";
	ret << context->enum_get_str(this);
	ret << ", ";
	ret << this->alignment;
	ret << ", ";
	ret << (void const*)this;
	ret << " }";
	return ret.str();

}

void GameBody::set_target(GameBody* newTarget, bool skipOld)
{
	// An object can't make inself it's own target
	assert(newTarget != this);
	assert(target != this);

	if (target == newTarget)
		return;
	/*DEBUG("set_target with object %s %d %p, target is %p",
		context->enum_get_str(this).c_str(),
		(int)this->alignment,
		this, newTarget);*/

	// Remove this object as a follower from the old target
	if (this->target != nullptr)
	{
		std::vector<VariantPtr<GameBody>>& followersRef =
			this->target->followers;
		/*DEBUG("Target %s %d %p have currently %d followers",
			context->enum_get_str(target).c_str(),
			(int)target->alignment,
			target,
			(int)followersRef.size());*/

		if (this->target != newTarget && followersRef.size())
		{
			for (auto itr = followersRef.begin(); itr != followersRef.end();)
			{
				VariantPtr<GameBody> bodyPtr = *itr;
				//DEBUG("Trying to remove %p from target followers, comparing it to %p", bodyPtr.get(), this);

				if (bodyPtr == this)
				{
					itr = followersRef.erase(itr);
				}
				else
				{
					++itr;
				}
			}
		}

		//DEBUG("In the end %d followers", (int)followersRef.size());

	}

	// Add this object as a follower to the new target
	if (newTarget != nullptr)
	{
		std::vector<VariantPtr<GameBody>>& followersRef =
			newTarget->followers;

		bool addFollower = true;
		if (followersRef.size())
		{

			auto itr = std::find_if(
				followersRef.begin(),
				followersRef.end(),
				[this](const VariantPtr<GameBody>& ptr)
				{
					return ptr == this;
				});
			addFollower = itr == followersRef.end();
		}

		if (addFollower)
		{
			followersRef.push_back(this);
		}
	}

	// Set the new target
	this->target = newTarget;
}