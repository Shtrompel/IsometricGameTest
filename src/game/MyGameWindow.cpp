#include "game/MyGameWindow.hpp"

#include "window/WindowManager.hpp"
#include "window/gui_utils.hpp"

#include <cstdlib> // wcstombs_s

struct PopupSaveOverride : PopupConfirmationData
{
	WindowGameplay* window;
	std::wstring fileName;

	PopupSaveOverride(
		std::string title,
		std::string description,
		WindowGameplay* window) :
		PopupConfirmationData(title, description)
	{
		this->window = window;
	}

	void onConfirmation() override
	{
		if (fileName == L"")
		{
			WARNING("fileName on PopupSaveOverride was not set, can't save file.");
			return;
		}

		t_jsonpack jsonPack;
		LOG("Converting game data into Json Pack");
		jsonPack = window->jsonpack_from_game();
		LOG("Save Json Pack to file");
		window->file_write_jsonpack(fileName, jsonPack, false);

		window->gui_open_saved_games(false);
	}

	void postInit() override
	{
		DEBUG("%s", __FUNCTION__);
	}
};

struct PopupDeleteSave : PopupConfirmationData
{
	WindowGameplay* window;
	std::wstring fileName;

	PopupDeleteSave(
		std::string title,
		std::string description,
		WindowGameplay* window) :
		PopupConfirmationData(title, description)
	{
		this->window = window;
	}

	void onConfirmation() override
	{
		if (fileName == L"")
		{
			WARNING("fileName on PopupDeleteSave was not set, can't save file.");
			return;
		}

		window->file_delete(fileName);
		window->gui_open_saved_games(false);
	}

	void postInit() override
	{
		DEBUG("%s", __FUNCTION__);
	}
};

struct PopupLoadGame : PopupConfirmationData
{
	WindowGameplay* window;
	std::wstring fileName;

	PopupLoadGame(
		std::string title,
		std::string description,
		WindowGameplay* window) :
		PopupConfirmationData(title, description)
	{
		this->window = window;
	}

	void onConfirmation() override
	{
		if (fileName == L"")
		{
			WARNING("fileName on PopupLoadGame was not set, can't save file.");
			return;
		}

		t_jsonpack jsonPack;
		LOG("Reading Json Pack from file");
		jsonPack = window->file_read_jsonpack(fileName, false);
		LOG("Loading Json Pack");
		window->jsonpack_to_game(jsonPack);
	}

	void postInit() override
	{
		DEBUG("%s", __FUNCTION__);
	}
};

 std::vector<UpgradeTree*>
recursive_get_last_layer_tree(
	UpgradeTree* step,
	const std::vector<int>& upgrades)
{
	std::vector<UpgradeTree*> ret;

	if (!step)
		return ret;

	auto itr = std::find(
		upgrades.begin(),
		upgrades.end(),
		step->upgradeId);

	if (itr == upgrades.end())
	{
	}
	else
	{
		bool hasAny = false;

		auto& childs = step->children;
		for (auto itr = childs.begin(); itr != childs.end(); ++itr)
		{
			UpgradeTree* s = &*itr;

			auto itr2 = std::find(
				upgrades.begin(),
				upgrades.end(),
				s->upgradeId);
			if (itr2 != upgrades.end())
				hasAny = true;
			else
				continue;

			merge_vectors(
				ret,
				recursive_get_last_layer_tree(s, upgrades));
		}

		if (!hasAny)
			ret.push_back(step);
	}

	return ret;
}

_UNUSED void printBuildQuadTree(ChunkQuadTree<Tile*>::Node* tree)
{
	using T = ChunkQuadTree<Tile*>;

	T::printNode("Node", tree);
	for (auto& point : tree->points)
	{
		T::printPtrPoint("", point);
		printf("Point %d %d\n", point.pos.x, point.pos.y);
		BuildingBody* b = point.type->building;
		printf("Build type %d in %d %d\n", (int)b->base->buildType, b->tilePos.x, b->tilePos.y);
	}

	for (T::Node* node : tree->next)
		if (node)
			printBuildQuadTree(node);
}



WindowGameplay::WindowGameplay()
{
}

WindowGameplay::~WindowGameplay()
{
	delete popupSaveOverride;
	delete popupLoadGame;
	delete popupDeleteSave;

	LOG("Unconstructing GameWindow");
}

bool WindowGameplay::init_window(sf::RenderWindow* window, sf::View* view)
{
	GameWindow::init_window(window, view);

	LOG("GameWindow initialized");
	renderer = RendererClass(window, view, &assets);
	gui.setTarget(*(dynamic_cast<sf::RenderTarget*>(window)));


#define REGISTER(a, b) \
	data.variantFactory.register_variant<a, b>()

	REGISTER(
		SERIALIZABLE_NETWORK,
		PowerNetwork);
	REGISTER(
		SERIALIZABLE_BUILD_BODY,
		BuildingBody);
	REGISTER(
		SERIALIZABLE_BUILD_BASE,
		BuildingBase);
	REGISTER(
		SERIALIZABLE_ENTITY,
		EntityCitizen);
	REGISTER(
		SERIALIZABLE_CITIZEN,
		EntityCitizen);
	REGISTER(
		SERIALIZABLE_ENEMY,
		EntityEnemy);
	REGISTER(
		SERIALIZABLE_CHUNKS,
		Chunks);
	REGISTER(
		SERIALIZABLE_DATA,
		GameData);
	REGISTER(
		SERIALIZABLE_BULLET,
		BulletBody);
	REGISTER(
		SERIALIZABLE_CONSTRUCTION,
		Constructions);

	LOG("%d %d", (int)data.variantFactory.variant_type_to_id<Chunks>(), (int)SERIALIZABLE_CHUNKS);

	popupSaveOverride = new PopupSaveOverride(
		"Override file?",
		"Are you sure you want to override this save? Old save will be lost.",
		this);
	popupDeleteSave = new PopupDeleteSave(
		"Delete file?",
		"Are you sure you want to delete this save? This save will be lost.",
		this);
	popupLoadGame = new PopupLoadGame(
		"Load game?",
		"Are you sure you want to load this save? This game's data might be lost.",
		this);

	return true;
}

bool WindowGameplay::load()
{

	// Load resources
	using namespace nlohmann;

	auto parseJson = [](nlohmann::json& json, FILE* file, const std::string& name) {
		if (!file)
		{
			LOG_ERROR("%s not found", name.c_str());
			return false;
		}
		try
		{
			json = json::parse(file, nullptr, true, true);
		}
		catch (nlohmann::detail::parse_error& e)
		{
			LOG_ERROR("%s parsing error %s", name.c_str(), e.what());
			return false;
		}

		if (json.is_discarded())
		{
			LOG_ERROR("%s parse error", name.c_str());
			return false;
		}
		else
			LOG("%s parsed succesfully", name.c_str());
		return true;
		};

	typedef FILE* t_cfile;

	const std::string FILES_NAMES[] = {
		"enums", "consts",
		"entities", "buildings", "resources",
		"textures", "bullets" };
	const size_t FILES_NAMES_COUNT = sizeof(FILES_NAMES) / sizeof(*FILES_NAMES);
	std::map< std::string, nlohmann::json> jsonMap;

	for (size_t i = 0u; i < FILES_NAMES_COUNT; ++i)
	{
		std::string path;
		path = std::string("assets/json/") + FILES_NAMES[i] + ".json";
		if (!json_file_load(jsonMap[FILES_NAMES[i]], path))
			return false;
	}

	try
	{
		for (auto itr = jsonMap["consts"].begin();
			itr != jsonMap["consts"].end();
			++itr)
		{
			const std::string& key = str_trim(str_uppercase(itr.key()));
			const json& val = itr.value();
			size_t index;
			try
			{
				index = CONSTS_MAP.at(key);
			}
			catch (const std::out_of_range& e)
			{
				WARNING(
					"Constant of string: \"%s\" was not found in CONSTS_MAP: %s",
					key.c_str(),
					e.what());
				continue;
			}

			// DEBUG("%s-%d %s", key.c_str(),(int)index, val.dump().c_str());

			if (val.is_number_float())
				data.constFloating[index] = val.get<float>();
			else if (val.is_number_integer())
				data.constNumeric[index] = val.get<int>();
			else
				continue;
		}
	}
	catch (nlohmann::detail::type_error& e)
	{
		LOG_ERROR("Error while reading from consts.json: %s", e.what());
		return false;
	}

	try
	{
		for (auto& texInfo : jsonMap["resources"]["resources"])
		{
			auto& weights = data.resourceWeights;
			if (!texInfo.contains("name") && 0)
				continue;
			int weight = 1, id = -1;
			std::string name = "";
			name = texInfo["name"].get<std::string>();
			if (texInfo.contains("weight"))
				weight = texInfo["weight"].get<int>();
			if (texInfo.contains("id"))
				id = texInfo["id"].get<int>();

			if (id == -1)
			{
				id = (int)weights.next_key();
				weights.add(id, str_uppercase(name), weight);
			}
			else
			{
				id = (int)weights.push_and_move(id, str_uppercase(name), weight);
			}
		}
	}
	catch (nlohmann::detail::type_error& e)
	{
		LOG_ERROR("Can't load resources from data.json because of a json parsing error: %s", e.what());
		return false;
	}
	LOG("Successfuply loaded resources from data.json");

	if (0) // Show resources weights
		for (auto& x : data.resourceWeights)
		{
			std::string str = data.resourceWeights.get_str(x.first);
			DEBUG("%d %s %d", (int)x.first, str.c_str(), (int)x.second);
		}

	// Generate null/missing texture
	assets.generate_null_texture();

	// Load texture data
	for (auto& texInfo : jsonMap["textures"]["textures"])
	{
		if (!texInfo.contains("name"))
			continue;

		IVec count = { 1, 1 };
		IVec divisions = { 1, 1 };
		if (texInfo.contains("count"))
		{
			auto nums = str_split(texInfo["count"], ",");
			if (nums.size() >= 2)
				count = {
					str_to_int(nums[0]),
					str_to_int(nums[1]) };
		}
		if (texInfo.contains("divisions"))
		{
			auto nums = str_split(texInfo["divisions"], ",");
			if (nums.size() >= 2)
				divisions = {
					str_to_int(nums[0]),
					str_to_int(nums[1]) };
		}
		else
		{
			divisions = count;
		}

		try
		{
			std::string name = texInfo["name"].get<std::string>();
			assets.load_texture(name.c_str(), count, divisions);
		}
		catch (nlohmann::detail::type_error& e)
		{
			WARNING("Can't load texture because of a json parsing error: %s", e.what());
		}
	}
	LOG("Successfuply load textures");

	std::map<std::string, size_t> mapSerTree;

	if (!load_enum_trees(
		jsonMap["enums"]["enums"],
		data.mapEnumTree))
	{
		ASSERT_ERROR(false, "Can't load upgrade tree.");
		return false;
	}
	LOG("Successfuply load enums");

	if (!load_serializable(
		jsonMap["enums"]["serializables"],
		mapSerTree))
	{
		ASSERT_ERROR(false, "Can't load serializables.");
		return false;
	}
	LOG("Successfuply load serializables");

	if (!load_upgrade_trees(
		data.bodyConfigMap,
		assets,
		data.mapEnumTree,
		mapSerTree,
		&data.resourceWeights,
		jsonMap["buildings"]))
	{
		ASSERT_ERROR(false, "Can't load upgrade tree.");
		return false;
	}
	LOG("Successfuply load buildings");

	if (!load_entity_stats(
		data.bodyConfigMap,
		&data,
		assets,
		data.mapEnumTree,
		mapSerTree,
		jsonMap["entities"]))
	{
		ASSERT_ERROR(false, "Can't load entity stats.");
		return false;
	}
	LOG("Successfuply load entities");

	if (!load_bullets_stats(
		data.bodyConfigMap,
		&data,
		assets,
		data.mapEnumTree,
		jsonMap["bullets"]))
	{
		ASSERT_ERROR(false, "Can't load bullets stats.");
		return false;
	}
	LOG("Successfuply load bullets");

	//for (auto& x : data.bodyConfigMap.at(ENUM_BULLET_TYPE))
	//{
	//	DEBUG("%s", x.second.at(0)->to_string().c_str());
	//}


	renderer.spriteGrass =
		assets.load_sprites("tile", { 0, 0 }, { 0, 0 });
	renderer.spriteTileSelected =
		assets.load_sprites("tile", { 1, 0 }, { 1, 0 });
	renderer.spriteEntity =
		assets.load_sprites("entity", { 0, 0 }, { 1, 0 });
	renderer.spriteEnemy =
		assets.load_sprites("enemy", { 0, 0 }, { 1, 0 });
	renderer.spriteConstruction =
		assets.load_sprites("other", { 0, 0 }, { 0, 0 });
	renderer.spriteIconDelete =
		assets.load_sprites("other", { 1, 0 }, { 1, 0 });
	renderer.spriteIconBuild =
		assets.load_sprites("other", { 2, 0 }, { 2, 0 });
	renderer.spriteTmpBullet =
		assets.load_sprites("bullet", { 0, 0 }, { 7, 0 });


	if (!font.loadFromFile("assets/OpenSans-Regular.ttf"))
	{
		LOG_ERROR("Missing file OpenSans-Regular.ttf");
	}
	else
		LOG("File OpenSans-Regular.ttf loaded aucceafully");

	{
		sf::Font font2;
		if (!font2.loadFromFile("assets/fonts/bankgothic-regular.ttf"))
		{
			LOG_ERROR("Unable to load font bankgothic-regular.ttf");
			assert(0);
		}

		sf::Text text;
		text.setCharacterSize(30);
		text.setFillColor(sf::Color::Black);
		text.setFont(font2);

		charSpriteSheet.init(font2, text);
	}

	return true;

}

