#include "window/WindowMenu.hpp"


#include <filesystem>

#include "window/WindowManager.hpp"
#include "utils/class/logger.hpp"

struct PopupTest : PopupConfirmationData
{
	PopupTest(
		std::string title = "asd",
		std::string description = "dsa") :
		PopupConfirmationData(title, description)
	{
	}

	void onConfirmation() override
	{
	}

	void postInit() override
	{
		DEBUG("%s", __FUNCTION__);
	}
};

WindowMenu::WindowMenu()
{
}

WindowMenu::~WindowMenu()
{
	delete popupTest;
}

bool WindowMenu::init_window(sf::RenderWindow* window, sf::View* view)
{
	GameWindow::init_window(window, view);
	gui.setTarget(*window);
	return true;
}

bool WindowMenu::init()
{
	popupTest = new PopupTest;


	std::string guiFormPath =
		std::filesystem::current_path().string() + "\\assets\\gui\\FormMenu.txt";
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

	get_widget<tgui::Button>("ButtonSaveFiles")->onPress([](
		WindowMenu* gameWindow,
		tgui::Gui& gui) {
			guiutils_save_files(gui);
		},
		this,
		 std::ref(gui));

	get_widget<tgui::Button>("ButtonSettings")->onPress([](
		WindowMenu* gameWindow,
		tgui::Gui& gui,
		WindowManager* manager) {
			guiutils_settings(
				manager,
				gui);
		}, 
		this,
		std::ref(gui),
		std::ref(this->managerParent));

	get_widget<tgui::Button>("ButtonAbout")->onPress([](WindowMenu* gameWindow) {
		LOG("About window to be implemented");
		}, this);

	get_widget<tgui::Button>("ButtonExitGame")->onPress([](WindowMenu* gameWindow) {
		if (gameWindow->window)
			gameWindow->window->close();
		}, this);
		
	return true;
}

void WindowMenu::close()
{
}

void WindowMenu::update(float delta)
{
}

void WindowMenu::render()
{
	gui.draw();
}

void WindowMenu::mouse_released(const sf::Vector2i& mousePos)
{

	if (0)
	{
	PopupTest* pso;
	pso = dynamic_cast<PopupTest*>(popupTest);
	guiutils_popup_confirmation_window(gui, pso);

	managerParent->change_window(
		managerParent->get_windows().front()
		);
	}
}
