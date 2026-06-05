// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Gaussians : ModuleRules
{
    public string PluginRoot =>
            System.IO.Path.GetFullPath(
                System.IO.Path.Combine(ModuleDirectory, "../../")
            );

    public Gaussians(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        string EnginePath = Path.GetFullPath(Target.RelativeEnginePath);

        PrivateIncludePaths.AddRange(
            new string[] {
                EnginePath + "Source/Runtime/Renderer/Private/",
                EnginePath + "Source/Runtime/Renderer/Internal/",

				PluginRoot + "Shaders/Shared/"
            }
        );

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ProceduralMeshComponent"
            }
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",

				"RHI",
				"RenderCore", 
				"Renderer",

				"NiagaraCore", 
				"Niagara", 

				"VectorVM", 
				"Projects"
            }
		);
	}
}
