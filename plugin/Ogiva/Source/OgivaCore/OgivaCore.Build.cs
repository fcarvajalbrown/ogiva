using System.IO;
using EpicGames.Core;
using UnrealBuildTool;

public class OgivaCore : ModuleRules
{
	public OgivaCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		FPSemantics = FPSemanticsMode.Precise;
		bUseUnity = false;
		bEnableExceptions = false;
		bUseRTTI = false;

		PrivateDependencyModuleNames.Add("Core");
		ForceIncludeFiles.Add("HAL/Platform.h");

		JsonObject Descriptor = JsonObject.Read(new FileReference(Path.Combine(PluginDirectory, "Ogiva.uplugin")));
		string[] Version = Descriptor.GetStringField("VersionName").Split('.');
		PrivateDefinitions.Add("OGIVA_VERSION_MAJOR=" + Version[0]);
		PrivateDefinitions.Add("OGIVA_VERSION_MINOR=" + Version[1]);
		PrivateDefinitions.Add("OGIVA_VERSION_PATCH=" + Version[2]);
	}
}
