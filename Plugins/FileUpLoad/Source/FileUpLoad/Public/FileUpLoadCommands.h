// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "FileUpLoadStyle.h"

class FFileUpLoadCommands : public TCommands<FFileUpLoadCommands>
{
public:

	FFileUpLoadCommands()
		: TCommands<FFileUpLoadCommands>(TEXT("FileUpLoad"), NSLOCTEXT("Contexts", "FileUpLoad", "FileUpLoad Plugin"), NAME_None, FFileUpLoadStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
}; 