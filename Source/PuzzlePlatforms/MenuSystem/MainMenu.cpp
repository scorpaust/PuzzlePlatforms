// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu.h"
#include "Components/Button.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/WidgetSwitcher.h"
#include "ServerRow.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"


UMainMenu::UMainMenu(const FObjectInitializer& ObjectInitializer) {

	ConstructorHelpers::FClassFinder<UUserWidget> ServerRowBPClass(TEXT("/Game/MenuSystem/ServerRow_WBP"));

	if (!ensure(ServerRowBPClass.Class != nullptr)) return;

	ServerRowClass = ServerRowBPClass.Class;

}

bool UMainMenu::Initialize() {

	bool Success = Super::Initialize();

	if (!Success) return false;

	//TODO: SETUP

	if (!ensure(HostButton != nullptr)) return false;

	HostButton->OnClicked.AddDynamic(this, &UMainMenu::OpenHostMenu);

	if (!ensure(JoinButton != nullptr)) return false;

	JoinButton->OnClicked.AddDynamic(this, &UMainMenu::OpenJoinMenu);

	if (!ensure(QuitButton != nullptr)) return false;

	QuitButton->OnClicked.AddDynamic(this, &UMainMenu::QuitGame);

	if (!ensure(CancelHostButton != nullptr)) return false;

	CancelHostButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);

	if (!ensure(HostGameButton != nullptr)) return false;

	HostGameButton->OnClicked.AddDynamic(this, &UMainMenu::HostServer);

	if (!ensure(CancelJoinButton != nullptr)) return false;

	CancelJoinButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);

	if (!ensure(JoinIPButton != nullptr)) return false;

	JoinIPButton->OnClicked.AddDynamic(this, &UMainMenu::JoinServer);

	return true;
}

void UMainMenu::OpenHostMenu() {

	MenuSwitcher->SetActiveWidget(HostMenu);

}


void UMainMenu::HostServer() {

	if (MenuInterface != nullptr) {

		FString ServerName = ServerHostName->Text.ToString();

		MenuInterface->Host(ServerName);
	}
}

void UMainMenu::SetServerList(TArray<FServerData> ServerNames) {

	UWorld* World = this->GetWorld();

	if (!ensure(World != nullptr)) return;

	ServerList->ClearChildren();

	uint32 i = 0;

	for (const FServerData& ServerData : ServerNames) {

		UServerRow* Row = CreateWidget<UServerRow>(this, ServerRowClass);

		Row->ServerName->SetText(FText::FromString(ServerData.Name));

		Row->HostUser->SetText(FText::FromString(ServerData.HostUserName));

		FString FractionText = FString::Printf(TEXT("%d / %d"), ServerData.CurrentPlayers, ServerData.TotalPlayers);

		Row->ConnectionFraction->SetText(FText::FromString(FractionText));

		Row->Setup(this, i);

		++i;

		ServerList->AddChild(Row);
	}	
}

void UMainMenu::SelectIndex(uint32 Index) {

	SelectedIndex = Index;
	UpdateChildren();
}

void UMainMenu::UpdateChildren() {

	for (int32 i = 0; i < ServerList->GetChildrenCount(); i++) {

		auto Row = Cast<UServerRow>(ServerList->GetChildAt(i));

		if (Row != nullptr) {

			Row->bSelected = (SelectedIndex.IsSet() && SelectedIndex.GetValue() == i);
		}
	}
}

void UMainMenu::JoinServer() {

	if (SelectedIndex.IsSet() && MenuInterface != nullptr) {

		UE_LOG(LogTemp, Warning, TEXT("Selected Index: %d"), SelectedIndex.GetValue());
		MenuInterface->Join(SelectedIndex.GetValue());
	}
	else {

		UE_LOG(LogTemp, Warning, TEXT("Selected Index Not Set"));
	}
}

void UMainMenu::QuitGame() {

	UWorld* World = GetWorld();

	if (!ensure(World != nullptr)) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();

	PlayerController->ConsoleCommand("quit");
}

void UMainMenu::OpenJoinMenu() {

	if (!ensure(MenuSwitcher != nullptr)) return;

	if (!ensure(JoinMenu != nullptr)) return;

	MenuSwitcher->SetActiveWidget(JoinMenu);

	if (MenuInterface != nullptr) {

		MenuInterface->RefreshingServerList();
	}
}

void UMainMenu::OpenMainMenu() {

	if (!ensure(MenuSwitcher != nullptr)) return;

	if (!ensure(this != nullptr)) return;

	MenuSwitcher->SetActiveWidget(this);
}