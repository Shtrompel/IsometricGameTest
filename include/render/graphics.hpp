#ifndef _GAME_GRAPHICS
#define _GAME_GRAPHICS

#include "../game/game_data.hpp"

#include "utils/class/logger.hpp"
#include "../utils/math.hpp"
#include "../utils/utils.hpp"

#include "../file/asset_manager.hpp"
#include "../render/gui.hpp"

#include <set>
#include <unordered_map>

struct Orientation
{
	// World scale, scale of each tile compared to it's original size
	sf::Vector2f scale = {1.f, 1.f};
	// The tile size in pixels
	sf::Vector2i tileSize = DEFAULT_TILE_SIZE;
	// Times the world rotated in 90 degrees
	int rotation = ROTATION_INIT;

	sf::Vector2i worldPos = {0, 0};

	int zoomFactor = 0;

	std::string to_string()
	{
		std::stringstream ret;
		ret << "{";
		ret << "Scale : " << vec_str(scale);
		ret << ", ";
		ret << "Tile Pixel Size : " << vec_str(tileSize);
		ret << ", ";
		ret << "World rotation : " << rotation * 4 << " Degrees";
		ret << "}";
		return ret.str();
	}
};

static void to_json(json &j, const Orientation &v)
{
	j = {v.scale, v.tileSize, v.rotation, v.worldPos, v.zoomFactor};
}

static void from_json(const json &j, Orientation &v)
{
	j.at(0).get_to(v.scale);
	j.at(1).get_to(v.tileSize);
	j.at(2).get_to(v.rotation);
	j.at(3).get_to(v.worldPos);
	j.at(4).get_to(v.zoomFactor);
}

static sf::Vector2f screen_pos_to_world_pos_src(const sf::Vector2i &posIn, const Orientation &gridDraw)
{
	sf::Vector2i pos = posIn - gridDraw.worldPos;
	sf::Vector2f tileDrawSize = gridDraw.tileSize * gridDraw.scale;

	float a = 0.5f * tileDrawSize.x;
	float b = -a;
	float c = 0.5f * tileDrawSize.y;
	float d = c;

	sf::Vector2f posf = {
		(float)(d * pos.x - b * pos.y),
		(float)(-c * pos.x + a * pos.y)};
	//posf *= 1.0f / (a * d - b * c);
	posf = posf / (a * d - b * c);

	return posf;
}

inline sf::Vector2f screen_pos_to_world_pos(const sf::Vector2i &posIn, const Orientation &gridDraw)
{
	return vec_90_rotate(
		screen_pos_to_world_pos_src(posIn, gridDraw),
		gridDraw.rotation);
}

inline sf::Vector2i screen_pos_to_tile_pos(const sf::Vector2i &posIn, const Orientation &gridDraw)
{
	sf::Vector2f posf = screen_pos_to_world_pos_src(posIn, gridDraw);
	sf::Vector2i posi;

	posf = vec_90_rotate(
		posf,
		gridDraw.rotation);
	posi = sf::Vector2i{(int)math_floor(posf.x), (int)math_floor(posf.y)};
	return posi;
}

template <typename U = int>
sf::Vector2i world_pos_to_screen_pos(const sf::Vector2<U> &tilePos, const Orientation &gridDraw)
{
	auto fixedTilePos = tilePos;

	sf::Vector2<U> tileDrawPos = {
		fixedTilePos.x - fixedTilePos.y,
		fixedTilePos.x + fixedTilePos.y};
	sf::Vector2f tileDrawSize = gridDraw.tileSize * gridDraw.scale;

	tileDrawPos = vec_90_rotate(
		tileDrawPos,
		((int)gridDraw.rotation + 2) % 2);

	// Trial and error test, i don't understand it myself
	if (1 <= gridDraw.rotation &&
		gridDraw.rotation <= 2)
		tileDrawPos = tileDrawPos * (U)-1;

	tileDrawPos = (sf::Vector2<U>)(tileDrawPos * tileDrawSize);
	tileDrawPos /= (U)2;
	tileDrawPos += (sf::Vector2<U>)gridDraw.worldPos;

	return (sf::Vector2i)tileDrawPos;
}