bool WindowGameplay::init()
{

	LOG("\tInit start");

	text.setFont(font);
	text.setCharacterSize(16);
	text.setFillColor(sf::Color::Black);

	LOG("Gui start");

	std::string guiFormPath = 
		std::filesystem::current_path().string() + "\\assets\\gui\\FormGame.txt";
	try
	{
		gui.loadWidgetsFromFile(guiFormPath);
	}
	catch (std::exception const& e)
	{
		LOG_ERROR(
			"Unable to call loadWidgetsFromFile. "
			"Error: %s", 
			e.what());
		return false;
	}

	get_widget<tgui::Button>("ButtonBuild")->onPress([](WindowGameplay* gameWindow) {
		//gameWindow->gui_change_group("GroupBuild");
		tgui::ChildWindow::Ptr w1;
		w1 = gameWindow->gui.get<tgui::ChildWindow>("WindowConstructions");
		assert(w1);
		w1->setVisible(!w1->isVisible());
		gameWindow->gameMode = GameMode::BUILD;
		}, this);
	get_widget<tgui::Button>("ButtonRemove")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->gui_change_group("GroupBuild");
		gameWindow->gameMode = GameMode::DELETE;
		}, this);
	get_widget<tgui::Button>("ButtonUpgrade")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->gui_change_group("GroupBuild");
		gameWindow->gameMode = GameMode::UPGRADE;
		}, this);
	get_widget<tgui::Button>("ButtonBuildCancel")->onPress([](WindowGameplay* gameWindow) {
		for (const tgui::Widget::Ptr& widget : gameWindow->gui.getWidgets())
		{
			gameWindow->gui_change_group("GroupMain");
			gameWindow->gameMode = GameMode::VIEW;
			gameWindow->suggestionBuilds.clear();
			gameWindow->enableDraw = false;
			gameWindow->gui_hide_upgrade_info();
		}
		}, this);

	get_widget<tgui::Button>("ButtonMenuLoad")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->gui_open_saved_games(false);
		}, this);
	get_widget<tgui::Button>("ButtonMenuSave")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->gui_open_saved_games(true);
		}, this);

	get_widget<tgui::Button>("ButtonContinue")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->get_widget < tgui::ChildWindow>("WindowMenu")->setVisible(false);
		}, this);
	get_widget<tgui::Button>("ButtonMenu")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->get_widget < tgui::ChildWindow>("WindowMenu")->setVisible(true);
		}, this);

	get_widget<tgui::ChildWindow>("WindowMenu")->onClosing([](WindowGameplay* gameWindow, bool* abort) {
		gameWindow->get_widget < tgui::ChildWindow>("WindowMenu")->setVisible(false);
		*abort = true;
		}, this);

	get_widget<tgui::Button>("ButtonSettings")->onPress([](
		WindowGameplay* gameWindow,
		WindowManager* manager) {

			guiutils_settings(manager, gameWindow->gui);

		}, this, managerParent);

	get_widget<tgui::Button>("ButtonExit")->onPress([](
		WindowGameplay* gameWindow,
		WindowManager* manager) {

			GameWindow* window = manager->get_window_from_id("MENU");
			manager->change_window(window);

		}, this, managerParent);
	get_widget<tgui::Button>("ButtonRotate")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->rotate_world(1);
		}, this);
	get_widget<tgui::Button>("ButtonUpgrade")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->gameMode = GameMode::UPGRADE;
		}, this);
	get_widget<tgui::Button>("ButtonConstructionBack")->onClick(
		[](tgui::Gui& gui) {
			tgui::ChildWindow::Ptr w1, w2;
			w1 = gui.get<tgui::ChildWindow>("WindowConstructions");
			w2 = gui.get<tgui::ChildWindow>("WindowConstruction");
			if (w1 && w2)
			{
				w1->setVisible(true);
				w2->setVisible(false);
			}
			else
				assert(0);
		}, std::ref(gui));
	gui.get<tgui::ChildWindow>("WindowConstructions")->onClosing(
		[](tgui::Gui& gui, bool* abort) {
			tgui::ChildWindow::Ptr w1;
			w1 = gui.get<tgui::ChildWindow>("WindowConstructions");
			w1->setVisible(false);
			*abort = true;
		}, std::ref(gui));
	gui.get<tgui::ChildWindow>("WindowConstruction")->onClosing(
		[](tgui::Gui& gui, bool* abort) {
			tgui::ChildWindow::Ptr w1;
			w1 = gui.get<tgui::ChildWindow>("WindowConstruction");
			w1->setVisible(false);
			*abort = true;
		}, std::ref(gui));

	get_widget<tgui::Button>("ButtonPaint")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->enableDraw = !gameWindow->enableDraw;
		}, this);
	get_widget<tgui::Button>("ButtonGridCancel")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->gui_disable_group("GroupGridBuild");
		gameWindow->suggestionBuilds.clear();
		gameWindow->gui_hide_upgrade_info();
		}, this);

	get_widget<tgui::Button>("ButtonGridApply")->onPress([](WindowGameplay* gameWindow) {
		gameWindow->gui_disable_group("GroupGridBuild");

		switch (gameWindow->gameMode)
		{
		case GameMode::BUILD:
		{
			UpgradeTree* tree = dynamic_cast<UpgradeTree*>(
				gameWindow->buildSelected[0]);
			assert(tree);
			for (auto& tp : gameWindow->suggestionBuilds)
				gameWindow->data.queue_add_build(
					tp,
					tree, gameWindow->enableConstruction, !gameWindow->godMode);
			gameWindow->suggestionBuilds.clear();
			break;
		}
		case GameMode::DELETE:
		{
			std::vector<BuildingBase*> builds;
			for (auto& tp : gameWindow->suggestionBuilds)
			{
				BuildingBody* body =
					gameWindow->data.chunks->get_build(tp.x, tp.y);
				if (body &&
					std::find(builds.begin(), builds.end(), body->base) == builds.end())
				{
					builds.push_back(body->base);
					body->base->flagDelete = true;
				}
			}
			gameWindow->suggestionBuilds.clear();
		}
		break;
		case GameMode::UPGRADE:
		{
			if (gameWindow->upgradeTree.empty())
				break;
			std::vector<BuildingBase*> builds;
			UpgradeTree* tree = *gameWindow->upgradeTreeItr;
			assert(tree);
			for (auto& tp : gameWindow->suggestionBuilds)
			{
				BuildingBody* body =
					gameWindow->data.chunks->get_build(tp.x, tp.y);
				if (body &&
					std::find(builds.begin(), builds.end(), body->base) == builds.end())
				{
					builds.push_back(body->base);
					gameWindow->upgrade_building(body->base, tree);
				}
			}
			gameWindow->suggestionBuilds.clear();
		}
		break;
		default:
			gameWindow->suggestionBuilds.clear();
			break;
		}
		}, this);

	gui_init_resource_tab();
	gui_update_constrs_window();


	LOG("Gui end");

	unsigned randSeed = 0;
	prng_seed_bytes((const unsigned char*)&randSeed, sizeof(randSeed));

	Chunks* chunksPtr = &chunks;
	auto generateChunks = [&chunksPtr](int radius) {
		// Initialize grid chunks
		int areaWidth = radius * 2 + 1;
		for (int i = 0; i < areaWidth * areaWidth; ++i)
		{
			Grid* grid = new Grid(
				CHUNK_W,
				CHUNK_H,
				i % areaWidth - areaWidth / 2,
				i / areaWidth - areaWidth / 2);
			chunksPtr->add(grid);
		}
		};

	if (0) {
		bool success;
		t_idpair pair = data.mapEnumTree.get_id("Park", &success);
		if (!success)
		{
			assert(0);
		}
		UpgradeTree* step = nullptr;
		step = dynamic_cast<UpgradeTree*>(
			data.bodyConfigMap.at(pair.group, pair.id)[0]);
		for (auto& x : step->spriteHolders)
		{
			auto a = assets.get_sprite(x.second)->get(0);
			assert(a);
			if (!a->getTexture())
				continue;
			a->getTexture()->copyToImage().saveToFile("toto.png");
		}
	}

	const auto autoAddResources = [this](
		int ores = 20,
		int gems = 6,
		int radius = 32) {
			BuildingBase* b = nullptr;
			Logger::set_priority(0);

			for (int i = 0; i < ores + gems; ++i)
			{
				sf::Vector2i pos;
				pos.x = (int)(radius * (prng_get_double() * 2 - 1));
				pos.y = (int)(radius * (prng_get_double() * 2 - 1));

				int maxCount = 10, minCount = 50;
				BuildingType buildType = BuildingType::RAW_ORE;
				int resourceId = 0;

				if (i > ores)
				{
					minCount = 3;
					maxCount = 15;
					resourceId = 1;
					buildType = BuildingType::RAW_GEMS;
				}

				b = data.add_building(
					pos, buildType, 0);
				if (!b)
					continue;
				b->rStorage[resourceId] = (int)math_floor(
					prng_get_double() *
					(maxCount - minCount + 1) +
					minCount);
			}
			Logger::set_priority(99);
		};

	list_saved_games();

	// Tests
	LOG("\tTest start");

	this->enableSaving = 1;

	static const int TEST_NOTHING = 7;
	static const int TEST_MAZE = 0;
	static const int TEST_FIGHTERS = 1;
	static const int TEST_PATHFIND = 2;
	static const int TEST_SYSTEM2 = 4;
	static const int TEST_GAME = 5;
	static const int TEST_MINIMUM = 8;
	static const int TEST_ENEMY = 9;
	static const int TEST_BULLETS = 10;
	static const int TEST_MASS_PATHFIND = 11;
	static const int TEST_SHOOTING = 12;
	static const int TESTS_COUNT = 13;


	int selected = TEST_SHOOTING;
	std::function<void(void)> tests[TESTS_COUNT];
	tests[TEST_SHOOTING] = [this, &generateChunks, &autoAddResources]()
		{
			generateChunks(2);


			BuildingBase* b;
			b = data.add_building(
				{ 0, 0 },
				BuildingType::HOME, 0);
			b = data.add_building(
				{ 0, 1 },
				BuildingType::HOME, 0);

		 b = data.add_building(
				{ 2, 0 },
				BuildingType::GENRATOR, 0);
		 b = data.add_building(
			 { 2, 1 },
			 BuildingType::GENRATOR, 0);

		 b = data.add_building(
				{ 4, 0 },
				BuildingType::CAPACITOR, 0);

			return;

			// focus_on(b->get_center_pos());

			for (int i = 0; i < 1; ++i)
			{
				BuildingBase* b = data.add_building(
					{ 2 + i * 2, -1 },
					BuildingType::TOWER, 0);
				b->alignment = ALIGNMENT_FRIENDLY;
				if (b->get_upgrades().size())
					upgrade_building(b, b->get_upgrades().back());
			}

			for (int i = 0; i < 0; i++)
			{
				float x = (float)prng_get_double() * 10.f;
				float y = (float)prng_get_double() * 10.f;
				//x = 2, y = 0;
				auto s1 = data.get_entity_stats(
					ENUM_CITIZEN_JOB,
					(t_id)CitizenJob::NONE);
				EntityCitizen* e = data.add_entity_citizen(
					{ x, y },
					s1);
			}

			for (int i = 0; i < 0; i++)
			{
				float x = (float)prng_get_double() * 10.f;
				float y = (float)prng_get_double() * 5.f + 5;
				//x = 2, y = 0;
				auto s1 = data.get_entity_stats(
					ENUM_ENEMY_TYPE,
					(t_id)EnemyType::BRUTE);
				EntityEnemy* e = data.add_entity_enemy(
					{ x, y },
					s1);
			}
		};


	tests[TEST_FIGHTERS] = [this, &generateChunks, &autoAddResources]()
		{
			generateChunks(2);

			for (int i = 0; i < 4; ++i)
				BuildingBase* b = data.add_building(
					{ 2 + i * 2, -1 },
					BuildingType::ARMORY, 0);

			for (int i = 0; i < 10; ++i)
			{
				{
					auto s1 = data.get_entity_stats(
						ENUM_ENEMY_TYPE,
						(t_id)EnemyType::BRUTE);
					float x = (float)prng_get_double() * 10.f;
					float y = (float)prng_get_double() * 5.f + 5;
					EntityEnemy* e = data.add_entity_enemy(
						{ x, y },
						s1);
				}
				{
					auto s1 = data.get_entity_stats(
						ENUM_CITIZEN_JOB,
						(t_id)CitizenJob::NONE);
					float x = (float)prng_get_double() * 10.f;
					float y = (float)prng_get_double() * 5.f;
					EntityCitizen* e = data.add_entity_citizen(
						{ x, y },
						s1);
				}
			}
		};

	tests[TEST_MASS_PATHFIND] = [this, &generateChunks, &autoAddResources]()
		{
			generateChunks(3);

			BuildingBase* b;
			const int C = 40;
			for (int y = 0; y < C; ++y)
			{
				for (int x = 0; x < C; ++x)
				{
					b = data.add_building(
						{ x*2, y*2 }, 
						BuildingType::HOME, 0);
					assert(b);
					b->spawn = data.spawn_info_get(
						SERIALIZABLE_CITIZEN,
						{ (t_group)ENUM_CITIZEN_JOB ,(t_id)CitizenJob::TEST01});
					b->entityLimit = 1;
					b->actionTimer->set_length(0.01f);

					focus_on((sf::Vector2f)b->get_center_pos());
				}
			}

		};

	tests[TEST_SYSTEM2] = [this, &generateChunks, &autoAddResources]()
		{
			generateChunks(3);

			autoAddResources(20, 6, 16);

			bool extended = false;
			BuildingBase* b;

			b = data.add_building({ 0, 0 }, "MINERS_POST", 0);
			b->rStoreCap = { 0, 0, 0 };
			if (!extended)
			{
				b->rStorage[Resources::ORE] = 30;
				b->entityLimit = 0;
			}

			if (extended)
			{
				b = data.add_building({ 1, 0 }, "HOME", 0);
			}
			b = data.add_building({ 0, 2 }, "STORAGE", 0);
			b->rStorage[Resources::ORE] = 50;

			b = data.add_building({ 0, 4 }, "LOGISTICS_CENTER", 0);
			b = data.add_building({ 1, 4 }, "HOME", 0);

			focus_on((sf::Vector2f)b->tilePos);

			if (extended)
			{
				b = data.add_building({ 0, 6 }, "GENERATOR", 0);
				b->load_upgrade_step(b->get_upgrades().at(0));
				b = data.add_building({ 1, 6 }, "HOME", 0);

				b = data.add_building({ 2, 6 }, "CAPACITOR", 0);
			}
		};

	tests[TEST_BULLETS] = [this, &generateChunks]() {
		generateChunks(3);

		auto s1 = data.get_entity_stats(
			ENUM_ENEMY_TYPE,
			(t_id)EnemyType::BRUTE);

		assert(s1);
		data.add_entity_enemy(
			{ 8.f, 8.f },
			s1);

		for (int i = 0; i < 9; i++)
		{
			if (i == 4) continue;

			UpgradeTree* ut = dynamic_cast<UpgradeTree*>(
				data.bodyConfigMap.at(ENUM_BUILDING_TYPE, (t_id)BuildingType::WALL)[0]);
			ut->alignment = ALIGNMENT_ENEMY;
			BuildingBase* bb = data.add_building(
				IVec{ 7 + (i % 3), 7 + (i / 3) },
				ut,
				false);
		}

		BulletBody* bb;
		BulletStats* bulletStat =
			data.get_generic_stats<BulletStats>(
				ENUM_BULLET_TYPE,
				(t_id)BulletType::ENEMY_BULLET_01);
		if (0)
			for (int i = 0; i < 25; i++)
			{
				int x = (i % 5);
				int y = (i / 5);
				if (1 <= x && x < 5 && 1 <= y && y < 5) continue;
				bb = dynamic_cast<BulletBody*>(data.add_game_body(
					SERIALIZABLE_BULLET,
					bulletStat,
					{ 6.5f + (float)x, 6.5f + (float)y }));
			}

		focus_on({ 8, 8 });
		};

	tests[TEST_ENEMY] = [this, &generateChunks]() {
		generateChunks(0);

		auto bBuild = data.add_building({ 5, 5 }, BuildingType::HOME, false);
		bBuild->entityLimit = 5;

		Logger::set_priority(0);
		for (int x = -2; x <= 2; ++x)
		{
			for (int y = -2; y <= 2; ++y)
			{
				if (y <= 0 && x == 0)
					continue;
				data.add_building(
					{ 5 + x, 5 + y },
					BuildingType::WALL, false);
			}
		}
		Logger::set_priority(99);

		data.add_building({ 5, 10 }, BuildingType::ENEMY_SPAWN, false);
		focus_on({ 5, 5 });
		};

	tests[TEST_MINIMUM] = [this, &generateChunks]() {
		generateChunks(0);

		data.add_building({ 0, 0 }, BuildingType::CAPACITOR, false);
		data.add_entity_citizen(
			{ 1, 1 }, data.get_entity_stats(
				ENUM_CITIZEN_JOB,
				(t_id)CitizenJob::NONE));
		focus_on({ 1, 1 });
		};

	auto postLoad = [this]() {
		return;
		assert(data.entityCitizens.size());
		for (auto& e : data.entityCitizens)
		{
			DEBUG("%d", (int)e->objectId);
			DEBUG("%s", e->workplace->name);
			DEBUG("%d %s",
				(int)e->action,
				e->pathData.destination->building->base->name);
			DEBUG("%d",
				(int)data.constructions.count(
					e->pathData.destination->building->base));
			printf("\n");
		}
		};

	tests[TEST_GAME] = [this, &generateChunks, &autoAddResources]() {
		generateChunks(1);
		BuildingBase* b = nullptr;

		b = data.add_building({ 2, 8 }, "HOME", 0);


		b = data.add_building(
			{ 3, 2 },
			"Enemy_Spawn",
			0);

		// data.add_entity_enemy({4, 4}, data.get_entity_stats("EnemyType::Brute"));

		};

	tests[TEST_NOTHING] = []() {};

	// Generate maze from image
	tests[TEST_MAZE] = [this, &generateChunks]() {
		generateChunks(1);

		sf::Vector2i start = { 0, 0 }, end = { 7, 17 };
		sf::Image mazeImg;
		assert(mazeImg.loadFromFile("assets/maze.png"));
		Logger::set_priority(0);
		BuildingBase* b;
		LOOP(i, mazeImg.getSize().x)
		{
			LOOP(j, mazeImg.getSize().y)
			{
				sf::Color c = mazeImg.getPixel((unsigned)i, (unsigned)j);
				sf::Vector2i p = sf::Vector2i{ (int)i, (int)j };
				switch (c.toInteger())
				{
				case 0x000000FF:
					data.add_building(p, BuildingType::WALL, false);
					break;
				case 0x0000FFFF:
					b = data.add_building(p, BuildingType::HOME, false);
					b->entityLimit = 50;
					b->actionTimer->set_length(0.1f);
					break;
				case 0xFF0000FF:
					b = data.add_building(p, BuildingType::GENRATOR, false);
					b->entityLimit = 50;
					break;
				case 0xFFFFFFFF:
					end = p;
					if (data.add_building(p, BuildingType::ROAD, false))
					{
					}
					break;
				default:
					printf("%s %d %x\n", FILE_NAME, __LINE__, c.toInteger());
					break;
				}
			}
		}
		Logger::set_priority(10);
		};

	// Pathfinding and job test
	tests[TEST_PATHFIND] = [this, &generateChunks]() {
		generateChunks(1);
		BuildingBase* b = nullptr;

		Logger::set_priority(0);
		//priority = 0;
		for (int i = 0; i < 0; ++i)
			b = data.add_building(
				{ prng_get_byte() / 16, prng_get_byte() / 16 },
				BuildingType::ROAD, 0);
		Logger::set_priority(Logger::LogType::Debug);

		// Ore miner
		b = data.add_building({ 10, 6 }, "HOME", 0);
		assert(b);
		b->actionTimer->set_length(0.01f);

		b = data.add_building({ 11, 11 }, BuildingType::RAW_ORE);
		assert(b);
		b->rStorage[Resources::ORE] = 15;
		b = data.add_building({ 4, 9 }, BuildingType::RAW_ORE);
		assert(b);
		b->rStorage[Resources::ORE] = 30;
		b = data.add_building({ 13, 11 }, BuildingType::RAW_ORE);
		assert(b);
		b->rStorage[Resources::ORE] = 45;

		b = data.add_building({ 11, 7 }, BuildingType::MINERS_POST, 0);
		assert(b);

		// Power production
		b = data.add_building({ 15, 8 }, BuildingType::GENRATOR, 0);
		assert(b);
		focus_on((sf::Vector2f)b->tilePos);
		rotate_world(0);

		b->load_upgrade_step(b->get_upgrades()[0]);
		b = data.add_building({ 17, 10 }, BuildingType::STORAGE, 0);
		assert(b);
		b->rStorage[Resources::ORE] = 20;

		b = data.add_building({ 16, 9 }, BuildingType::CAPACITOR, 0);
		assert(b);
		b = data.add_building({ 17, 11 }, "HOME", 0);
		assert(b);
		};

	tests[selected]();

	// guiButtons["mode delete"].click();
	// guiButtons["next build"].click();

	//postLoad();

	LOG("\tTest end");
	LOG("\tInit end");

	return true;
}

