#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "GSEditorSubsystem.generated.h"


// Need one per world (because this passes along editor callbacks to the main world subsystem)
// Otherwise this would have been an editor subsystem
// TODO: Name is potentially confusing
UCLASS()
class UGSEditorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	FDelegateHandle OnActorsDeleteHandle;

};