struct IVecCompare
{
	inline bool operator()(const IVec &a, const IVec &b) const
	{
		return vec_compare(a, b);
	}
};

struct RendererClass
{
	sf::RenderWindow *window = nullptr;
	sf::View *view = nullptr;

	// Rendering
	Orientation orientation;
	bool zooming = false;
	bool isBtnPressed = 0;

	bool drawPath = true;

	t_sprite spriteGrass = -1;
	t_sprite spriteTileSelected = -1;
	t_sprite spriteEntity = -1;
	t_sprite spriteConstruction = -1;
	t_sprite spriteIconBuild = -1;
	t_sprite spriteIconDelete = -1;
	t_sprite spriteEnemy = -1;
	t_sprite spriteTmpBullet = -1;

	size_t renderFrame = 0;

	AssetManager *assets = nullptr;

	RendererClass()
	{
	}

	RendererClass(
		sf::RenderWindow *window,
		sf::View *view,
		AssetManager *assets) : window(window), view(view), assets(assets)
	{
	}

	void render_grid(Chunks *chunks)
	{
		assert(window);
		assert(view);

		sf::Vector2i p = screen_pos_to_tile_pos(
			(sf::Vector2i)view->getCenter(),
			orientation);

		int
			idx = math_floordiv(p.x, CHUNK_W),
			idy = math_floordiv(p.y, CHUNK_H);

		const int COUNT = 4;
		sf::Sprite* grassSprite = assets->get_sprite(spriteGrass)->get(0);
		for (int i = COUNT; i < 9 - COUNT; ++i)
		{
			sub_render_grid(
				chunks,
				idx + i % 3 - 1,
				idy + i / 3 - 1,
				grassSprite);
		}
	}

	FRect render_object(GameBody* body, bool bSearch, ImageAlphaGrid* gridOut = nullptr)
	{
		
		if (!body->visible)
			return{};

		FVec bodyPos = body->pos;
		bodyPos.y -= body->z;
		
		sf::Vector2i pos = world_pos_to_screen_pos(
			bodyPos,
			orientation);

		if (!vec_inside<float>(
			(sf::Vector2f)pos,
			0,
			0,
			view->getSize().x,
			view->getSize().y))
		{
			return{};
		}
		
		t_sprite spriteHolder;
		if (body->type == BodyType::BUILDING)
		{
			BuildingBody* building = dynamic_cast<BuildingBody*>(body);
			assert(building);

			spriteHolder = building->get_sprite(orientation.rotation);
		}
		else
		{
			spriteHolder = body->spriteHolder;
		}

		if (spriteHolder == -1)
			return{};

		AssetSprites* sprites = assets
			->get_sprite(spriteHolder);

		sf::Sprite* sTile;
		size_t spriteId;
		if (body->type == BodyType::BULLET)
		{
			BulletBody* bullet;
			bullet = dynamic_cast<BulletBody*>(body);
			assert(bullet);

			spriteId = (size_t)bullet->get_direction_frame(orientation.rotation);
		}
		else
		{
			spriteId = body->animFrame;
		}
		sTile = sprites->get(spriteId);

		if (bSearch)
			*gridOut = sprites->getAlphaGrid(spriteId);

		IVec textureSize = sprites->spriteSize;


		TextureOrigin texOrigin = TextureOrigin::BOTTOM;
		switch (body->type)
		{
		case BodyType::BULLET:
			texOrigin = TextureOrigin::CENTERED;
			break;
		default:
			break;
		}

		FVec posOut;
		if (sTile)
		{
			posOut = sub_render(
				pos,
				orientation.scale,
				*sTile,
				texOrigin);
		}

		if (body->type == BodyType::ENTITY && drawPath)
		{
			EntityBody* entity = dynamic_cast<EntityBody*>(body);
			assert(entity);
			sub_render_draw(entity->pathData, { 1.f, 1.f });
		}

		if (bSearch)
		{
			FVec size = orientation.scale * sprites->spriteSize;
			return{
				(FVec)posOut,
				size };
		}
		return {};

	}

