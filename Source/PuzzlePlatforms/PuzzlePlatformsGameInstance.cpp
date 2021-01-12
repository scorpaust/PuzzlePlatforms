// Fill out your copyright notice in the Description page of Project Settings.


#include "PuzzlePlatformsGameInstance.h"
#include "Engine/Engine.h"
#include "TriggerPlatform.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MenuSystem/MainMenu.h"
#include "MenuSystem/MenuWidget.h"

const static FName SESSION_NAME = TEXT("GameSession");
const static FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");

UPuzzlePlatformsGameInstance::UPuzzlePlatformsGameInstance(const FObjectInitializer& ObjectInitializer) {

	ConstructorHelpers::FClassFinder<UUserWidget> MenuBPClass(TEXT("/Game/MenuSystem/MainMenu_WBP"));
	
	if (!ensure(MenuBPClass.Class != nullptr)) return;

	MenuClass = MenuBPClass.Class;

	UE_LOG(LogTemp, Warning, TEXT("Found class %s"), *MenuClass->GetName());

	ConstructorHelpers::FClassFinder<UMenuWidget> InGameMenuBPClass(TEXT("/Game/MenuSystem/InGameMenu_WBP"));

	if (!ensure(InGameMenuBPClass.Class != nullptr)) return;

	InGameMenuClass = InGameMenuBPClass.Class;

	UE_LOG(LogTemp, Warning, TEXT("Found class %s"), *InGameMenuClass->GetName());
}


void UPuzzlePlatformsGameInstance::Init() {
	
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!ensure(Subsystem != nullptr)) return;

	UE_LOG(LogTemp, Warning, TEXT("Found Subsystem %s"), *Subsystem->GetSubsystemName().ToString());

	SessionInterface = Subsystem->GetSessionInterface();

	if (SessionInterface != nullptr) {

		UE_LOG(LogTemp, Warning, TEXT("Found Session Interface"));

		SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnCreateSessionComplete);

		SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnDestroySessionComplete);

		SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnFindSessionsComplete);

		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformsGameInstance::OnJoinSessionComplete);	
	}

	if (GEngine != nullptr) {

		GEngine->OnNetworkFailure().AddUObject(this, &UPuzzlePlatformsGameInstance::OnNetworkFailure);

	}
	
}

void UPuzzlePlatformsGameInstance::Host(FString ServerName) {

	DesiredServerName = ServerName;

	if (SessionInterface.IsValid()) {

		auto ExistingSession = SessionInterface->GetNamedSession(SESSION_NAME);

		if (ExistingSession != nullptr) {

			SessionInterface->DestroySession(SESSION_NAME);
		}

		else {

			CreateSession();
		}

		
	}
	
}

void UPuzzlePlatformsGameInstance::OnCreateSessionComplete(FName SessionName, bool Succeeded) {

	if (!Succeeded) {
		
		UE_LOG(LogTemp, Warning, TEXT("Could not create session."));
		
		return;
	}


	if (Menu != nullptr) {

		Menu->TearDown();
	}

	UEngine* Engine = GetEngine();

	if (!ensure(Engine != nullptr)) return;

	Engine->AddOnScreenDebugMessage(0, 2.f, FColor::Green, TEXT("Hosting"));

	UWorld* World = GetWorld();

	if (!ensure(World != nullptr)) return;

	World->ServerTravel("/Game/PuzzlePlatforms/Maps/Lobby?listen");
}

void UPuzzlePlatformsGameInstance::RefreshingServerList() {

	SessionSearch = MakeShareable(new FOnlineSessionSearch());

	if (SessionSearch.IsValid()) {

		//SessionSearch->bIsLanQuery = true;

		SessionSearch->MaxSearchResults = 100;

		SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

		UE_LOG(LogTemp, Warning, TEXT("Starting to find Session..."));

		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}

void UPuzzlePlatformsGameInstance::OnFindSessionsComplete(bool Succeeded) {

	if (SessionSearch && Succeeded && Menu != nullptr) {

		UE_LOG(LogTemp, Warning, TEXT("Finished find session."));

		TArray<FServerData> ServerNames;

		for (FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults) {

			UE_LOG(LogTemp, Warning, TEXT("Found session names: %s"), *SearchResult.GetSessionIdStr());
			FServerData Data;
			Data.TotalPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			Data.CurrentPlayers = Data.TotalPlayers - SearchResult.Session.NumOpenPublicConnections;
			
			FString ServerName;
			if (SearchResult.Session.SessionSettings.Get(SERVER_NAME_SETTINGS_KEY, ServerName)) {

				Data.Name = ServerName;

			}
			else {

				Data.Name = "Could Not Find Name";
			}

			Data.HostUserName = SearchResult.Session.OwningUserName;
			ServerNames.Add(Data);
		}

		Menu->SetServerList(ServerNames);
	}
}

void UPuzzlePlatformsGameInstance::OnDestroySessionComplete(FName SessionName, bool Succeeded) {

	if (Succeeded) {

		CreateSession();
	}

}

void UPuzzlePlatformsGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) {

	LoadMainMenu();

}

void UPuzzlePlatformsGameInstance::CreateSession() {

	if (SessionInterface.IsValid()) {

		FOnlineSessionSettings SessionSettings;

		if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL") {

			SessionSettings.bIsLANMatch = true;
		}
		else {

			SessionSettings.bIsLANMatch = false;
		}

		SessionSettings.NumPublicConnections = 5;

		SessionSettings.bShouldAdvertise = true;

		SessionSettings.bUsesPresence = true;

		SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
	}
}

void UPuzzlePlatformsGameInstance::Join(uint32 Index) {

	if (!ensure(SessionInterface != nullptr)) return;

	if (!ensure(SessionSearch != nullptr)) return;
	
	if (Menu != nullptr) {

		Menu->TearDown();

	}

	SessionInterface->JoinSession(0, SESSION_NAME, SessionSearch->SearchResults[Index]);
}

void UPuzzlePlatformsGameInstance::StartSession() {

	if (SessionInterface.IsValid()) {

		SessionInterface->StartSession(SESSION_NAME);

	}

}

void UPuzzlePlatformsGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result) {

	if (!SessionInterface.IsValid()) return;

	FString Address;

	if (!SessionInterface->GetResolvedConnectString(SessionName, Address)) {

		UE_LOG(LogTemp, Warning, TEXT("Could not get connect string"));
		return;
	}

	UEngine* Engine = GetEngine();

	if (!ensure(Engine != nullptr)) return;

	Engine->AddOnScreenDebugMessage(0, 2.f, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	APlayerController* PlayerController = GetFirstLocalPlayerController();

	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);

}

void UPuzzlePlatformsGameInstance::LoadMenu() {

	if (!ensure(MenuClass != nullptr)) return;

	Menu = CreateWidget<UMainMenu>(this, MenuClass);

	if (!ensure(Menu != nullptr)) return;

	Menu->Setup();

	Menu->SetMainMenuInterface(this);
}

void UPuzzlePlatformsGameInstance::LoadPauseMenu() {

	if (!ensure(InGameMenuClass != nullptr)) return;

	UMenuWidget* MenuWidget = CreateWidget<UMenuWidget>(this, InGameMenuClass);

	if (!ensure(MenuWidget != nullptr)) return;

	MenuWidget->Setup();

	MenuWidget->SetMainMenuInterface(this);
}

void UPuzzlePlatformsGameInstance::LoadMainMenu() {

	APlayerController* PlayerController = GetFirstLocalPlayerController();

	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ClientTravel("/Game/MenuSystem/MainMenu", ETravelType::TRAVEL_Absolute);

}