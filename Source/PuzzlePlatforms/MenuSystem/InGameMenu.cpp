// Fill out your copyright notice in the Description page of Project Settings.


#include "InGameMenu.h"
#include "Components/Button.h"

bool UInGameMenu::Initialize() {

	bool Success = Super::Initialize();

	if (!Success) return false;

	//TODO: SETUP

	if (!ensure(CancelPauseMenuButton != nullptr)) return false;

	CancelPauseMenuButton->OnClicked.AddDynamic(this, &UInGameMenu::ResumeGame);

	if (!ensure(QuitToMainMenuButton != nullptr)) return false;

	QuitToMainMenuButton->OnClicked.AddDynamic(this, &UInGameMenu::LoadMainMenu);

	return true;
}

void UInGameMenu::ResumeGame() {

	TearDown();
}

void UInGameMenu::LoadMainMenu() {

	if (MenuInterface != nullptr) {

		TearDown();
		MenuInterface->LoadMainMenu();
	}
}