void WindowGameplay::close()
{
	LOG("\tClose start");
	LOG("\tClose end");
}

void WindowGameplay::update(float delta)
{
	// Todo account add_build
	bool gridChange = false;
	t_seconds start = data.get_time();
	static t_seconds max = 0.0f;

	// Remove buildings that are marked for deletion
	auto& buildings = data.buildingBases;

	

	// Add all building that pending to be added
	while (data.buildingQueue.size())
	{
		data.add_building(data.buildingQueue.front());
		data.buildingQueue.pop_front();
	}

	

	// Calculate total power
	data.powerTotal = 0;
	for (auto& network : data.networks)
	{
		network->powerValue =
			math_min(
				network->powerStore,
				network->powerOut);
		network->storeCount = network->powerValue;
		data.powerTotal += network->storeCount;
	}

	data.resources = Resources::res_empty(&data.resourceWeights);
	data.power = 0;
	auto itr = buildings.begin();

	while (itr != buildings.end())
	{
		bool removed = false;
		BuildingBase* b = (*itr);
		assert(b);
		b->update();

		// Calculate power networks power values
		if (b->props.bool_is(PropertyBool::POWER_NETWORK) &&
			b->network)
		{
			auto& network = *b->network;
			if (b->powerIn)
			{
				network.powerValue -= b->powerIn;
				b->sufficient = network.powerValue >= 0;
			}

			if (b->powerStore)
			{
				b->powerValue = math_min(
					network.storeCount,
					b->powerStore);
				network.storeCount -= b->powerValue;
				b->powerValue = math_max(0, b->powerValue);

				data.power += b->powerValue;
			}
		}

		// Update info window
		if (b->updateInfo)
		{
			b->updateInfo = false;
			if (has_build_info(b))
			{
				show_build_info(b, true);
			}
		}

		// Calculate all storage
		if (b->is_any_storage() &&
			!b->props.bool_is(PropertyBool::HARVESTABLE))
		{
			data.resources += b->rStorage;
		}

		// Delete buildings that are flagged to be deleted
		// Must be last
		if (b->flagDelete)
		{
			itr = data.delete_building(b->tilePos);
			removed = true;
			gridChange = true;
		}

		gui_update_resources_tab(UiResources::BODIES, data.resources[Resources::BODIES]);
		gui_update_resources_tab(UiResources::ORE, data.resources[Resources::ORE]);
		gui_update_resources_tab(UiResources::GEM, data.resources[Resources::GEMS]);
		gui_update_resources_tab(UiResources::ELECTRICITY, data.powerTotal);
		gui_update_resources_tab(UiResources::PEOPLE, (int)data.entityCitizens.size());

		// If not removed, go the the next building
		if (!removed)
			itr++;
	}


	size_t removedEntities = 0u;
	for (auto itr = data.bodies.begin(); itr != data.bodies.end(); )
	{
		GameBody* body = *itr;
		assert(body);

		if (body->dead)
		{
			for (auto& x : data.bodyQueue)
			{
				if (x.target == body)
					x.target = nullptr;
			}

			if (body->type == BodyType::ENTITY)
			{
				data.delete_entity(dynamic_cast<EntityBody*>(body));
			}
			
			data.delete_game_body_generic(body);
			itr = data.bodies.erase(itr);

			continue;
		}
		else
			++itr;
		
		// Movement
		body->update(delta);
		
		if (body->type == BodyType::ENTITY)
		{
			EntityBody* entity = dynamic_cast<EntityBody*>(body);
			if (entity->updateInfo)
			{
				entity->updateInfo = false;
				if (has_entity_info(entity))
				{
					show_entity_info(entity, true);
				}
			}
		}
		
	}

	while (!data.entityQueue.empty())
	{
		BuildingBase* build = data.entityQueue.front();
		data.add_entity_citizen(build);
		data.entityQueue.pop_front();
	}

	while (!data.bodyQueue.empty())
	{
		const BodyQueueData bodyData =
			data.bodyQueue.back();
		data.bodyQueue.pop_back();

		const SpawnInfo& spawn =
			bodyData.spawn;

		bool isEntity = false;
		// Is the spawn info spawns an entity
		if (bodyData.spawn.id.group == ENUM_CITIZEN_JOB ||
			bodyData.spawn.id.group == ENUM_ENEMY_TYPE ||
			(bodyData.spawn.id.group == ENUM_BODY_TYPE &&
				bodyData.spawn.id.id == (t_id)BodyType::ENTITY))
		{
			isEntity = true;
		}

		if (isEntity && bodyData.home)
		{
			if (bodyData.home->storedEntities.size() >=
				bodyData.home->entityLimit)
				continue;
		}

		if (bodyData.home && 
			bodyData.home->entities.size() >=
			bodyData.home->entityLimit)
		{
			continue;
		}

		GameBody* body;
		body = data.add_game_body(
			spawn.serializableID,
			spawn.id.group,
			spawn.id.id,
			bodyData.pos);

		assert(body);
		if (!body)
		{
			WARNING("Body insertion failed! %s - %s", 
				spawn.id.to_string().c_str(),
				(int)spawn.serializableID);
		}

		if (isEntity)
		{
			assert(body->type == BodyType::ENTITY);
			EntityBody* entity = dynamic_cast<EntityBody*>(body);
			assert(entity);

			if (entity->entityType == EntityType::CITIZEN)
			{
				EntityCitizen* citizen = dynamic_cast<EntityCitizen*>(entity);
				citizen->home = bodyData.home;
			}

			if (bodyData.home)
			{
				bodyData.home->entities.push_back(entity);
				bodyData.home->updateInfo = true;
			}
		}

		if (bodyData.target)
		{
			assert(body != dynamic_cast<GameBody*>(bodyData.target));
			body->set_target(bodyData.target);
		}


		body->vel = bodyData.vel;
		body->alignment = bodyData.alignment;
	}

	

	data.addedTiles.clear();
	data.removedTiles.clear();
	float dif = data.get_time() - start;
	max = std::max(dif, max);

	++data.frameCount;
}