	GameBody* render_objects(
		std::vector<GameBody*>& bodies, 
		bool searchEntity, 
		FVec searchEntityPos,
		uint32_t entityFilter = -1)
	{
		assert(window);
		assert(view);

		GameBody* out = nullptr;
		for (auto bodyItr = bodies.begin(); bodyItr != bodies.end(); ++bodyItr)
		{
			GameBody* body = *bodyItr;
			assert(body);

			ImageAlphaGrid alphaGrid;
			ImageAlphaGrid* alphaGridPtr = &alphaGrid;
			FRect rect = render_object(body, searchEntity, &alphaGrid);

			IVec iSearchPos = (IVec)( (searchEntityPos - rect.pos) / orientation.scale.x);

			if (searchEntity)
			{
				// Check if the mask permits this object to be selected
				bool maskPass = entityFilter >> body->objectId & 0x1;
				/*
				DEBUG("Rect: %s %s, %d %d %d", 
					VEC_CSTR(rect.pos), 
					VEC_CSTR(rect.size),
					maskPass,
					vec_inside(searchEntityPos, rect.pos, rect.size),
					!alphaGrid.getPixel(iSearchPos.x, iSearchPos.y));
					*/

				if (rect.size == FVec{ 0.f, 0.f })
					continue;


				// Return this object if: 
				// Mask enables the selection, 
				// mouse inside the sprite rectangle,
				// pixel is not transparent.
				if (maskPass &&
					vec_inside(searchEntityPos, rect.pos, rect.size) &&
					alphaGrid.getPixel(iSearchPos.x, iSearchPos.y) != ALPHAGRID_TRANSPARENT)
				{
					out = body;
				}
			}
		}

		++renderFrame;
		return out;
	}

	void render_suggestion(
		UpgradeTree *buildTree,
		Chunks *chunks,
		GameMode gameMode,
		std::set<sf::Vector2i, IVecCompare> &suggestionBuilds)
	{
		assert(window);
		assert(view);

		// Show tiles that are suggested to be build over /
		// become empty.
		if (gameMode == GameMode::BUILD && buildTree)
		{
			for (auto itr = suggestionBuilds.begin();
				 itr != suggestionBuilds.end();
				 ++itr)
			{
				auto &tilePos = *itr;

				for (int x = 0; x < buildTree->size.x; ++x)
				{
					for (int y = 0; y < buildTree->size.y; ++y)
					{
						sf::Vector2i tileId = sf::Vector2i{x, y};
						sf::Vector2f addition = sf::Vector2f{.1f, .1f};
						addition = vec_mult(
							addition,
							vec_90_rotate_axis(-orientation.rotation));
						sf::Vector2f pos = sf::Vector2f(tilePos + tileId) +
										   sf::Vector2f{.5f, .5f} -
										   addition;
						sf::Vector2i drawPos = world_pos_to_screen_pos<float>(pos, orientation);
						sf::Vector2f tileDrawSize = orientation.tileSize * orientation.scale;

						if (buildTree->spriteHolders.empty() ||
							!vec_inside<float>(
								(sf::Vector2f)drawPos,
								-tileDrawSize.x,
								-tileDrawSize.y,
								view->getSize().x + 2.f * tileDrawSize.x,
								view->getSize().y + 2.f * tileDrawSize.y))
							continue;

						if (!buildTree->spriteHolders.count(tileId))
							continue;

						t_sprite holder = buildTree->spriteHolders.at(tileId);
						sf::Sprite *sTile = nullptr;
						if (holder < assets->sprites.size())
							sTile = assets->sprites.at(holder)->get(0);

						if (!sTile)
							continue;
						
						float fx = ((orientation.rotation % 2 == 0) ? 1.f : -1.f);
						sub_render(
							drawPos, 
							{fx * orientation.scale.x, orientation.scale.y},
							*sTile,
							TextureOrigin::BOTTOM);
					}
				}
			}
		}
		else if (gameMode == GameMode::DELETE ||
				 gameMode == GameMode::UPGRADE)
		{
			for (auto &tilePos : suggestionBuilds)
			{
				BuildingBody *b = chunks->get_build(tilePos.x, tilePos.y);
				sf::Vector2f pos = (sf::Vector2f)tilePos + sf::Vector2f{.5f, .5f};
				sf::Vector2i drawPos = world_pos_to_screen_pos<float>(pos, orientation);
				sf::Sprite *sIcon;

				if (b)
				{
					drawPos.y -= 20;
					sIcon = assets->sprites.at(spriteIconDelete)->get(0);
				}
				else
				{
					drawPos.y -= (int)(5.0f * orientation.scale.y);
					sIcon = assets->sprites.at(spriteTileSelected)->get(0);
				}
				sub_render(
					drawPos, 
					0.5f * orientation.scale,
					*sIcon, 
					TextureOrigin::BOTTOM);
			}
		}
	}

