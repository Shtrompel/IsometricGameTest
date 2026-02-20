#pragma once

#include <string>

#include "TGUI/TGUI.hpp"

struct GameSettings;

struct SettingsDefaults;

class WindowManager;

struct PopupConfirmationData
{
	std::string title = "";
	std::string description = "";

	std::string cancelStr = "Cancel";
	std::string okStr = "Okay";

	PopupConfirmationData(
		const std::string& title,
		const std::string& description)
	{
		this->title = title;
		this->description = description;
	}

	virtual void onConfirmation()
	{
	}

	virtual void onCancel()
	{
	}

	virtual void postInit()
	{
	}
};


void guiutils_popup_confirmation_window(tgui::Gui& gui, PopupConfirmationData* data);

bool guiutils_settings(
	WindowManager* manager,
	tgui::Gui& gui
	);

bool guiutils_save_files(tgui::Gui& gui);