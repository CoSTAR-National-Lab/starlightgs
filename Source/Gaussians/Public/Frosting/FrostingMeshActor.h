

#pragma once

#include "CoreMinimal.h"
#include "FrostingAsset.h"
#include "GSplatAsset.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "FrostingMeshActor.generated.h"

UCLASS()
class GAUSSIANS_API AFrostingMeshActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFrostingMeshActor();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(CallInEditor, Category="Frosting")
	void GenerateMesh();

	UPROPERTY(EditAnywhere, Category="Frosting")
	bool bGenerateNormals = true;

	UPROPERTY(BlueprintType, EditAnywhere)
	UProceduralMeshComponent* ProceduralMesh;
	
	UPROPERTY(BlueprintType, EditAnywhere, Category = "Frosting")
	UFrostingDataAsset* SourceFrostingData;



};