	void render_gui(
		GameData &data,
		float fps)
	{
		assert(window);
		assert(view);

		sf::CircleShape cs;
		cs.setFillColor(sf::Color::Green);
		cs.setRadius(16.f);
		cs.setOrigin(16, 16);
		cs.setPosition(view->getCenter());
		window->draw(cs);

		static char infoRes[60] = "";
		sprintf(
			infoRes,
			"Power: %d, Ore: %d, Gems: %d, Bodies: %d",
			data.power,
			data.resources[Resources::ORE],
			data.resources[Resources::GEMS],
			data.resources[Resources::BODIES]);
		static char fpsCStr[8];
		sprintf(fpsCStr, "%.3f", fps);
	}

	void sub_render_grid(Chunks *chunks, int idx, int idy, sf::Sprite *tileSprite)
	{
		sf::Sprite *sprite = nullptr;
		Grid *grid = chunks->get(idx, idy);
		if (!grid)
			return;

		for (int x = 0; x < grid->cx; x++)
		{
			for (int y = 0; y < grid->cy; y++)
			{
				sf::Vector2i tilePos = {x, y};

				tilePos += {
					grid->idx * CHUNK_W,
					grid->idy * CHUNK_H};

				sf::Vector2i drawPos = world_pos_to_screen_pos(tilePos, orientation);
				sf::Vector2f tileDrawSize = orientation.tileSize * orientation.scale;
				bool doRender = (*grid)(x, y).visible;
				doRender &= vec_inside<float>(
					(sf::Vector2f)drawPos,
					-tileDrawSize.x,
					-tileDrawSize.y,
					view->getSize().x + 2 * tileDrawSize.x,
					view->getSize().y + 2 * tileDrawSize.y);

				if (!doRender)
					continue;

				Tile &tile = (*grid)(x, y);
				BuildingBody *building = tile.building;

				// gridDraw2.rotation = WorldRotation::ROT0;
				bool bMouseTile = tilePos == screen_pos_to_tile_pos(
												 (sf::Vector2i)view->getCenter(),
												 orientation);
				if (bMouseTile)
					sprite = assets->get_sprite(spriteTileSelected)->get(0);
				else if ((building && building->base->buildType == (t_id)BuildingType::ROAD))
					sprite = assets->get_sprite(building->spriteHolder)->get(0);
				else
					sprite = tileSprite;

				if (tileSprite)
				{
					sub_render_tile(drawPos, orientation.scale, *sprite);
					/*
					sub_render(
						drawPos,
						orientation.scale, 
						*sprite,
						TextureOrigin::CENTERED
					);*/
				}
			}
		}
	}

	template <typename U>
	void sub_render_tile(
		sf::Vector2<U> pos,
		sf::Vector2f scale,
		sf::Sprite &s)
	{
		float x = (float)pos.x;
		float y = (float)pos.y;
		float ox = (float)s.getTextureRect().width;
		float oy = (float)s.getTextureRect().height;

		s.setScale(scale);
		s.setPosition(x, y);
		float originX, originY;



		float ORIGINS[4][2] = 
		{
			{ox / 2.f, oy - orientation.tileSize.y},
			{ox, oy - orientation.tileSize.y / 2.f},
			{ox / 2.f, oy},
			{0, oy - orientation.tileSize.y * 0.5f}
		};
		/*{
			{ox / 2.f, 0.f},
			{ox, oy / 2.f },
			{ox / 2.f, oy},
			{0.f, oy / 2.f}
		};*/
		originX = ORIGINS[orientation.rotation % 4][0];
		originY =  ORIGINS[orientation.rotation % 4][1];

		/*
		switch (orientation.rotation)
		{
		case ROTATION_0:
			originX = ox / 2.f;
			originY = 0;
			break;
		case ROTATION_90:
			originX = ox;
			originY = oy / 2.f;
			break;
		case ROTATION_180:
			originX = ox / 2.f;
			originY = oy;
			break;
		case ROTATION_270:
			originX = 0;
			originY = oy / 2.f;
			break;
		default:
			break;
		}
		*/
		s.setOrigin(originX, originY);

		window->draw(s);
	}

