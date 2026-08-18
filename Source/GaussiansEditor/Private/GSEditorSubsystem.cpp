#include "GSEditorSubsystem.h"
#include "Editor.h"

#include "GSWorldSubsystem.h"


void UGSEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OnActorsDeleteHandle = FEditorDelegates::OnDeleteActorsEnd.AddLambda([this]()
	{
		if (auto Subsystem = GetWorld()->GetSubsystem<UGSWorldSubsystem>())
		{
			Subsystem->CleanClippingVolumesCollection();
			Subsystem->MarkClippingDirtyFlags();
		}
	});
}

void UGSEditorSubsystem::Deinitialize()
{
	Super::Deinitialize();

	FEditorDelegates::OnDeleteActorsEnd.Remove(OnActorsDeleteHandle);
}
