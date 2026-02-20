#include "window/gui_utils.hpp"


#include <filesystem>
#include <memory>

#include "utils/class/logger.hpp"
#include "window/GameWindow.hpp"
#include "window/WindowManager.hpp"

#define QUICK_FIND(container,strId,TYPE) \
(std::find_if(container.begin(), container.end(), [strId](const TYPE& a) \
{ \
	return a.title == strId; \
}))

void guiutils_popup_confirmation_window(tgui::Gui& gui, PopupConfirmationData* data)
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

static bool guiutils_offload_settings(
	tgui::ChildWindow::Ptr& gui,
	GameSettings& settings,
	const SettingsDefaults& defaults)
{
	tgui::Slider::Ptr sliderUIVolume, sliderMusicVolume, sliderFxVolume;
	sliderUIVolume = gui->get<tgui::Slider>("SliderUIVolume");
	if (sliderUIVolume != nullptr)
		settings.volumeUI = (int)sliderUIVolume->getValue();
	else
		return false;
	sliderMusicVolume = gui->get<tgui::Slider>("SliderMusicVolume");
	if (sliderMusicVolume != nullptr)
		settings.volumeMusic = (int)sliderMusicVolume->getValue();
	else
		return false;
	sliderFxVolume = gui->get<tgui::Slider>("SliderFxVolume");
	if (sliderFxVolume != nullptr)
		settings.volumeGame = (int)sliderFxVolume->getValue();
	else
		return false;

	tgui::ComboBox::Ptr
		comboGraphicsQuality, comboResolution, comboSaving;

	
	comboGraphicsQuality = gui->get<tgui::ComboBox>("ComboGraphicsQuality");
	if (comboGraphicsQuality != nullptr)
	{
		std::string quality = comboGraphicsQuality->
			getSelectedItemId().toStdString();
		auto itr = QUICK_FIND(defaults.qualities, quality, SettingsQuality);
		if (itr != defaults.qualities.end())
			settings.quality = *itr;
	}
	else
		return false;
	comboResolution = gui->get<tgui::ComboBox>("ComboResolution");
	if (comboResolution != nullptr)
	{
		std::string resolution = comboResolution->
			getSelectedItemId().toStdString();
		auto itr = QUICK_FIND(defaults.resolutions, resolution, SettingsResolution);
		if (itr != defaults.resolutions.end())
			settings.resolution = *itr;
	}
	else
		return false;
	comboSaving = gui->get<tgui::ComboBox>("ComboSaving");
	if (comboSaving != nullptr)
	{
		std::string interval = comboSaving->
			getSelectedItemId().toStdString();
		auto itr = QUICK_FIND(defaults.intervals, interval, SettingsAutosaveInterval);
		if (itr != defaults.intervals.end())
			settings.interval = *itr;
	}
	else
		return false;

	tgui::CheckBox::Ptr checkFullscreen, checkVSync;
	checkFullscreen = gui->get<tgui::CheckBox>("CheckFullscreen");
	if (checkFullscreen != nullptr)
		settings.isFullscreen = checkFullscreen->isChecked();
	else
		return false;

	checkVSync = gui->get<tgui::CheckBox>("CheckVSync");
	if (checkVSync != nullptr)
		settings.enableVSync = checkVSync->isChecked();
	else
		return false;

	return true;
}

static bool guiutils_populate_settings(
	tgui::ChildWindow::Ptr& gui,
	const GameSettings& settings)
{
	tgui::Slider::Ptr sliderUIVolume, sliderMusicVolume, sliderFxVolume;
	sliderUIVolume = gui->get<tgui::Slider>("SliderUIVolume");
	if (sliderUIVolume != nullptr)
		sliderUIVolume->setValue((float)settings.volumeUI);
	sliderMusicVolume = gui->get<tgui::Slider>("SliderMusicVolume");
	if(sliderMusicVolume != nullptr)
		sliderMusicVolume->setValue((float)settings.volumeMusic);
	sliderFxVolume = gui->get<tgui::Slider>("SliderFxVolume");
	if (sliderFxVolume != nullptr)
		sliderFxVolume->setValue((float)settings.volumeGame);

	tgui::ComboBox::Ptr
		comboGraphicsQuality, comboResolution, comboSaving;

	comboGraphicsQuality = gui->get<tgui::ComboBox>("ComboGraphicsQuality");
	if (comboGraphicsQuality != nullptr)
		comboGraphicsQuality->setSelectedItemById(settings.quality.title);
	comboResolution = gui->get<tgui::ComboBox>("ComboResolution");
	if (comboResolution != nullptr)
		comboResolution->setSelectedItemById(settings.resolution.title);
	comboSaving = gui->get<tgui::ComboBox>("ComboSaving");
	if (comboSaving != nullptr)
		comboSaving->setSelectedItemById(settings.interval.title);

	tgui::CheckBox::Ptr checkFullscreen, checkVSync;
	checkFullscreen = gui->get<tgui::CheckBox>("CheckFullscreen");
	if (checkFullscreen != nullptr)
		checkFullscreen->setChecked(settings.isFullscreen);

	checkVSync = gui->get<tgui::CheckBox>("CheckVSync");
	if (checkVSync != nullptr)
		checkVSync->setChecked(settings.enableVSync);

	return true;
}

