#ifndef _GAME_ASSET_MANAGER
#define _GAME_ASSET_MANAGER

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>


#include "utils/class/logger.hpp"
#include "utils/globals.hpp"
#include "utils/utils.hpp"
#include "utils/math.hpp"

#include <cassert>
#include <functional>
#include <map>
#include <unordered_map>

constexpr bool ALPHAGRID_TRANSPARENT = 1;

// Converts image to a 2d array repsesenting whether each pixel
// is transparent or not.
struct ImageAlphaGrid
{

	ImageAlphaGrid()
	{
	}

	ImageAlphaGrid(ImageAlphaGrid&& other) noexcept
	{
		this->operator=(std::forward<ImageAlphaGrid>(other));
	}

	ImageAlphaGrid(const ImageAlphaGrid& other) noexcept
	{
		this->operator=(other);
	}

	ImageAlphaGrid(const sf::Image& img)
	{
		loadFromImage(img);
	}

	ImageAlphaGrid(bool** buffer, unsigned w, unsigned h)
	{
		this->grid = buffer;
		this->gridW = w;
		this->gridH = h;
	}

	~ImageAlphaGrid()
	{
		if (grid == nullptr)
			return;

		for (unsigned y = 0; y < gridH; ++y)
			delete[] grid[y];
		delete[] grid;
	}

	ImageAlphaGrid& operator=(const ImageAlphaGrid& other)
	{
		this->gridW = other.gridW;
		this->gridH = other.gridH;
		this->grid = new bool* [gridH];
		for (unsigned y = 0; y < gridH; ++y)
		{
			this->grid[y] = new bool[gridW];
			for (unsigned x = 0; x < gridW; ++x)
			{
				this->grid[y][x] = other.grid[y][x];
			}
		}

		return *this;
	}

	ImageAlphaGrid& operator=(ImageAlphaGrid&& other) noexcept
	{
		this->grid = other.grid;
		this->gridW = other.gridW;
		this->gridH = other.gridH;
		other.grid = nullptr;

		return *this;
	}

	bool loadFromImage(const sf::Image& img)
	{
		const sf::Uint8* pixels = img.getPixelsPtr();

		gridW = img.getSize().x;
		gridH = img.getSize().y;

		grid = new bool* [gridH];
		for (unsigned y = 0; y < gridH; ++y)
			grid[y] = new bool[gridW];

		bool* gridPtr = *grid;

		if (pixels == nullptr)
			return false;

		for (unsigned y = 0; y < gridH; ++y)
		{
			gridPtr = grid[y];
			for (unsigned x = 0; x < gridW; ++x)
			{
				unsigned isAlpha = 0;
				if (img.getPixel(x, y).a == sf::Color::Transparent.a)
					isAlpha = 1;
				*gridPtr++ = isAlpha;
			}
		}

		return true;
	}

	bool getPixel(int x, int y)
	{
		if (grid == nullptr)
			return true;

		if (0 <= x && x < (int)gridW &&
			0 <= y && y < (int)gridH)
		{
			return  grid[y][x];
		}

		return true;
	}

	ImageAlphaGrid subsection(unsigned x, unsigned y, unsigned w, unsigned h)
	{
		if (gridH == 0 || gridW == 0)
			return{};

		if (!grid)
			return{};

		unsigned newGridW = w;
		unsigned newGridH = h;

		bool** newGrid = new bool* [newGridH];
		for (unsigned y = 0; y < newGridH; ++y)
			newGrid[y] = new bool[newGridW];


		bool* newGridPtr = *newGrid;
		bool* gridPtr = *grid;
		for (unsigned iy = y; iy < y + h; ++iy)
		{
			gridPtr = grid[iy];
			newGridPtr = newGrid[iy - y];
			for (unsigned ix = x; ix < x + w; ++ix)
			{
				bool bitSource = true;
				if (ix < gridW && iy < gridH)
					bitSource = gridPtr[ix];
				*newGridPtr++ = bitSource;
			}
		}

		return ImageAlphaGrid{ newGrid, newGridW, newGridH };
	}