void WindowGameplay::render()
{
	data.depth_sort(renderer.orientation.rotation);
	renderer.render_grid(&chunks);

	GameBody* selectedBody = nullptr;

	if ((selectedBody = renderer.render_objects(
		data.bodies,
		searchEntity,
		searchEntityPos)))
	{
		switch (selectedBody->type)
		{
		case BodyType::ENTITY:
		{
			EntityBody* entity =
				dynamic_cast<EntityBody*>(selectedBody);
			show_entity_info(entity);
			break;
		}
		case BodyType::BUILDING:
		{
			BuildingBody* b =
				dynamic_cast<BuildingBody*>(selectedBody);
			this->data.infoBuild = b->base;
			show_build_info(b->base);
			break;
		}
		default:
			AAAA;
		}
	}

	searchEntity = false;

	// If a building aws selected
	if (suggestionBuilds.size())
	{
		UpgradeTree* tree = nullptr;
		if (buildSelected.size())
			tree = dynamic_cast<UpgradeTree*>(buildSelected[0]);
		renderer.render_suggestion(
			tree,
			&chunks,
			gameMode,
			suggestionBuilds);
	}

	tgui::String str = tgui::String::fromNumber(fps);
	str += '\n';
	IVec mouseTilePos = screen_pos_to_tile_pos(mousePos, renderer.orientation);
	str += vec_str(mouseTilePos);

	get_widget<tgui::Label>("LabelFramerate")->setText(
		str);

	gui.draw();
}

void WindowGameplay::mouse_start(const sf::Vector2i& mousePos)

{


	IVec m = screen_pos_to_tile_pos(
		mousePos,
		renderer.orientation);
	drawInsert = true;
	// Remove / add buildings to the suggestionBuilds list
	if (enableDraw && chunks.is_free(m.x, m.y) != (gameMode == GameMode::DELETE))
	{
		drawInsert = !suggestionBuilds.count(m);
		if (drawInsert)
			suggestionBuilds.erase(m);
		else
			suggestionBuilds.insert(m);
	}
}

void WindowGameplay::mouse_dragged(const sf::Vector2i& mousePos, const sf::Vector2i& pmousePos)

{
	auto m0 = screen_pos_to_tile_pos(
		pmousePos,
		renderer.orientation);
	auto m1 = screen_pos_to_tile_pos(
		mousePos,
		renderer.orientation);
	if (enableDraw &&
		m0 != m1 &&
		chunks.is_free(m1.x, m1.y) != (gameMode == GameMode::DELETE))
	{
		bool has = suggestionBuilds.count(m1);
		if (has && !drawInsert)
		{
			suggestionBuilds.erase(m1);
		}
		else if (!has && drawInsert)
		{
			suggestionBuilds.insert(m1);
		}

		grid_change_confirmation(m0);
		return;
	}

	if (!enableDraw)
		mouse_zoom_dragged(mousePos, pmousePos);
}

void WindowGameplay::mouse_released(const sf::Vector2i& mousePos)
{
}

void WindowGameplay::mouse_pressed(const sf::Vector2i& mousePos)

{
	sf::Vector2i p = screen_pos_to_tile_pos(
		mousePos,
		renderer.orientation);

	if (isBtnPressed)
	{
		isBtnPressed = false;
		return;
	}

	if (gameMode == GameMode::VIEW)
	{
		BuildingBody* b;
		if ((b = data.chunks->get_build(p.x, p.y)))
		{
			this->data.infoBuild = b->base;
			show_build_info(b->base);
			return;
		}
		else
		{
			searchEntity = true;
			searchEntityPos = (FVec)mousePos;
		}
	}

	switch (gameMode)
	{
	case GameMode::DELETE:
	{
		if (!data.chunks->has_tile(p.x, p.y))
			break;
		BuildingBody* body = data.chunks->get_tile(p.x, p.y).building;
		if (body && !body->base->props.bool_is(PropertyBool::UNREMOVABLE))
		{
			if (suggestionBuilds.count(p))
				for (auto& b : body->base->get_bodies())
					suggestionBuilds.erase(b->tilePos);
			else
				for (auto& b : body->base->get_bodies())
					suggestionBuilds.insert(b->tilePos);
		}
	}
	break;
	case GameMode::BUILD:

		if (buildSelected.empty())
			break;

		if (suggestionBuilds.count(p))
			suggestionBuilds.erase(p);
		else
			suggestionBuilds.insert(p);
		// If pressed on  a building, show it's data.
		break;
	case GameMode::UPGRADE:
	{
		BuildingBody* body = data.chunks->get_tile(p.x, p.y).building;
		if (body)
		{
			this->data.infoBuild = body->base;
			auto pos = body->tilePos;
			if (suggestionBuilds.count(pos))
			{
				suggestionBuilds.erase(pos);
				gui_hide_upgrade_info();
			}
			else
			{
				suggestionBuilds.insert(pos);
				gui_show_upgrade_info(body->base);
			}
		}
	}
	break;
	default:
		break;
	}

	switch (gameMode)
	{
	case GameMode::DELETE:
	case GameMode::BUILD:
	case GameMode::UPGRADE:
		grid_change_confirmation(p);
		break;
	default:
		break;
	}
}

void WindowGameplay::mouse_zoom_dragged(const sf::Vector2i& mousePos, const sf::Vector2i& pmousePos)
{
	auto dif = mousePos - pmousePos;
	renderer.orientation.worldPos += dif; // vec_90_rotate(dif, gridDraw.rotation);
}

void WindowGameplay::mouse_focus_start(const sf::Vector2i& a)
{
}

void WindowGameplay::mouse_focus(const sf::Vector2i& a, const sf::Vector2i& b)

{
	sf::Vector2i p1 = screen_pos_to_tile_pos(a,
		renderer.orientation);
	sf::Vector2i p2 = screen_pos_to_tile_pos(b,
		renderer.orientation);

	IVec treeSize = { 1, 1 };
	if (buildSelected.size())
	{
		UpgradeTree* treePtr = dynamic_cast<UpgradeTree*>(
			this->buildSelected[0]);
		assert(treePtr);
		UpgradeTree& tree = *treePtr;
		treeSize = tree.size;
	}

	auto xRange = Range<>(
		math_min(p1.x, p2.x),
		math_max(p1.x, p2.x),
		treeSize.x);
	auto yRange = Range<>(
		math_min(p1.y, p2.y),
		math_max(p1.y, p2.y),
		treeSize.y);
	// For nicer rendering
	xRange.reverse();
	yRange.reverse();

	auto axis = vec_90_rotate_axis(
		renderer.orientation.rotation);
	if (axis.x == -1)
		yRange.reverse();
	if (axis.y == -1)
		xRange.reverse();
	BuildingBase* mainBuild = nullptr;
	for (auto x : xRange)
	{
		for (auto y : yRange)
		{
			auto pos = sf::Vector2i{ x, y };
			if (!data.chunks->has_tile(pos.x, pos.y))
				continue;

			switch (gameMode)
			{
			case GameMode::BUILD:
				suggestionBuilds.insert(sf::Vector2i{ x, y });
				break;
			case GameMode::DELETE:
			{
				suggestionBuilds.insert(sf::Vector2i{ x, y });
				BuildingBody* body = data.chunks->get_tile(x, y).building;
				if (!body)
					continue;

				for (auto& b : body->base->get_bodies())
				{
					const auto& p = b->tilePos;
					if (!(p1.x <= p.x && p.x <= p2.x &&
						p1.y <= p.y && p.y <= p2.y))
						suggestionBuilds.insert(p);
				}
				break;
			}
			case GameMode::UPGRADE:
			{
				// Get body
				BuildingBody* body = data.chunks->get_tile(
					pos.x, pos.y)
					.building;
				// If there is no body
				if (!body)
				{
					// Empty tile so you can keep it
					suggestionBuilds.insert(pos);
					continue;
				}
				BuildingBase* base = body->base;
				if (base->upgrades.empty())
					continue;
				if (base->alignment != ALIGNMENT_FRIENDLY)
					continue;

				if (mainBuild)
				{
					// Different building, remove
					if (!mainBuild->tree_similar(base))
						continue;
				}
				else
				{
					mainBuild = base;
				}
				for (auto& b : base->get_bodies())
					suggestionBuilds.insert(b->tilePos);
				break;
			}
			break;
			default:
				break;
			}
		}
	}
}

void WindowGameplay::mouse_focus_end(const sf::Vector2i& a, const sf::Vector2i& b)
{
	filter_suggestions();
	grid_change_confirmation(b);
}

void WindowGameplay::on_focus()
{
	data.timerManager.stop_all(
		data.get_time());
}

void WindowGameplay::on_unfocus()
{
	data.timerManager.resume_all(
		data.get_time());
}

std::map<int, std::wstring> WindowGameplay::list_saved_games()
{
	std::map<int, std::wstring> out{};
	// todo remove magic values
	for (const auto& entry : std::filesystem::directory_iterator(".\\saves")) {

		std::wstring widePath = entry.path();
		LOG("%ls", widePath.c_str());
		if (!entry.is_regular_file())
			continue;
		
		std::wstring filename = std::filesystem::path(widePath).stem().c_str();
		
		std::wstring fileName  =L"";
		std::wstring fileNumber = L"";
		bool validFileName = true;
		int step = 0;
		for (size_t i = 0; i < filename.size(); ++i)
		{
			wchar_t c = filename.at(i);
			if (isdigit(c))
			{
				if (i == 0)
				{
					
					validFileName = false;
				}
				if (step == 0)
					step = 1;
				fileNumber += c;
			}
			else
			{
				if (step != 0)
				{
					
					validFileName = false;
				}
				fileName += c;
			}
		}
		
		if (!validFileName)
			continue;
		
		if (fileName == L"" || fileName != L"save")
			continue;
		
		if (fileNumber == L"")
			continue;
		
		int num = (int)wcstol(fileNumber.c_str(), nullptr, 10);
		DEBUG("Listed Saved Game: %d", (int)num);
		if (1 <= num && num <= 16)
		{
			out[num] = filename;
		}
	}

	return out;
}

t_jsonpack WindowGameplay::file_read_jsonpack(const std::wstring& fileName, bool compact)
{
	t_jsonpack out{};

	std::ifstream fileIn;
	std::wstring path = L"saves\\" + fileName + L".json";
	fileIn.open(path);

	nlohmann::json j;

	try
	{
		if (compact)
		{
			//j = j.from_cbor(fileTmp);
			assert(0);
		}
		else
		{
			fileIn.clear();
			fileIn.seekg(0, std::ios::beg);
			j = json::parse(fileIn);
		}
	}
	catch (json::exception& e)
	{
		fileIn.close();
		LOG_ERROR("Can't parse json file \"%s\": %s",
			path.c_str(),
			e.what());
		return t_jsonpack{};
	}

	fileIn.close();

	for (auto& jsonGroup : j.items())
	{
		out[jsonGroup.key()] = jsonGroup.value();
	}

	return out;
}

bool WindowGameplay::file_write_jsonpack(const std::wstring& fileName, const t_jsonpack& jsonPack, bool compact)
{
	nlohmann::json j;
	for (auto& jsonSingle : jsonPack)
	{
		j[jsonSingle.first] = jsonSingle.second;
	}

	
	std::wstring path = L"saves\\" + fileName + L".json";
	std::ofstream fileOut(path);

	if (!fileOut || fileOut.bad())
	{
		
		LOG_ERROR("Could not open file at path \"%s\" for writing. Error: %s",
			path.c_str(),
			std::strerror(SYSERROR()));
		return false;
	}

	if (compact)
	{
		std::vector<std::uint8_t> v = json::to_cbor(j);
		fileOut.write(
			reinterpret_cast<char*>(v.data()),
			v.size());
	}
	else
	{
		fileOut << j.dump(4);
	}
	fileOut.close();

	return true;
}

bool WindowGameplay::file_delete(const std::wstring& fileName)
{
	std::wstring wPath = L"saves\\" + fileName + L".json";
	const wchar_t* cwPath = wPath.c_str();

	std::string cPath;
	size_t size;
	cPath.resize(wPath.length());

#ifdef _WIN32
	wcstombs_s(&size, &cPath[0], cPath.size() + 1, wPath.c_str(), wPath.size());
#else
	wcstombs(&cPath[0], wPath.c_str(), wPath.size());
#endif

	std::remove(cPath.c_str());

	return false;
}

bool WindowGameplay::jsonpack_to_game(const t_jsonpack& jsonPack)
{
	this->data.clean_world();
	data.variantFactory.list_types();

	auto jsonToVariants = [](
		const json& jsonFile,
		SerializeMap& map,
		VariantFactory& variantFactory,
		std::string keyName) {
			if (jsonFile.is_null())
			{
				LOG_ERROR("Json file is null.");
				return false;
			}
			if (!jsonFile.is_array())
			{
				LOG_ERROR("Json file isn't an array");
				return false;
			}
			// Every type of object always is in the same row
			// Meaning a json file that stores SERIALIZABLE_ENEMY will
			// Have only nulls up to this point.
			for (size_t i = 0; i < jsonFile.size(); ++i)
			{
				const json& jsonObjs = jsonFile[i];
				// Skip the nulls
				if (jsonObjs.is_null())
				{
					continue;
				}
				for (size_t j = 0; j < jsonObjs.size(); ++j)
				{
					const json& jsonObj = jsonObjs[j];
					if (jsonObj.is_null())
						continue;

					Variant* t;
					t = variantFactory.create(map, i, j);
					assert(t);
					t->from_json(jsonObj);
				}
			}
			return true;
		};
	
	bool COMPRESSED = false;
	std::vector<json> jsons;
	std::string jsonFiles[] = { "data", "builds", "entities" };
	// Containts all serializable variants
	SerializeMap map;
	map.add(
		SERIALIZABLE_DATA,
		0,
		dynamic_cast<Variant*>(&this->data));

	if (jsonPack.count("other"))
	{
		json jsonOther = jsonPack.at("other");
		renderer.orientation = jsonOther[0];
		const json jsonCons = jsonOther[1];
		map.add(SERIALIZABLE_CONSTRUCTION, 0, &data.constructions);

		data.constructions.from_json(jsonCons);
	}
	
	if (jsonPack.count("chunks"))
	{
		json jsonChunks = jsonPack.at("chunks");
		jsonChunks = jsonChunks[SERIALIZABLE_CHUNKS][0];
		map.add(SERIALIZABLE_CHUNKS, 0, &chunks);
		chunks.from_json(jsonChunks);
	}
	
	for (size_t i = 0; i < (sizeof(jsonFiles) / sizeof(*jsonFiles)); ++i)
	{
		DEBUG("%s", jsonFiles[i].c_str());
		if (!jsonPack.count(jsonFiles[i]))
			continue;
		
		auto jsonFileItr = jsonPack.find(jsonFiles[i]);
		
		json jsonFile = jsonFileItr->second;
		if (jsonFile.is_null())
			continue;
		
		assert(jsonToVariants(jsonFile, map, data.variantFactory, jsonFileItr->first));
	}

 	// Publish
	for (auto& y : map.data)
	{
		for (auto& x1 : y.second)
		{
			auto& x = x1.second;
			x->serialize_publish(map);
		}
	}

	
	// Debugging
	// for (auto &x : map) printf("{%d %d},\t", (int)x->objectType, (int)x->objectId);

	// Initialize
	for (auto& y : map.data)
	{
		for (auto& x1 : y.second)
		{
			auto& x = x1.second;
			{
				x->serialize_initialize(map);
			}
		}
	}
	
	return true;
}

t_jsonpack WindowGameplay::jsonpack_from_game()
{
	t_jsonpack out{};

	const bool COMPRESSED = false;

	json
		jsonBuilds,
		jsonEntities,
		jsonChunks, jsonData,
		jsonOther;
	for (auto& b : data.buildings)
		jsonBuilds[b->objectType][b->objectId] = b->to_json();
	for (auto& b : data.buildingBases)
		jsonBuilds[b->objectType][b->objectId] = b->to_json();
	out["builds"] = jsonBuilds;

	for (auto& b : data.entityCitizens)
		jsonEntities[b->objectType][b->objectId] = b->to_json();

	out["entities"] = jsonEntities;

	jsonChunks[chunks.objectType][chunks.objectId] =
		chunks.to_json();
	out["chunks"] = jsonChunks;

	//Constructions *c = data.constructions;
	//jsonData[c->objectType][c->objectId] = c->to_json();
	out["data"] = jsonData;

	jsonOther[0] = renderer.orientation;
	jsonOther[1] = data.constructions.to_json();
	out["other"] = jsonOther;

	json jsonInfo;
	{
		jsonInfo["citizens"] = data.entityCitizens.size();
		jsonInfo["scenario"] = scenarioName;

		const auto now = std::chrono::system_clock::now();

		time_t curr_time;
		tm* curr_tm;
		char date_string[100];

		std::time(&curr_time);
		curr_tm = localtime(&curr_time);

		strftime(date_string, 50, "%T %B %d, %Y", curr_tm);

		jsonInfo["date"] = date_string;
		jsonInfo["dateInt"] = curr_time;
	}
	out["info"] = jsonInfo;

	return out;
}

void WindowGameplay::popup_confirmation_window(PopupConfirmationData* data)
{
	tgui::ChildWindow::Ptr childWindow = 
		tgui::ChildWindow::create("popupWindow");

	childWindow->setSize({ "30%", "30%" });
	childWindow->setPosition({ "50%", "50%" });
	childWindow->setTitle(data->title);
	gui.add(childWindow);

	tgui::Button::Ptr btnOkay =
		tgui::Button::create("btnPopupOkay");
	btnOkay->setSize({ "25.05%", "16.7%" });
	btnOkay->setPosition({ "5%", "73.3%" });
	btnOkay->setText(data->okStr);
	btnOkay->onClick([](
		PopupConfirmationData* data,
		tgui::ChildWindow::Ptr childWindow)
		{
			data->onConfirmation();
			childWindow->close();
		},
		data, childWindow);
	childWindow->add(btnOkay);

	tgui::Button::Ptr btnCancel =
		tgui::Button::create("btnPopupOkay");
	btnCancel->setSize({ "25.05%", "16.7%" });
	btnCancel->setPosition({ "69.95%", "73.3%" });
	btnCancel->setText(data->cancelStr);
	btnCancel->onClick([](
		PopupConfirmationData* data, 
		tgui::ChildWindow::Ptr childWindow)
		{
			data->onCancel();
			childWindow->close();
		}, 
		data, childWindow);
	childWindow->add(btnCancel);

	tgui::Label::Ptr labelDescription = 
		tgui::Label::create("btnPopupLabel");
	labelDescription->setSize({ "90.2%", "60.29%" });
	labelDescription->setPosition({ "5%", "7.22%" });
	labelDescription->setText(data->description);
	childWindow->add(labelDescription);


	data->postInit();
}

void WindowGameplay::gui_init_resource_tab()
{
	for (auto& x : labelResources)
	{
		ResLabelData& data = x.second;
		data.ptr = FastLabel::create();
		data.ptr->setCharSpriteSheet(&charSpriteSheet);
		data.ptr->renderWindow = window;
		data.ptr->set(get_widget<tgui::Label>(data.title));
		data.ptr->setText("Test");
		gui.add(data.ptr);
	}
}

void WindowGameplay::gui_update_resources_tab(UiResources resources, int value)
{
	ResLabelData& data = labelResources[resources];
	if (data.lastValue != value)
		data.ptr->setText(std::to_string(value));
	data.lastValue = value;
}

bool WindowGameplay::gui_change_group(const std::string& groupName)
{
	for (const tgui::Widget::Ptr& widget : gui.getWidgets())
	{
		assert(widget);
		try {
			std::string str;
			str = widget->getUserData<tgui::String>()
				.toStdString();
			if (str == "GroupMainCore" || str == groupName)
				widget->setVisible(true);
			else
				widget->setVisible(false);
		}
		catch (std::bad_cast&)
		{
			continue;
		}
	}
	return true;
}

bool WindowGameplay::gui_enable_group(const std::string& groupName)
{
	for (const tgui::Widget::Ptr& widget : gui.getWidgets())
	{
		assert(widget);
		try {
			std::string str;
			str = widget->getUserData<tgui::String>()
				.toStdString();
			if (str == groupName)
				widget->setVisible(true);
		}
		catch (std::bad_cast&)
		{
			continue;
		}
	}
	return true;
}

bool WindowGameplay::gui_disable_group(const std::string& groupName)
{
	for (const tgui::Widget::Ptr& widget : gui.getWidgets())
	{
		assert(widget);
		try {
			std::string str;
			str = widget->getUserData<tgui::String>()
				.toStdString();
			if (str == groupName)
				widget->setVisible(false);
		}
		catch (std::bad_cast&)
		{
			continue;
		}
	}
	return true;
}

bool WindowGameplay::gui_update_upgrade_info(UpgradeTree* info, BuildingBase* build)

{
	tgui::ChildWindow::Ptr newWindow;
	newWindow = get_widget<tgui::ChildWindow>("WindowUpgrade");

	std::string name = string_format(build->name, info->name);
	str_replace(name, "_", " ");
	get_widget<tgui::Label>(newWindow, "LabelUpgradeTitle")->setText(
		name
	);
	get_widget<tgui::Label>(newWindow, "LabelUpgradeCost")->setText(
		"Cost: " +
		(info->build.empty() ?
			data.resources_to_str(info->build) :
			"Free")
	);



	// Layout stuff

	tgui::VerticalLayout::Ptr layout;
	layout = get_widget<tgui::VerticalLayout>(newWindow, "LayoutUpgradeInfo");

	layout->removeAllWidgets();

	if (info->spriteHolders.size())
	{
		t_sprite s = info->spriteHolders.at(IVec{ 0, 0 });
		if (s != -1)
		{
			sf::Sprite* sprite;
			auto sprites = renderer.assets->get_sprite(s);
			sprite = sprites->get(0);

			tgui::UIntRect rect;
			sf::FloatRect fr = sprite->getLocalBounds();
			rect = { (unsigned)fr.left, (unsigned)fr.top, (unsigned)fr.width, (unsigned)fr.height };
			tgui::Texture tex = tgui::Texture(*sprite->getTexture(), rect);
			get_widget<tgui::Picture>(
				newWindow,
				"PictureUpgradeWindow")->
				getRenderer()->setTexture(tex);
		}
	}

	if (strcmp(info->description, ""))
	{
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Description: " +
			tgui::String{ info->description });
		layout->add(label);
	}

	if (info->hp != NULL_INT && info->hp > 0)
	{
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"HP: " +
			tgui::String::fromNumber(info->hp));
		layout->add(label);
	}

	if (!info->storageCap.empty())
	{
		// Add how many resources in the building
		tgui::Label::Ptr labelRes = tgui::Label::create();
		labelRes->setText(
			"Storage cap: " +
			data.resources_to_str(info->storageCap));
		layout->add(labelRes);
	}

	if (info->weightCap != NULL_INT)
	{
		// Add how many resources in the building
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Weight cap: " +
			tgui::String::fromNumber(info->weightCap));
		layout->add(label);
	}

	if (info->entityLimit > 0)
	{
		std::string strEntityType = "Workers";
		if (info->props.bool_is(PropertyBool::HOME))
			strEntityType = "Residents";
		// Add how many resources in the building
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			strEntityType +
			" limit: " +
			tgui::String::fromNumber(info->entityLimit));
		layout->add(label);
	}

	if (!info->input.empty())
	{
		// Add how many resources in the building
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Resource requirement: " +
			data.resources_to_str(info->input));
		layout->add(label);
	}

	if (!info->output.empty())
	{
		// Add how many resources in the building
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Resource output: " +
			data.resources_to_str(info->output));
		layout->add(label);
	}

	if (info->powerIn > 0)
	{
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Power input: " +
			tgui::String::fromNumber(info->powerIn));
		layout->add(label);
	}

	if (info->powerOut > 0)
	{
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Power output: " +
			tgui::String::fromNumber(info->powerOut));
		layout->add(label);
	}

	if (info->powerStore > 0)
	{
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Power storage: " +
			tgui::String::fromNumber(info->powerStore));
		layout->add(label);
	}

	return true;
}

bool WindowGameplay::gui_hide_upgrade_info()

{
	tgui::ChildWindow::Ptr window;
	window = get_widget<tgui::ChildWindow>("WindowUpgrade");
	window->setVisible(false);

	this->buildUpgradeSelected = buildUpgradeSelected;

	return true;
}

bool WindowGameplay::gui_show_upgrade_info()
{
	return this->gui_show_upgrade_info(buildUpgradeSelected);
}

bool WindowGameplay::gui_show_upgrade_info(BuildingBase* build)

{
	this->buildUpgradeSelected = build;

	tgui::ChildWindow::Ptr window;
	window = get_widget<tgui::ChildWindow>("WindowUpgrade");

	window->setVisible(true);

	upgradeTree = build->get_upgrades();
	if (upgradeTree.size())
	{
		upgradeTreeItr = upgradeTree.begin();
		gui_update_upgrade_info(*upgradeTreeItr, build);
	}
	else
	{
		tgui::ChildWindow::Ptr newWindow;
		newWindow = get_widget<tgui::ChildWindow>("WindowUpgrade");
		tgui::VerticalLayout::Ptr layout;
		layout = get_widget<tgui::VerticalLayout>(newWindow, "LayoutUpgradeInfo");

		layout->removeAllWidgets();

		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"No upgrades available");
		layout->add(label);

		get_widget<tgui::Label>(newWindow, "LabelUpgradeTitle")->setText(
			build->get_format_name()
		);
	}

	get_widget<tgui::Button>("ButtonUpgradePrev")->onClick(
		[](WindowGameplay* window, BuildingBase* build)
		{
			if (window->upgradeTree.empty())
				return;
			if (window->upgradeTreeItr == window->upgradeTree.begin())
			{
				window->upgradeTreeItr = std::prev(
					window->upgradeTree.end());
			}
			else
				window->upgradeTreeItr =
				std::prev(window->upgradeTreeItr);
			window->gui_update_upgrade_info(
				*window->upgradeTreeItr, build);
		}, this, build);

	get_widget<tgui::Button>("ButtonUpgradeNext")->onClick(
		[](WindowGameplay* window, BuildingBase* build)
		{
			if (window->upgradeTree.empty())
				return;
			++window->upgradeTreeItr;
			if (window->upgradeTreeItr == window->upgradeTree.end())
			{
				window->upgradeTreeItr =
					window->upgradeTree.begin();
			}
			window->gui_update_upgrade_info(
				*window->upgradeTreeItr, build);
		}, this, build);

	return true;
}

bool WindowGameplay::gui_update_constr_build(t_id_config* idConfig, tgui::Texture tex)

{
	UpgradeTree* config = dynamic_cast<UpgradeTree*>(idConfig->at(0));

	const tgui::ChildWindow::Ptr& window = gui.get<tgui::ChildWindow>("WindowConstruction");
	if (!window)
		return false;

	tgui::String strTitle, strCost, strWork, strDesc;
	strTitle = BuildingBase::get_format_str(config->name);
	strCost = "Cost: " + data.resources_to_str(config->build);
	strWork = "Work: " + tgui::String::fromNumber(config->buildWork);
	strDesc = config->description;

	get_widget<tgui::Label>("LabelConstrTitle")->
		setText(strTitle);
	get_widget<tgui::Label>("LabelConstrCost")->
		setText(strCost);
	get_widget<tgui::Label>("LabelConstrWork")->
		setText(strWork);
	get_widget<tgui::Label>("LabelConstrDescr")->
		setText(strDesc);

	get_widget<tgui::Picture>(window, "PictureConstrWindow")->
		getRenderer()->setTexture(tex);

	LOG("%s", config->to_string().c_str());

	get_widget<tgui::Button>(window, "ButtonConstructionConfirm")->
		onClick([](WindowGameplay* gameWindow, t_id_config* config)
			{
				LOG("buildSelected has set");
				gameWindow->buildSelected = *config;
				gameWindow->gameMode = GameMode::BUILD;
				gameWindow->gui_change_group("GroupBuild");
			}, this, idConfig);

	return true;
}

bool WindowGameplay::gui_update_constrs_window()

{
	// For WindowConstructions

	const tgui::ScrollablePanel::Ptr& scroll = gui.get<tgui::ScrollablePanel>("ScrollableConstructions");
	const tgui::HorizontalWrap::Ptr& wrap = gui.get<tgui::HorizontalWrap>("HorizontalConstructions");
	tgui::Panel::Ptr panelClone = tgui::Panel::copy(gui.get<tgui::Panel>("PanelConstruction"));

	if (!scroll || !wrap || !panelClone)
		return false;

	wrap->removeAllWidgets();

	tgui::Vector2f wrapSize = wrap->getSize();
	float xPos = 0;
	float yCount = 0;
	auto& m = data.bodyConfigMap.at(ENUM_BUILDING_TYPE);
	for (auto& x : m)
	{
		UpgradeTree* config = dynamic_cast<UpgradeTree*>(x.second.at(0));
		assert(config);

		tgui::Panel::Ptr newPanel = tgui::Panel::copy(panelClone);
		wrap->add(newPanel);

		tgui::Label::Ptr labelConstruction = newPanel->get<tgui::Label>("LabelConstruction");
		tgui::Picture::Ptr labelImage = newPanel->get<tgui::Picture>("PictureConstruction");
		if (!labelConstruction)
			continue;

		labelConstruction->setText(
			tgui::String(BuildingBase::get_format_str(config->name)));

		tgui::Texture tex;
		if (config->spriteIcon != 0)
		{
			WARNING("todo");
		}
		else if (config->spriteHolders.size())
		{
			t_sprite s = config->spriteHolders.at(IVec{ 0, 0 });
			sf::Sprite* sprite = nullptr;
			if (renderer.assets->get_sprite(s))
			{
				sprite = renderer.assets->get_sprite(s)->get(0);

				tgui::UIntRect rect;
				sf::FloatRect fr = sprite->getLocalBounds();
				rect = { (unsigned)fr.left, (unsigned)fr.top, (unsigned)fr.width, (unsigned)fr.height };
				tex = tgui::Texture(*sprite->getTexture(), rect);
				labelImage->getRenderer()->setTexture(tex);
			}
		}

		xPos += newPanel->getSize().x;
		if (xPos + newPanel->getSize().x > wrapSize.x)
		{
			xPos = 0;
			yCount += newPanel->getSize().y;
		}

		newPanel->onClick([](
			tgui::Gui& gui,
			WindowGameplay* gameWindow,
			tgui::Texture tex,
			t_id_config* config)
			{
				tgui::ChildWindow::Ptr w1, w2;
				w1 = gui.get<tgui::ChildWindow>("WindowConstructions");
				w2 = gui.get<tgui::ChildWindow>("WindowConstruction");
				if (w1 && w2)
				{
					w1->setVisible(false);
					w2->setVisible(true);
				}
				else
					assert(0);
				gameWindow->gui_update_constr_build(config, tex);
			}, std::ref(gui), this, tex, &x.second);
	}
	if (xPos != 0)
		yCount += panelClone->getSize().y;

	wrap->setSize({ wrap->getSize().x, yCount });

	scroll->setContentSize({ 0.f, 0.f });
	scroll->setHorizontalScrollbarPolicy(tgui::Scrollbar::Policy::Never);
	scroll->setVerticalScrollbarPolicy(tgui::Scrollbar::Policy::Always);

	return 1;
}

void WindowGameplay::gui_open_saved_games(bool isSave)
{
	const tgui::ChildWindow::Ptr& windowSaves =
		gui.get<tgui::ChildWindow>("WindowSaves");
	const tgui::HorizontalWrap::Ptr& layoutSaves =
		gui.get<tgui::HorizontalWrap>("LayoutSaves");;

	const tgui::ScrollablePanel::Ptr& scroll =
		gui.get<tgui::ScrollablePanel>("ScrollableSave");

	windowSaves->setVisible(true);

	static tgui::Panel::Ptr panelSave =
		tgui::Panel::copy(windowSaves->get<tgui::Panel>("PanelSave"));

	layoutSaves->removeAllWidgets();

	savedGamesCached = list_saved_games();

	float countY = 0;
	json jsonData{};
	for (int i = 1; i <= 16; ++i)
	{
		auto itrFind = savedGamesCached.find(i);
		bool found = true;
		t_jsonpack jsonPack{};
		if (itrFind == savedGamesCached.end())
		{
			found = false;
		}
		else
		{
			std::wstring str = (*itrFind).second;
			jsonPack = file_read_jsonpack(str, false);
			if (jsonPack.count("other"))
				jsonData = jsonPack.at("other");
		}

		tgui::Label::Ptr labelTitle, labelTime,
			labelScenario, labelPopulation;
		tgui::Button::Ptr buttonLoad, buttonSave,
			buttonDelete;

		tgui::Panel::Ptr newSave = tgui::Panel::copy(panelSave);
		labelTitle =
			newSave->get< tgui::Label>("LabelSaveName");
		std::wstring saveName = std::wstring(L"save" + std::to_wstring(i));
		labelTitle->setText(saveName);

		labelTime =
			newSave->get< tgui::Label>("LabelSaveTime");
		labelScenario =
			newSave->get< tgui::Label>("LabelSaveScenario");
		labelPopulation =
			newSave->get< tgui::Label>("LabelSavePopulation");

		buttonLoad =
			newSave->get< tgui::Button>("ButtonSaveLoad");
		buttonSave =
			newSave->get< tgui::Button>("ButtonSaveSave");
		buttonDelete =
			newSave->get< tgui::Button>("ButtonSaveDelete");

		buttonLoad->onClick(
			[](
				WindowGameplay* window,
				std::wstring fileName,
				PopupConfirmationData* popup)
			{
				/*
				t_jsonpack jsonPack;
				LOG("Reading Json Pack from file");
				jsonPack = window->file_read_jsonpack(fileName, false);
				LOG("Loading Json Pack");
				window->jsonpack_to_game(jsonPack);
				*/

				assert(window);
				assert(popup);

				PopupLoadGame* plg;
				plg = dynamic_cast<PopupLoadGame*>(popup);
				plg->fileName = fileName;
				window->popup_confirmation_window(popup);
			}, 
			this, 
			saveName,
			popupLoadGame);

		buttonSave->onClick(
			[](
				WindowGameplay* window,
				std::wstring fileName,
				PopupConfirmationData* popup,
				t_savedgames* savedGames,
				int saveNum,
				tgui::Gui* gui)
			{
				assert(window);
				assert(popup);
				assert(savedGames);
				if (savedGames->count(saveNum))
				{
					PopupSaveOverride* pso;
					pso = dynamic_cast<PopupSaveOverride*>(popup);
					pso->fileName = fileName;
					guiutils_popup_confirmation_window(*gui, popup);
				}
				else
				{
					t_jsonpack jsonPack;
					LOG("Converting game data into Json Pack");
					jsonPack = window->jsonpack_from_game();
					LOG("Save Json Pack to file");
					window->file_write_jsonpack(fileName, jsonPack, false);

					window->gui_open_saved_games(false);
				}
			},
			this,
			saveName,
			popupSaveOverride,
			& savedGamesCached,
			i,
			&this->gui);
		

		buttonDelete->onClick(
			[](
				WindowGameplay* window,
				std::wstring fileName,
				PopupConfirmationData* popup)
			{
				assert(window);
				assert(popup);

				PopupDeleteSave* pds;
				pds = dynamic_cast<PopupDeleteSave*>(popup);
				pds->fileName = fileName;
				window->popup_confirmation_window(popup);
			}, 
			this, 
			saveName,
			popupDeleteSave);

		assert(labelTitle);
		assert(labelTime);
		assert(labelScenario);
		assert(labelPopulation);
		assert(buttonLoad);

		if (found)
		{
			labelTime->setVisible(true);
			labelScenario->setVisible(true);
			labelPopulation->setVisible(true);

			if (jsonData.count("date"))
				labelTime->setText(jsonData.at("date").get<std::string>());
			if (jsonData.count("scenario"))
				labelScenario->setText(jsonData.at("scenario").get<std::string>());
			if (jsonData.count("population"))
				labelPopulation->setText(jsonData.at("population").get<std::string>());

			buttonLoad->setVisible(true);
			buttonDelete->setVisible(true);
		}
		else
		{
			labelTime->setVisible(false);
			labelScenario->setVisible(false);
			labelPopulation->setVisible(false);
			buttonLoad->setVisible(false);
			buttonDelete->setVisible(false);
		}

		countY += newSave->getSize().y;
		layoutSaves->add(newSave);
	}




	layoutSaves->setSize({ layoutSaves->getSize().x, 16 * 377 });
	assert(scroll);
	if (scroll)
	{
		scroll->setContentSize({ scroll->getSize().x, countY });
		scroll->setHorizontalScrollbarPolicy(tgui::Scrollbar::Policy::Never);
		scroll->setVerticalScrollbarPolicy(tgui::Scrollbar::Policy::Always);
	}
}

void WindowGameplay::filter_suggestions()

{
	for (auto itr = suggestionBuilds.begin();
		itr != suggestionBuilds.end();)
	{
		bool skipDelete;
		if (gameMode == GameMode::BUILD)
		{
			skipDelete =
				data.chunks->get_tile(itr->x, itr->y).building;
		}
		else
		{
			BuildingBody* body =
				data.chunks->get_tile(itr->x, itr->y).building;
			skipDelete = !(body &&
				!body->base->props.bool_is(PropertyBool::UNREMOVABLE));
		}
		if (skipDelete)
			itr = suggestionBuilds.erase(itr);
		else
			++itr;
	}
}

void WindowGameplay::filter_upgrade_buildings(const IVec& latest)
{
	BuildingBody* beginBuild = data.chunks->get_build(
		latest.x,
		latest.y);
	assert(beginBuild);
	for (auto itr = suggestionBuilds.begin();
		itr != suggestionBuilds.end();)
	{
		auto& pos = *itr;
		BuildingBody* b = data.chunks->get_build(pos.x, pos.y);
		assert(b);
		if (!b->base->tree_similar(beginBuild->base))
			itr = suggestionBuilds.erase(itr);
		else
			++itr;
	}
}

bool WindowGameplay::has_build_info(BuildingBase* infoBuild)
{
	return guiBInfoWin.count(infoBuild->id);
}

void WindowGameplay::show_build_info(BuildingBase* infoBuild, bool update)

{
	data.infoBuild = infoBuild;
	tgui::ChildWindow::Ptr newWindow;

	if (!guiBInfoWin.count(infoBuild->id))
	{

		newWindow = tgui::ChildWindow::copy(
			get_widget<tgui::ChildWindow>("WindowBuild"));
		newWindow->setVisible(true);
		newWindow->setResizable(true);
		gui.add(newWindow);

		newWindow->setPosition(
			guiBInfoPtr.size() * 20,
			newWindow->getPosition().y);

		guiBInfoWin.insert({ infoBuild->id, newWindow });
		guiBInfoPtr.insert({ infoBuild->id, infoBuild });

		get_widget<tgui::Button>(newWindow, "ButtonBuildBack")->onClick(
			[](
				WindowGameplay* window,
				BuildingBase* infoBuild,
				tgui::ChildWindow::Ptr childWindow,
				tgui::Gui* gui)
			{
				childWindow->setVisible(false);
				window->guiBInfoPtr.erase(infoBuild->id);
				window->guiBInfoWin.erase(infoBuild->id);
				gui->remove(childWindow);
			}, this, infoBuild, newWindow, &gui);

		get_widget<tgui::Label>(newWindow, "LabelBuildTitle")->setText(
			tgui::String(infoBuild->get_format_name())
		);
	}
	else
	{
		newWindow = guiBInfoWin.at(infoBuild->id);
		if (!update)
		{
			newWindow->setVisible(false);
			guiBInfoPtr.erase(infoBuild->id);
			guiBInfoWin.erase(infoBuild->id);
			gui.remove(newWindow);
			return;
		}
	}


	get_widget<tgui::Label>(newWindow, "LabelBuildHp")->setText(
		tgui::String("HP: ") + tgui::String::fromNumber(infoBuild->hp)
	);

	tgui::VerticalLayout::Ptr layout;
	layout =
		get_widget<tgui::VerticalLayout>(newWindow, "LayoutBuildInfo");

	layout->removeAllWidgets();

	auto makeLine = []()
		{
			tgui::Panel::Ptr panel;
			panel = tgui::Panel::create();

			tgui::SeparatorLine::Ptr lineSep;
			lineSep = tgui::SeparatorLine::create();
			lineSep->setPosition("10%, 50%");
			lineSep->setSize({ "80%", "5" });
			lineSep->setWidgetName("coolLine");

			panel->add(lineSep);
			panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

			return panel;
		};

	auto x = makeLine();
	layout->add(x);

	// Add how much of the building's storage is full
	if (infoBuild->weightCap != NULL_INT || !infoBuild->rStoreCap.empty())
	{
		// Add how many resources in the building
		tgui::Label::Ptr labelRes = tgui::Label::create();
		labelRes->setText(
			"Stored Resources: " +
			data.resources_to_str(infoBuild->rStorage));
		layout->add(labelRes);


		int max = -1, val = 0;
		tgui::String weightVal, weightMax;
		if (infoBuild->weightCap != NULL_INT)
		{
			val = infoBuild->rStorage.weight(&data.resourceWeights);
			max = infoBuild->weightCap;
		}
		else if (!infoBuild->rStoreCap.empty())
		{
			val = infoBuild->rStorage.weight(&data.resourceWeights);
			max = infoBuild->rStoreCap.weight(&data.resourceWeights);
		}

		if (max != -1)
		{
			weightVal = tgui::String::fromNumber(val);
			weightMax = tgui::String::fromNumber(max);


			tgui::HorizontalLayout::Ptr layoutRes;
			layoutRes = tgui::HorizontalLayout::create();

			tgui::Label::Ptr labelResCapPer = tgui::Label::create();
			labelResCapPer->setText(
				"Capacity: " +
				weightVal +
				" / " +
				weightMax);

			tgui::ProgressBar::Ptr pb;
			pb = tgui::ProgressBar::create();
			pb->setMinimum(0);
			pb->setMaximum(max);
			pb->setValue(val);

			layoutRes->add(labelResCapPer);
			layoutRes->add(pb);
			layout->add(layoutRes);

			layout->add(makeLine());
		}
	}

	if (infoBuild->props.bool_is(PropertyBool::WORKPLACE))
	{

		std::string strEntityType = "workers";
		if (infoBuild->props.bool_is(PropertyBool::HOME))
			strEntityType = "residents";
		// Add how many resources in the building
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Entity " +
			strEntityType +
			": " +
			tgui::String::fromNumber(infoBuild->entities.size()));
		layout->add(label);


		tgui::HorizontalLayout::Ptr layoutRes;
		layoutRes = tgui::HorizontalLayout::create();

		tgui::Label::Ptr labelResCapPer = tgui::Label::create();
		labelResCapPer->setText(
			tgui::String::fromNumber(infoBuild->entities.size()) +
			" / " +
			tgui::String::fromNumber(infoBuild->entityLimit));

		tgui::ProgressBar::Ptr pb;
		pb = tgui::ProgressBar::create();
		pb->setMinimum(0);
		pb->setMaximum(infoBuild->entityLimit);
		pb->setValue((unsigned)infoBuild->entities.size());

		layoutRes->add(labelResCapPer);
		layoutRes->add(pb);
		layout->add(layoutRes);

		layout->add(makeLine());
	}

	if (!infoBuild->rIn.empty())
	{
		// Add how many resources in the building
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Resource requirement: " +
			data.resources_to_str(infoBuild->rIn));
		layout->add(label);

		layout->add(makeLine());
	}

	if (!infoBuild->rOut.empty())
	{
		// Add how many resources in the building
		tgui::Label::Ptr label = tgui::Label::create();
		label->setText(
			"Resource output: " +
			data.resources_to_str(infoBuild->rOut));
		layout->add(label);

		layout->add(makeLine());
	}

	if (infoBuild->network)
	{
		if (infoBuild->powerIn > 0)
		{
			tgui::Label::Ptr label = tgui::Label::create();
			label->setText(
				"Power input: " +
				tgui::String::fromNumber(infoBuild->powerIn));
			layout->add(label);
		}

		if (infoBuild->powerOut > 0)
		{
			tgui::Label::Ptr label = tgui::Label::create();
			label->setText(
				"Power output: " +
				tgui::String::fromNumber(infoBuild->powerOut));
			layout->add(label);
		}

		if (infoBuild->powerStore > 0)
		{
			tgui::HorizontalLayout::Ptr layoutRes;
			layoutRes = tgui::HorizontalLayout::create();

			tgui::Label::Ptr label = tgui::Label::create();
			label->setText(
				"Power stored: " +
				tgui::String::fromNumber(infoBuild->powerValue) +
				" / " +
				tgui::String::fromNumber(infoBuild->powerStore));
			layout->add(label);

			tgui::ProgressBar::Ptr pb;
			pb = tgui::ProgressBar::create();
			pb->setMinimum(0);
			pb->setMaximum(infoBuild->powerStore);
			pb->setValue(infoBuild->powerValue);

			layoutRes->add(label);
			layoutRes->add(pb);
			layout->add(layoutRes);
		}

		layout->add(makeLine());
	}

	// todo
	auto str = infoBuild->to_string();
	if (infoBuild->buildType ==
		(t_id)BuildingType::CONSTRUCTION)
	{
		assert(0);

		str = "In Construction:";
		ConstructionData* construct = nullptr;
		try
		{
			construct = &data.constructions.at(infoBuild);
		}
		catch (std::out_of_range& e)
		{
			LOG_ERROR("Invalid construction building: %s", e.what());
		}

		if (construct)
		{
			int wn = construct->work_needed();
			str += std::to_string(wn - construct->work_left()) +
				" : " +
				std::to_string(wn);
			str += "\n";
			str += str_unfold(
				construct->queueData.step->to_string(&data, false));
		}
		else
		{
			str += "\nMissing construction data";
		}
	}
}

