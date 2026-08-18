using UnrealBuildTool;
using System.IO;

public class GaussiansEditor: ModuleRules
{
    public GaussiansEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/happly"));

        PublicDependencyModuleNames.Add("Gaussians");

        PrivateDependencyModuleNames.AddRange(new string[] 
        {
            "Core", 
            "CoreUObject", 

            "Engine", 
            "UnrealEd",
            "ApplicationCore",
            "InputCore",

            "Slate",
            "SlateCore",
            "ToolWidgets",

            "GaussiansRenderer", 
            "Gaussians", 
        });

        bUseRTTI = true;
    }
}
