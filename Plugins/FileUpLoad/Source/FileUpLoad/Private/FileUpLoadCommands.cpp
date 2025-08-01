// Copyright Epic Games, Inc. All Rights Reserved.

#include "FileUpLoadCommands.h"

#define LOCTEXT_NAMESPACE "FFileUpLoadModule"

void FFileUpLoadCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "FileUpLoad", "Execute FileUpLoad action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE 