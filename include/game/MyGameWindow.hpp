#pragma once

#ifdef _WIN32
#include <windows.h>
#undef ERROR
#undef WARNING
#undef DELETE
#undef FALSE
#undef TRUE
#undef min
#undef max
#undef IN
#undef OUT
#define SYSERROR()  GetLastError()
#else
#include <errno.h>
#define SYSERROR()  errno
#endif

#include <fstream>
#include <regex> // std::regex
#include <set>
#include <filesystem>

#include <SFML/Graphics.hpp>

#include <TGUI/TGUI.hpp>

#include <type_traits>
#include <chrono>

#include "utils/math.hpp"
#include "file/json_assets.hpp"

#include "utils/class/logger.hpp"
#include "utils/math.hpp"
#include "utils/utils.hpp"

#include "file/asset_manager.hpp"
#include "render/graphics.hpp"
#include "render/gui.hpp"
#include "render/FastLabel.hpp"

#include "window/GameWindow.hpp"
#include "window/gui_utils.hpp"

extern "C"
{
#include "../libs/prng.h"
}


static const std::string OUT_PATH =
"D:\\Projects\\VisualStudio\\Projects\\IsometricGame\\IsometricGame\\out\\";



std::vector<UpgradeTree*>
recursive_get_last_layer_tree(
	UpgradeTree* step,
	const std::vector<int>& upgrades);


// Defined in json_assets.cpp
Resources string_to_cost(const std::string& str, t_weights* weights);

void printBuildQuadTree(ChunkQuadTree<Tile*>::Node* tree);

typedef std::unordered_map<std::string, json> t_jsonpack;
typedef std::map<int, std::wstring> t_savedgames;

struct WindowGameplay : GameWindow
{

	enum class UiResources
	{
		ELECTRICITY,
		PEOPLE,
		ORE,
		GEM,
		BODIES,
		COUNT
	};

	struct ResLabelData
	{

		ResLabelData(std::string title = "") :
			title(title)
		{

		}

		std::string title;
		int lastValue = -1;
		FastLabel::Ptr ptr = nullptr;
	};

	// Readonly
	AssetManager assets = AssetManager("assets");

	Chunks chunks = Chunks(CHUNK_W, CHUNK_H);
	GameData data = GameData(&chunks);
	RendererClass renderer;
	GameMode gameMode = GameMode::VIEW;

	GameTimerManager<t_seconds> timerManager;

	PopupConfirmationData* popupSaveOverride = nullptr;
	// todo
	PopupConfirmationData* popupLoadGame = nullptr;
	PopupConfirmationData* popupDeleteSave = nullptr;
	t_savedgames savedGamesCached;

	t_configmap::t_inner::iterator currentTree;
	// The building that is currenlty selected
	t_id_config buildSelected{};
	BuildingBase* buildUpgradeSelected = nullptr;

	std::set<sf::Vector2i, IVecCompare> suggestionBuilds;

	int suggestionUpgrade = 0;
	bool godMode = 1;
	bool enableSaving = false;
	bool enableConstruction = 0;
	bool enableDraw = false;

	bool searchEntity = false;
	FVec searchEntityPos;

	// Rendering
	bool zooming = false;
	bool isBtnPressed = 0;

	sf::Font font;
	sf::Text text;
	CharSpriteSheet charSpriteSheet;

	std::unordered_map<size_t, BuildingBase*> guiBInfoPtr;
	std::unordered_map<size_t, tgui::ChildWindow::Ptr> guiBInfoWin;

	std::unordered_map<size_t, EntityBody*> guiEInfoPtr;
	std::unordered_map<size_t, tgui::ChildWindow::Ptr> guiEInfoWin;

	// todo serialize
	std::string scenarioName = "Unknown";


	sf::Vector2i lastPressedTile;
	// "drawInsert" checks if the user wants to add
	bool drawInsert = true;

	std::unordered_map< UiResources, ResLabelData> labelResources = {
		std::pair<UiResources, ResLabelData>{UiResources::ELECTRICITY, {"LabelResElctr"}},
		std::pair<UiResources, ResLabelData>{UiResources::PEOPLE, {"LabelResPpl"}},
		std::pair<UiResources, ResLabelData>{UiResources::ORE, {"LabelResOre"}},
		std::pair<UiResources, ResLabelData>{UiResources::GEM, {"LabelResGem"}},
		std::pair<UiResources, ResLabelData>{UiResources::BODIES, {"LabelResBodies"}}
	};

	std::vector<UpgradeTree*> upgradeTree;
	std::vector<UpgradeTree*>::const_iterator upgradeTreeItr;




	inline BuildingBase* suggestion_build()
	{
		if (suggestionBuilds.empty())
			return nullptr;
		auto& pos = *suggestionBuilds.begin();
		BuildingBase* base = nullptr;
		base = data.chunks->get_build(pos.x, pos.y)->base;
		return base;
	}

	WindowGameplay();

	~WindowGameplay();

	bool
		init_window(sf::RenderWindow* window, sf::View* view) override;

	bool
		load() override;

	bool
		init() override;

	void close() override;

	void update(float delta) override;

	void
		render() override;

	void
		mouse_start(const sf::Vector2i& mousePos) override;

	void mouse_dragged(
			const sf::Vector2i& mousePos, 
		const sf::Vector2i& pmousePos) override;

	void
		mouse_released(const sf::Vector2i& mousePos) override;

	void mouse_pressed(const sf::Vector2i& mousePos) override;

	void
		mouse_zoom_dragged(
			const sf::Vector2i& mousePos, 
			const sf::Vector2i& pmousePos) override;

	void mouse_focus_start(const sf::Vector2i& a) override;

	void mouse_focus(const sf::Vector2i& a, const sf::Vector2i& b) override;

	void mouse_focus_end(
		const sf::Vector2i& a, 
		const sf::Vector2i& b) override;

	void on_focus() override;

	void on_unfocus() override;

	void mouse_zoom(
		const sf::Vector2i& iCenterPos, 
		int dist) override;

	void focus_on(const sf::Vector2f& fpos);

	void rotate_world(int dif);

	// Load/Save

	t_savedgames list_saved_games();

	t_jsonpack file_read_jsonpack(const std::wstring& fileName, bool compact);

	bool file_write_jsonpack(
		const std::wstring& fileName, 
		const t_jsonpack&, 
		bool compact);

	bool file_delete(const std::wstring& fileName);

	bool jsonpack_to_game(const t_jsonpack&);

	t_jsonpack jsonpack_from_game();

	// Gui

	void popup_confirmation_window(PopupConfirmationData*);

	void gui_init_resource_tab();

	void gui_update_resources_tab(UiResources resources, int value);

	bool gui_change_group(const std::string& groupName);

	bool gui_enable_group(const std::string& groupName);

	bool gui_disable_group(const std::string& groupName);

	bool gui_update_upgrade_info(
		UpgradeTree* info,
		BuildingBase* build);

	bool gui_hide_upgrade_info();

	bool gui_show_upgrade_info();

	bool gui_show_upgrade_info(BuildingBase* build);

	bool gui_update_constr_build(t_id_config* idConfig, tgui::Texture tex);

	bool gui_update_constrs_window();

	void gui_open_saved_games(bool isSave);

	void filter_suggestions();

	void filter_upgrade_buildings(const IVec& latest);

	bool has_build_info(BuildingBase* infoBuild);

	void show_build_info(BuildingBase* infoBuild, bool update = false);

	bool has_entity_info(EntityBody* entity);

	void show_entity_info(EntityBody* entity, bool update = false);

	void grid_change_confirmation(IVec latest);

	void update_upgrade_info();



	// Get the new frames of the upgraded building
	void upgrade_building_get_frames(
		BuildingBase* build,
		UpgradeTree* step,
		IVec& frame0, IVec& frame1);

	// Wrapper for BuildingBody::upgrade() to apply sprite assets
	void upgrade_building(
		BuildingBase* build,
		UpgradeTree* step);
};