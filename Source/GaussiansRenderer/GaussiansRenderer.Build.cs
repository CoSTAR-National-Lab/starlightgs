using UnrealBuildTool;
using System.IO;

public class GaussiansRenderer : ModuleRules
{
    public string PluginRoot =>
            System.IO.Path.GetFullPath(
                System.IO.Path.Combine(ModuleDirectory, "../../")
            );

    public GaussiansRenderer(ReadOnlyTargetRules Target) : base(Target)
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
				"GameplayTags",

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
