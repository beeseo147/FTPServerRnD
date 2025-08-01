// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FileUpLoad : ModuleRules
{
	public FileUpLoad(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// C++ 표준 설정
		CppStandard = CppStandardVersion.Cpp17;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);

        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"InputCore",
				"Slate", 
				"SlateCore",
				"UnrealEd",
				"Json",
				"JsonUtilities",
				"Projects",
				"EditorFramework",
				"ToolMenus",
				"EditorSubsystem"
			}
		);

        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// 중복된 모듈들 제거
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