	bool** grid = nullptr;
	unsigned gridH = 0, gridW = 0;
};

struct AssetKeySprites
{
	std::string texName;
	int start, end;
	// For multisized buildings, every tile has a
	// different id.
	unsigned id = 0;

	bool operator==(const AssetKeySprites &other) const
	{
		return this->texName == other.texName &&
			   this->start == other.start &&
			   this->end == other.end &&
			   this->id == other.id;
	}

	std::string to_string() const
	{
		std::stringstream ret;
		ret << "{ ";
		ret << "Texture name: ";
		ret << texName << ", ";
		ret << "Start: ";
		ret << start << ", ";
		ret << "End: ";
		ret << end;
		ret << " }";
		return ret.str();
	}
};

struct AssetSprites
{
	// Frames
	sf::Sprite *sprites = nullptr;
	// For every sprite, hold a alpga grid
	ImageAlphaGrid* alphaGrids = nullptr;

	int spriteCount;
	IVec sectionCount = {1, 1};
	IVec frameCount = {1, 1};
	AssetKeySprites key;
	bool allowOverflow = true;

	IVec spriteSize = { 0, 0 };

	sf::Sprite *get(const size_t i)
	{
		if (spriteCount == 0)
		{
			WARNING(
				"AssetSprites with texture: %s is empty and can't pass sprites.",
				key.texName.c_str());
			assert(0);
			return nullptr;
		}
		if (i >= spriteCount && !allowOverflow)
		{
			WARNING(
				"AssetSprites with texture: %s and sprite count: %zu was asked to get sprite at index: %zu.",
				key.texName.c_str(),
				spriteCount,
				i);
			assert(0);
			return nullptr;
		}
		if (!sprites)
		{
			WARNING(
				"\"sprites\" is null.",
				key.texName.c_str(),
				spriteCount,
				i);
			return nullptr;
		}
		return &sprites[i % spriteCount];
	}

	ImageAlphaGrid& getAlphaGrid(const size_t i)
	{
		if (spriteCount == 0)
		{
			WARNING(
				"ImageAlphaGrid with texture: %s is empty and can't pass sprites.",
				key.texName.c_str());
			assert(0);
		}
		if (i >= spriteCount && !allowOverflow)
		{
			WARNING(
				"ImageAlphaGrid with texture: %s and sprite count: %zu was asked to get sprite at index: %zu.",
				key.texName.c_str(),
				spriteCount,
				i);
			assert(0);
		}
		if (!sprites)
		{
			WARNING(
				"\"ImageAlphaGrid\" is null.",
				key.texName.c_str(),
				spriteCount,
				i);
		}
		return alphaGrids[i % spriteCount];
	}

	std::string to_string() const
	{
		std::stringstream ret;
		ret << "{ ";
		ret << "Sprite Count: ";
		ret << spriteCount << ", ";
		ret << "Key ";
		ret << key.to_string();
		ret << " }";
		return ret.str();
	}
};

struct AssetTexture
{
	sf::Texture texture;
	ImageAlphaGrid alphaGrid;

	IVec frameSize;
	IVec frameCount = {1, 1};
	IVec divisions = {1, 1};

	std::string to_string() const
	{
		std::stringstream ret;
		ret << "{ ";
		ret << "Texture Handle: ";
		ret << texture.getNativeHandle() << ", ";
		ret << "Frame Size: ";
		ret << vec_str(frameSize) << ", ";
		ret << "Frame Count: ";
		ret << vec_str(frameCount);
		ret << " }";
		return ret.str();
	}
};

namespace std
{
template <>
struct hash<AssetKeySprites>
{
	size_t operator()(AssetKeySprites const &key) const noexcept
	{
		size_t seed = std::hash<std::string>{}(key.texName);
		hash_combine(seed, key.start);
		hash_combine(seed, key.end);
		hash_combine(seed, key.id);
		return seed;
	}
};
} // namespace std



struct AssetManager
{
	const sf::Vector2i non = {-1, -1};

	std::vector<AssetTexture*> textures;
	std::vector<AssetSprites*> sprites;
	std::map<std::string, int> texturesHash;
	std::unordered_map<AssetKeySprites, int> spritesHash;
	std::string path;