	// Return the position of the rop left corner of where 
	// the sprites start to being drawn
	template <typename U>
	FVec sub_render(
		const sf::Vector2<U>& pos,
		const sf::Vector2f& scale,
		sf::Sprite& s,
		TextureOrigin origin,
		IVec originPoint = {},
		bool drawBorders = false)
	{
		float x = (float)pos.x;
		float y = (float)pos.y;
		float ox = (float)s.getTextureRect().width;
		float oy = (float)s.getTextureRect().height;

		const sf::Vector2i& tileWorldSize = orientation.tileSize;

		switch (origin)
		{
		case TextureOrigin::BOTTOM:
			s.setOrigin(ox / 2.f, oy - tileWorldSize.y / 2.f);
			break;
		case TextureOrigin::CENTERED:
			s.setOrigin(ox / 2.f, oy / 2.f);
			break;
		case TextureOrigin::TOP_LEFT:
			s.setOrigin(0, 0);
			break;
		case TextureOrigin::POINT:
			s.setOrigin(
				(float)originPoint.x,
				(float)originPoint.y);
			break;
		default:
			break;
		}
		s.setPosition(x, y);
		s.setScale(scale);

		window->draw(s);

		// Draw rect representing the bounds of the sprite
		if (drawBorders)
		{
			sf::RectangleShape rectShape;
			rectShape.setPosition(
				s.getGlobalBounds().left,
				s.getGlobalBounds().top);
			rectShape.setSize(
				{ s.getGlobalBounds().width,
				s.getGlobalBounds().height });
			rectShape.setFillColor(sf::Color::Transparent);
			rectShape.setOutlineColor(sf::Color::Black);
			rectShape.setOutlineThickness(4);
			window->draw(rectShape);
		}

		{
			sf::CircleShape circle;
			circle.setRadius(4.f);
			circle.setOrigin(circle.getRadius(), circle.getRadius());
			circle.setFillColor(sf::Color::Yellow);
			circle.setOutlineColor(sf::Color::Black);

			circle.setPosition(x, y);

			window->draw(circle);
		}

		return {
			s.getGlobalBounds().left,
			s.getGlobalBounds().top };
	}

	void sub_render_draw(
		PathData &pathData,
		sf::Vector2f scale,
		bool relevantSegment = true)
	{
		if (!pathData.valid())
			return;
		sf::CircleShape circle;
		circle.setRadius(10.f);
		circle.setOrigin(circle.getRadius(), circle.getRadius());
		circle.setFillColor(sf::Color::Yellow);
		circle.setOutlineColor(sf::Color::Black);

		auto &path = pathData.path;
		sf::Vector2f a, b;
		bool first = 0;
		auto itr = path.cbegin();
		if (relevantSegment)
			itr = pathData.follower;
		for (; itr != path.end(); ++itr)
		{
			a = (sf::Vector2f)(*itr).value + sf::Vector2f{.5f, .5f};
			if (first)
			{
				sf::Vector2f pos1 = a;
				sf::Vector2f pos2 = b;
				pos1 = (sf::Vector2f)world_pos_to_screen_pos(pos1, orientation);
				pos2 = (sf::Vector2f)world_pos_to_screen_pos(pos2, orientation);
				sf::Vertex line[2] = {
					sf::Vertex(pos1),
					sf::Vertex(pos2)};
				window->draw(line, 2, sf::Lines);
			}
			{
				auto pos = world_pos_to_screen_pos(a, orientation);
				circle.setPosition((sf::Vector2f)pos);
				window->draw(circle);
			}

			first = 1;
			b = a;
		}
	}
};

#endif // _GAME_GRAPHICS