bool WindowGameplay::has_entity_info(EntityBody* entity)
{
	return guiEInfoWin.count(entity->objectId);
}

void WindowGameplay::show_entity_info(EntityBody* entity, bool update)

{
	data.infoEntity = entity;
	tgui::ChildWindow::Ptr newWindow;

	if (!guiEInfoWin.count(entity->objectId))
	{
		newWindow = tgui::ChildWindow::copy(
			get_widget<tgui::ChildWindow>("WindowEntity"));
		newWindow->setVisible(true);
		newWindow->setResizable(true);
		gui.add(newWindow);

		newWindow->setPosition(
			guiBInfoPtr.size() * 20,
			newWindow->getPosition().y);

		guiEInfoWin.insert({ entity->objectId, newWindow });
		guiEInfoPtr.insert({ entity->objectId, entity });

		get_widget<tgui::Button>(newWindow, "ButtonEntityClose")->onClick(
			[](
				WindowGameplay* window,
				EntityBody* entity,
				tgui::ChildWindow::Ptr childWindow,
				tgui::Gui* gui)
			{
				childWindow->setVisible(false);
				window->guiEInfoPtr.erase(entity->objectId);
				window->guiEInfoWin.erase(entity->objectId);
				gui->remove(childWindow);
			}, this, entity, newWindow, &gui);
	}
	else
	{
		newWindow = guiEInfoWin.at(entity->objectId);
		if (!update)
		{
			newWindow->setVisible(false);
			gui.remove(newWindow);
			guiEInfoPtr.erase(entity->objectId);
			guiEInfoWin.erase(entity->objectId);
			return;
		}
	}

	auto makeLine = []()
		{
			tgui::Panel::Ptr panel;
			panel = tgui::Panel::create();

			tgui::SeparatorLine::Ptr lineSep;
			lineSep = tgui::SeparatorLine::create();
			lineSep->setPosition("10%, 50%");
			lineSep->setSize({ "80%", "5" });
			lineSep->setWidgetName("coolLine");

			panel->add(lineSep);
			panel->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

			return panel;
		};

	if (entity->entityType == EntityType::CITIZEN)
	{
		EntityCitizen* citizen =
			dynamic_cast<EntityCitizen*>(entity);

		std::string jobName;
		jobName = str_lowercase(data.enum_get_str(
			ENUM_CITIZEN_JOB,
			(t_id)citizen->job,
			true));
		jobName += " - " + std::to_string((int)citizen->objectId);

		get_widget<tgui::Label>(newWindow, "LabelEntityJob")->setText(
			tgui::String(jobName)
		);

		tgui::VerticalLayout::Ptr layout;
		layout =
			get_widget<tgui::VerticalLayout>(newWindow, "LayoutEntity");

		layout->removeAllWidgets();

		auto x = makeLine();
		layout->add(x);

		{
			std::string strAction = EntityBodyStr::ACTION_STRS[(size_t)citizen->action];
			if (strAction.find("#destination") != std::string::npos &&
				citizen->pathData.valid())
			{
				BuildingBase* buildDest =
					citizen->pathData.destination->get_building();
				std::string destStr = "";
				if (buildDest)
				{
					destStr += buildDest->get_format_name();
					destStr += " at position ";
					destStr += vec_str(buildDest->tilePos);
					// todo remove magic string
					str_replace(strAction, "#destination", destStr);
				}
				else
				{
					destStr += "position ";
					destStr += vec_str(citizen->pathData.destPos);
					// todo remove magic string
					str_replace(strAction, "#destination", destStr);
				}
				;

			}
			tgui::Label::Ptr labelAction = tgui::Label::create();
			labelAction->setText(
				strAction);
			layout->add(labelAction);
		}

		layout->add(makeLine());

		{
			str_lowercase(data.enum_get_str(
				ENUM_CITIZEN_JOB,
				(t_id)citizen->job,
				true));
		}


		// Add how much of the building's storage is full
		if (citizen->inventorySize != NULL_INT || !citizen->rInventoryCap.empty())
		{
			// Add how many resources in the building
			tgui::Label::Ptr labelRes = tgui::Label::create();
			labelRes->setText(
				"Stored Resources: " +
				data.resources_to_str(citizen->rInventory));
			layout->add(labelRes);


			int max = -1, val = 0;
			tgui::String weightVal, weightMax;
			if (citizen->inventorySize != NULL_INT)
			{
				val = citizen->rInventory.weight(&data.resourceWeights);
				max = citizen->inventorySize;
			}
			else if (!citizen->rInventoryCap.empty())
			{
				val = citizen->rInventory.weight(&data.resourceWeights);
				max = citizen->rInventoryCap.weight(&data.resourceWeights);
			}

			if (max != -1)
			{
				weightVal = tgui::String::fromNumber(val);
				weightMax = tgui::String::fromNumber(max);


				tgui::HorizontalLayout::Ptr layoutRes;
				layoutRes = tgui::HorizontalLayout::create();

				tgui::Label::Ptr labelResCapPer = tgui::Label::create();
				labelResCapPer->setText(
					"Capacity: " +
					weightVal +
					" / " +
					weightMax);

				tgui::ProgressBar::Ptr pb;
				pb = tgui::ProgressBar::create();
				pb->setMinimum(0);
				pb->setMaximum(max);
				pb->setValue(val);

				layoutRes->add(labelResCapPer);
				layoutRes->add(pb);
				layout->add(layoutRes);

				layout->add(makeLine());
			}
		}


	}
}

void WindowGameplay::grid_change_confirmation(IVec latest)

{
	if (suggestionBuilds.empty())
	{
		gui_disable_group("GroupGridBuild");
		return;
	}
	// todo
	// guiButtons["grid cost"].active = 1;

	gui_enable_group("GroupGridBuild");

	switch (gameMode)
	{
	case GameMode::BUILD:
	{
		if (buildSelected.empty())
			break;
		UpgradeTree* tree;
		tree = dynamic_cast<UpgradeTree*>(
			buildSelected[0]);
		assert(tree);
		Resources costRes = tree->build;
		costRes = costRes * suggestionBuilds.size();
		std::string costStr =
			(costRes.empty()
				? "Free"
				: costRes.to_string(&data.resourceWeights));;
	}
	break;

	case GameMode::UPGRADE:
	{
		if (data.chunks->has_build(latest.x, latest.y))
			filter_upgrade_buildings(latest);

		tgui::ChildWindow::Ptr newWindow;
		newWindow = get_widget<tgui::ChildWindow>("WindowUpgrade");
		if (suggestionBuilds.empty())
			gui_hide_upgrade_info();
		else
			gui_show_upgrade_info();
	}
	break;
	default:
		break;
	}
}

void WindowGameplay::update_upgrade_info()

{
	assert(0);

	BuildingBase* base = suggestion_build();
	if (!base)
		return;

	std::string str =
		"\t" +
		base->get_format_name() +
		"\n";

	auto ul = base->get_upgrades();
	if (ul.empty())
	{
		str += "No upgrades found";
		(
			str);
		return;
	}

	suggestionUpgrade = math_min(
		suggestionUpgrade,
		(int)ul.size() - 1);
	suggestionUpgrade = math_max(
		suggestionUpgrade,
		0);

	auto& u = ul.at(suggestionUpgrade);
	auto str2 = u->to_string(&data, false);
	str2 = str_unfold(str2);
	str +=
		"\tCost: " +
		(u->build * suggestionBuilds.size()).to_string() +
		"\n" +
		str2;
	(
		str);
}

void WindowGameplay::upgrade_building_get_frames(BuildingBase* build, UpgradeTree* step, IVec& frame0, IVec& frame1)

{
	std::vector<UpgradeTree*> steps =
		recursive_get_last_layer_tree(
			build->tree,
			build->upgrades);

	frame0 = build->tree->framesStart;
	frame1 = build->tree->framesEnd;
	DEBUG("%s %s",
		VEC_CSTR(frame0), VEC_CSTR(frame1));

	for (UpgradeTree* upgrade : steps)
	{
		IVec frame2 = upgrade->framesStart;
		IVec frame3 = upgrade->framesEnd;
		frame0 = {
			frame2.x == -1 ? frame0.x : frame2.x,
			frame2.y == -1 ? frame0.y : frame2.y };
		frame1 = {
			frame3.x == -1 ? frame1.x : frame3.x,
			frame3.y == -1 ? frame1.y : frame3.y };
		DEBUG("%s %s",
			VEC_CSTR(frame2), VEC_CSTR(frame3));
	}
}

void WindowGameplay::upgrade_building(BuildingBase* build, UpgradeTree* step)

{
	assert(build);
	assert(step);

	// Upgrade the building
	build->load_upgrade_step(step);

	// Get the upgrade tree's leafs 
	std::vector<UpgradeTree*> steps =
		recursive_get_last_layer_tree(
			build->tree,
			build->upgrades);

	IVec start, end;
	upgrade_building_get_frames(build, step, start, end);

	auto& info = *build->tree;

	IVec frameCount;
	frameCount = assets.get_texture_from_name(
		info.image)
		.frameCount;
	int mainSprite = assets.load_sprites(
		info.image,
		start,
		end);

	DEBUG("%s %d %s %s",
		info.image,
		(int)mainSprite,
		VEC_CSTR(start),
		VEC_CSTR(end));

	if (mainSprite == -1)
		WARNING("Error on loading image: \"%s\". Args: %s %s",
			info.image,
			VEC_CSTR(start),
			VEC_CSTR(end));

	build->sprites[sf::Vector2i{ 0, 0 }] = mainSprite;
	// build->update_sprites(renderer.orientation.rotation);

	// todo
	WARNING("upgrade texture unimplemented");
}

void WindowGameplay::mouse_zoom(const sf::Vector2i& iCenterPos, int dist)
{

	if (gui.getWidgetAtPosition({ 
		(float)iCenterPos.x, 
		(float)iCenterPos.y }).get())
		return;

	auto& orientation = renderer.orientation;
	auto& zoomFactor = orientation.zoomFactor;
	auto& worldPos = orientation.worldPos;

	float prevS, currS;
	// The location of the mouse on screen
	sf::Vector2f centerPos = (sf::Vector2f)vec_90_rotate(iCenterPos, 0 * orientation.rotation);

	prevS = powf(2.0f, zoomFactor / 256.f);
	zoomFactor += dist;
	currS = powf(2.0f, zoomFactor / 256.f);
	orientation.scale = { currS, currS };

	sf::Vector2i nextWorldPos =
		(sf::Vector2i)(currS / prevS * ((sf::Vector2f)worldPos - centerPos) + centerPos);

	worldPos = nextWorldPos;
}

void WindowGameplay::focus_on(const sf::Vector2f& fpos)
{
	auto& orientation = renderer.orientation;
	orientation.worldPos = (sf::Vector2i)view->getCenter();
	sf::Vector2f center = screen_pos_to_world_pos(
		sf::Vector2i{ 0, 0 },
		orientation);

	orientation.worldPos = { 0, 0 };
	orientation.worldPos = -world_pos_to_screen_pos<float>(
		fpos + center,
		orientation);
}

void WindowGameplay::rotate_world(int dif)
{
	auto& orientation = renderer.orientation;
	dif = math_mod(dif, 4);

	sf::Vector2f center = screen_pos_to_world_pos(
		(sf::Vector2i)view->getCenter(),
		orientation);
	orientation.worldPos = { 0, 0 };
	orientation.rotation = math_mod(orientation.rotation + dif, 4);

	auto pos = world_pos_to_screen_pos(sf::Vector2f{ 10.f, 10.f }, orientation);
	DEBUG("%s", VEC_CSTR(pos));

	focus_on(center);
}