	const AssetTexture nullTexture = {};

	typedef std::map<std::string, int>::iterator t_texhash_itr;

	AssetManager(const char *path)
	{
		this->path = std::string(path);
	}

	~AssetManager()
	{
		AssetSprites* sprite = nullptr;
		for (auto itr = sprites.begin(); itr != sprites.end(); itr++)
		{
			sprite = (*itr);
			delete[] sprite->sprites;
			delete[] sprite->alphaGrids;
			delete sprite;
		}
		for (auto itr = textures.begin(); itr != textures.end(); itr++)
		{
			delete (*itr);
		}
	}

	AssetSprites *get_sprite(int index)
	{
		if (index == -1)
		  return nullptr;
		if (0 <= index && index < sprites.size())
			return sprites[index];
		return nullptr;
	}
	
	int generate_null_texture()
	{
		sf::Image img;
		img.create(512, 512);
		
		for (int i = 0; i < 32; ++i)
		{
			int x = i % 8, y = i / 8;
			sf::Color c;
			c = ((i % 2) ? sf::Color::Black : sf::Color::Magenta);
			for (int px = 0; px < 512 / 32; ++px)
			{
				for (int py = 0; py < 512 / 32; ++py)
				{
					img.setPixel(px + x * 16, py + y * 16, c);
				}
			}
		}
		
		textures.push_back(new AssetTexture{});
		sf::Texture& t = textures.back()->texture;
		t.loadFromImage(img);

		DEBUG("todo");
		return -1;
	}

	int load_texture(
		const char *name,
		const IVec &frameCount = {1, 1},
		const IVec &divisions = {1, 1},
		const char *type = "png")
	{
		int texId = -1;
		textures.push_back(new AssetTexture{});

		sf::Texture& t = textures.back()->texture;
		char *filePath = new char[path.size() + strlen(name) + strlen(type) + 3];
		sprintf(filePath, "%s/%s.%s", path.c_str(), name, type);

		sf::Image img;
		if (img.loadFromFile(filePath))
		{
			textures.back()->alphaGrid.loadFromImage(img);
			if (t.loadFromImage(img))
			{
				texId = (int)textures.size() - 1u;
				AssetTexture& atex = *textures.back();
				atex.frameCount = frameCount;
				atex.frameSize = {
					(int)(t.getSize().x / frameCount.x),
					(int)(t.getSize().y / frameCount.y) };
				atex.divisions = divisions;
				texturesHash[str_uppercase(name)] = texId;
			}
			else
			{
				LOG_ERROR("Can't load texture from image that was originally at path \"%s\"", filePath);
			}
		}
		else
		{
			LOG_ERROR("Can't load image \"%s\"", filePath);
		}

		delete[] filePath;

		return texId;
	}

