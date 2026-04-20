#include "Frosting/FrostingMeshActor.h"
#include "Algo/Reverse.h"


// Sets default values
AFrostingMeshActor::AFrostingMeshActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FrostingMesh"));
	SetRootComponent(ProceduralMesh);
}

void AFrostingMeshActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

// Called when the game starts or when spawned
void AFrostingMeshActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFrostingMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFrostingMeshActor::GenerateMesh()
{
	if (SourceFrostingData)
	{
		TArray<FVector> Normals;
		TArray<FVector2D> UVS;
		TArray<FLinearColor> LinearColors;
		TArray<FProcMeshTangent> Tangents;

		auto Indices = SourceFrostingData->Frosting.Mesh.Indices;

		Algo::Reverse(Indices);
		
		if (bGenerateNormals)
		{
			Normals.Init(FVector(0, 0, 0), SourceFrostingData->Frosting.Mesh.Vertices.Num());

			for (int i = 0; i < Indices.Num(); i += 3)
			{
				auto& v0 = SourceFrostingData->Frosting.Mesh.Vertices[Indices[i]];
				auto& v1 = SourceFrostingData->Frosting.Mesh.Vertices[Indices[i + 1]];
				auto& v2 = SourceFrostingData->Frosting.Mesh.Vertices[Indices[i + 2]];

				auto t = (v1 - v0);
				auto b = (v1 - v2);
				auto n = FVector::CrossProduct(t, b);

				n.Normalize();

				Normals[Indices[i]] += n;
				Normals[Indices[i + 1]] += n;
				Normals[Indices[i + 2]] += n;
			}

			for (auto& n : Normals)
			{
				n.Normalize();
			}
		}

		ProceduralMesh->ClearAllMeshSections();
		ProceduralMesh->CreateMeshSection_LinearColor(0, SourceFrostingData->Frosting.Mesh.Vertices, Indices, Normals, SourceFrostingData->Frosting.Mesh.InnerOuterPrismDistance, LinearColors, Tangents, false);
		
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(0, 5, FColor::Red, "Source Frosting Data is missing");
	}
}