bool guiutils_settings(
	WindowManager* manager,
	tgui::Gui& gui)
{

	tgui::Gui guiTmp;

	std::string guiFormPath =
		std::filesystem::current_path().string() + "\\assets\\gui\\FormSettings.txt";
	try
	{
		guiTmp.loadWidgetsFromFile(guiFormPath);
	}
	catch (std::exception const& e)
	{
		LOG_ERROR(
			"Unable to call loadWidgetsFromFile. In %s."
			"Error: %s",
			FUNC_NAME,
			e.what());
		return false;
	}

	// If a window already exists, don't change it
	tgui::ChildWindow::Ptr w;
	if ((w = gui.get<tgui::ChildWindow>("WindowSettings")).get())
	{
		w->setVisible(!w->isVisible());
		w->moveToFront();
		guiutils_populate_settings(w, manager->get_settings());
		return true;
	}

	tgui::ChildWindow::Ptr windowSettings, windowNew;
	windowSettings =
		guiTmp.get<tgui::ChildWindow>("WindowSettings");
	windowNew = tgui::ChildWindow::create("WindowSettings");

	windowNew->setResizable(windowSettings->isResizable());

	DEBUG("open settings");
	for (tgui::Widget::Ptr ptr : windowSettings->getWidgets())
	{
		//tgui::Layout2d l2d = ptr->getSizeLayout();
		//DEBUG("%f %f", l2d.toString().c_str());
		//ptr->setSize(l2d);
		//ptr->setParent(nullptr);
		windowNew->add(ptr->clone(), ptr->getWidgetName());
		//windowNew->get<tgui::Widget>(ptr->getWidgetName())->setSize(l2d);
	}

	// Populate variations
	{
		const SettingsDefaults& defaults =
			manager->get_default_settings();

		tgui::ComboBox::Ptr
			comboGraphicsQuality,
			comboResolution,
			comboSaving;
		comboGraphicsQuality =
			windowNew->get<tgui::ComboBox>("ComboGraphicsQuality");
		comboResolution =
			windowNew->get<tgui::ComboBox>("ComboResolution");
		comboSaving =
			windowNew->get<tgui::ComboBox>("ComboSaving");

		for (auto& x : defaults.resolutions)
		{
			comboResolution->addItem(x.item_name(), x.title);
		}

		for (auto& x : defaults.qualities)
			comboGraphicsQuality->addItem(x.title, x.title);

		for (auto& x : defaults.intervals)
			comboSaving->addItem(x.title, x.title);
	}

	windowNew->get<tgui::Button>("ButtonCancel")->onClick(
		[](
			tgui::ChildWindow::Ptr window)
		{
			window->close();
		},
		windowNew);

	windowNew->get<tgui::Button>("ButtonApply")->onClick(
		[](
			tgui::Gui& gui,
			tgui::ChildWindow::Ptr window,
			WindowManager* manager)
		{
			GameSettings settings = manager->get_settings();
			assert(guiutils_offload_settings(
				window, 
				settings,
				manager->get_default_settings()));

			
			
			manager->set_settings_data(settings);
			manager->apply_settings();
		},
		std::ref(gui),
		windowNew,
		manager);

	windowNew->get<tgui::Button>("ButtonConfirm")->onClick(
		[](
			tgui::ChildWindow::Ptr window,
			WindowManager* manager)
		{
			GameSettings settings = manager->get_settings();
			assert(guiutils_offload_settings(
				window,
				settings,
				manager->get_default_settings()));

			manager->set_settings_data(settings);
			manager->apply_settings();

			window->close();
		},
		windowNew,
		manager);

	guiutils_populate_settings(windowNew, manager->get_settings());

	gui.add(windowNew, "WindowSettings");

	return true;
}

bool guiutils_save_files(tgui::Gui& gui)
{
	tgui::Gui guiTmp;

	std::string guiFormPath =
		std::filesystem::current_path().string() + "\\assets\\gui\\FormFiles.txt";
	try
	{
		guiTmp.loadWidgetsFromFile(guiFormPath);
	}
	catch (std::exception const& e)
	{
		LOG_ERROR(
			"Unable to call loadWidgetsFromFile. In %s."
			"Error: %s",
			FUNC_NAME,
			e.what());
		return false;
	}

	tgui::Widget::Ptr w;
	if ((w = gui.get<tgui::Widget>("WindowSaves")).get())
	{
		w->setVisible(!w->isVisible());
		return true;
	}

	tgui::ChildWindow::Ptr windowFiles, windowNew;
	windowFiles = tgui::ChildWindow::copy(
		guiTmp.get<tgui::ChildWindow>("WindowSaves"));
	windowNew = tgui::ChildWindow::create(
		"WindowSaves");

	for (tgui::Widget::Ptr ptr : windowFiles->getWidgets())
	{
		ptr->setParent(nullptr);
		windowNew->add(ptr, ptr->getWidgetName());
	}

	tgui::Panel::Ptr panelFile;
	tgui::VerticalLayout::Ptr layoutFiles;
	tgui::ScrollablePanel::Ptr scrollableFiles;

	panelFile = 
		windowNew->get<tgui::Panel>("PanelFile");
	layoutFiles = 
		windowNew->get<tgui::VerticalLayout>("LayoutFiles");
	scrollableFiles = 
		windowNew->get<tgui::ScrollablePanel>("ScrollableFiles");
	
	int panelHeight = 100;
	int panelCount = 1;
	for (size_t i = 0; i < panelCount; i++)
	{
		tgui::Panel::Ptr panelFileCopy;
		panelFileCopy = tgui::Panel::copy(panelFile);
		panelFileCopy->setWidgetName("fileNum" + std::to_string(i));
		panelFileCopy->setHeight(panelHeight);
		layoutFiles->add(panelFileCopy);
	}

	layoutFiles->remove(panelFile);
	layoutFiles->setHeight(panelCount * panelHeight);
	scrollableFiles->setContentSize(tgui::Vector2f{0.f, (float)(panelCount * panelHeight) });
	
	windowNew->setWidgetName("WindowSaves");
	windowNew->setResizable(windowFiles->isResizable());
	windowNew->setSize(windowFiles->getSize());

	gui.add(windowNew, "WindowSaves");

	return true;
}

#undef QUICK_FIND