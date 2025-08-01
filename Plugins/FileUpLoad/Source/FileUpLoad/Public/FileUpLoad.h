// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

class FToolBarBuilder;
class FMenuBuilder;

class FFileUpLoadModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();
	
	// FTP 시스템 초기화
	void InitializeFtpSystem();
	
	// 탭 스폰 함수들
	TSharedRef<SDockTab> SpawnFileUploadTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> SpawnFtpClientTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> SpawnFileManagerTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> SpawnUploadHistoryTab(const FSpawnTabArgs& SpawnTabArgs);

	public:
    bool UploadFile(const FString& LocalPath, const FString& RemoteUrl, const FString& User, const FString& Pass);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<FTabManager> FileUpLoadTabManager;
};