	int split_sprites(
		int index,
		const sf::Vector2i &framesStart,
		const sf::Vector2i &framesEnd,
		const sf::Vector2i &pos,
		const sf::Vector2i &buildSize)
	{
		// Get original sprite
		AssetSprites *ogSprites = get_sprite(index);
		if (index == DEBUG_SELECT_BUILDING_SPRITE)
		{
			DEBUG("spriteCount: %d", (int)ogSprites->spriteCount);
		}
		if (!ogSprites)
		{
			WARNING("Sprite with key %d was not found", (int)index);
			return -1;
		}

		AssetKeySprites key = ogSprites->key;
		// Unique tile id for multitiled textures
		key.id = pos.x + pos.y * buildSize.x;

		// Create new sprite
		sprites.push_back(new AssetSprites{});
		int ret = (int)sprites.size() - 1;
		spritesHash[key] = ret;
		AssetSprites &sprite = *sprites.back();
		auto dif = framesEnd - framesStart + sf::Vector2i{1, 1};
		sprite.spriteCount = vec_prod<int>(dif);
		sprite.sprites = new sf::Sprite[sprite.spriteCount];
		sprite.key = key;
		/*
        if (dif.x != 1)
            WARNING("When using multitiled building, no divisions can be made in the horizontal axis for now");
        for (int y = 0; y < dif.y; ++y)
        {
            int index = pos.y * buildSize.x + pos.x;

            sprite.sprites[y] = ogSprites->sprites[index];
        }*/

		if (index == DEBUG_SELECT_BUILDING_SPRITE)
			DEBUG("\nframesStart: %s\nframesEnd: %s\npos: %s\nbuildSize: %s\nsprite: %d\nspriteCount: %d\nog spriteCount: %d",
				VEC_CSTR(framesStart),
				VEC_CSTR(framesEnd),
				VEC_CSTR(pos), 
				VEC_CSTR(buildSize),
				ret,
				(int)sprite.spriteCount,
				(int)ogSprites->spriteCount);
		for (int i = 0; i < sprite.spriteCount; ++i)
		{
			IVec newPos = {0, 0};
			newPos += pos;
			// framesStart, framesEnd, buildSize, pos
			int spriteIndex = (newPos.x + newPos.y * buildSize.x);
			spriteIndex += vec_prod(buildSize) * i;

			if (index == DEBUG_SELECT_BUILDING_SPRITE)
			{
				DEBUG("Sprite Id, Frame: %d -> Pos: %d", i, (int)spriteIndex);
			}
			if (ogSprites && spriteIndex < ogSprites->spriteCount)
			{
				sprite.sprites[i] = ogSprites->sprites[spriteIndex];
				sprite.alphaGrids[i] = ogSprites->alphaGrids[spriteIndex];
			}
			else
			assert(0);

			/*DEBUG("%d %s %s %s", i, VEC_CSTR(pos), VEC_CSTR(buildSize),
		    VEC_CSTR(sprite.spriteCount));*/
		}
		return ret;
	}

	const AssetTexture &get_texture_from_name(const char *name)
	{
		t_texhash_itr texItr = texturesHash.find(str_uppercase(name));
		if (texItr == texturesHash.end())
		{
			WARNING("Texture with name \"%s\" wasn't found, don't forget to add it.", name);
			return nullTexture;
		}
		int textureId = (*texItr).second;

		if (textureId >= textures.size())
		{
			WARNING("Texture with id \"%zu\" wasn't found.", textureId);
			return nullTexture;
		}

		return *textures.at(textureId);
	}

	int load_sprites(
		const char* name,
		const sf::Vector2i& start,
		const sf::Vector2i& end,
		const sf::Vector2i& size = { 1, 1 })
	{
		if (!strcmp(name, ""))
		{
			WARNING("Can't load texture with no name.");
			return -1;
		}

		if (vec_or_equals<int>(start, non) || vec_or_equals<int>(end, non))
		{
			WARNING("Start or end can't be equal to -1 in the x or y axis.");
			return -1;
		}

		// Seach for texture id
		t_texhash_itr texItr = texturesHash.find(str_uppercase(name));
		if (texItr == texturesHash.end())
		{
			WARNING("Texture with name \"%s\" wasn't found, don't forget to add it.", str_uppercase(name).c_str());
			return -1;
		}
		int textureId = (*texItr).second;

		AssetTexture& texture = *textures.at(textureId);
		if (start.x >= (int)texture.frameSize.x ||
			start.y >= (int)texture.frameSize.y ||
			end.x >= (int)texture.frameSize.x ||
			end.y >= (int)texture.frameSize.y)
		{
			WARNING("Texture frame is out of bounds.");
			return -1;
		}
		
		int cx = end.x - start.x + 1;
		int cy = end.y - start.y + 1;

		if (cx <= 0 || cy <= 0)
		{
			WARNING("Animation end frame can't be before the start frame in the x or y axis.");
			return -1;
		}

		AssetKeySprites key = AssetKeySprites{
			std::string(name),
			start.x + start.y * texture.frameCount.x,
			end.x + start.y * texture.frameCount.x };

		// If sprite with the same characteristics was already made
		// return the existing sprite id
		if (spritesHash.count(key))
		{
			return (int)spritesHash.at(key);
		}

		IVec divs = static_cast<IVec>(texture.divisions);
		// Size of a section
		IVec sectionCount = texture.frameCount / divs;
		// Size in pixels
		IVec sectionSize = (IVec)texture.texture.getSize() / divs;

		//if (divs == IVec{ 1,1 }) sectionCount = { 1,1 };

		sprites.push_back(new AssetSprites{});
		int ret = (int)sprites.size() - 1u;
		spritesHash[key] = ret;
		AssetSprites& spriteOut = *sprites.back();
		// sprites will contain the animation frames for the tile
		// If the building has multiple tiles, sprites will also contain-
		// all of the tiles.
		spriteOut.spriteCount = cx * cy * vec_prod(size);
		spriteOut.sprites = new sf::Sprite[spriteOut.spriteCount];
		spriteOut.alphaGrids = new ImageAlphaGrid[spriteOut.spriteCount];
		spriteOut.key = key;
		spriteOut.sectionCount = sectionCount;
		spriteOut.frameCount = texture.frameCount;
		spriteOut.spriteSize = sectionSize;

		if (strcmp(name, DEBUG_SELECT_BUILDING_NAME) == STRCMP_EQUAL)
		{
			DEBUG(name);
			DEBUG("frameCount: %s\ndivs: %s",
				VEC_CSTR(texture.frameCount),
				VEC_CSTR(divs));
			DEBUG(
				"start: %s, end: %s, cx: %d,"
				"cy: %d, size: %s, divs:%s count:%s size:%s",
				VEC_CSTR(start),
				VEC_CSTR(end),
				cx, cy,
				VEC_CSTR(size),
				VEC_CSTR(divs),
				VEC_CSTR(sectionCount),
				VEC_CSTR(sectionSize));
			Logger::new_line();
			//sectionCount = texture.frameCount / divs;
		}

		int index = 0;
		auto sectionToSprites =
			[&spriteOut, &start, &texture, &index,
			 &sectionCount, &sectionSize, &name](
				const IVec &section) {
					
				for (int j = 0; j < sectionCount.y; ++j)
				{
					for (int i = 0; i < sectionCount.x; ++i)
					{
						if (strcmp(
							name,
							DEBUG_SELECT_BUILDING_NAME) ==
							STRCMP_EQUAL)
						{
							DEBUG("\ni, j: %d %d\nsection: %s",
								i,
								j,
								VEC_CSTR(section));
						}
						if (index >= spriteOut.spriteCount)
							continue;
						sf::Sprite &s = spriteOut.sprites[index];
						IVec size = texture.frameSize;
						IVec pos;
						pos = (start + section) * sectionCount;
						pos += IVec{ i, j };
						pos = pos * size;
						sf::IntRect rect{
							pos.x, pos.y, size.x, size.y};

						s.setTexture(texture.texture);
						s.setTextureRect(rect);
						if (strcmp(name, DEBUG_SELECT_BUILDING_NAME) == 
							STRCMP_EQUAL)
						{
							DEBUG("P: %s, S: %s, Frame: %d", 
								VEC_CSTR(pos), 
								VEC_CSTR(size), 
								(int)index);
						}

						ImageAlphaGrid g;
						g = texture.alphaGrid.subsection(
							pos.x, 
							pos.y, 
							size.x, 
							size.y);
						spriteOut.alphaGrids[index] = g;

						++index;
					}
				}
			};

		// Iterate through sections
		for (int j = 0; j < cy; ++j)
		{
			for (int i = 0; i < cx; ++i)
			{
				sectionToSprites({i, j});
			}
		}
		
		if (strcmp(name, DEBUG_SELECT_BUILDING_NAME) == STRCMP_EQUAL)
		{
			for (int i = 0; i < get_sprite(ret)->spriteCount; ++i)
			{
				DEBUG("%d < %d", i, get_sprite(ret)->spriteCount);
				assert(get_sprite(ret)->get(i)->getTexture());
			}
		}

		return (int)ret;
	}

	template <typename U>
	static inline bool vec_or_equals(const sf::Vector2<U> &a, const sf::Vector2<U> &b)
	{
		return a.x == b.x || a.y == b.y;
	}
};

#endif // _GAME_ASSET_MANAGER