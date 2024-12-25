#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "PackedNormal.h"
#include "Enums.h"
#include "VoxModel.generated.h"

UCLASS(MinimalAPI)
class AVoxModel : public AActor
{
	GENERATED_BODY()

public:
	AVoxModel();
	void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category="Vox Model")
	FString VoxFilePath;

	UPROPERTY(EditAnywhere, Category="Vox Model")
	float VoxelScale = 100.0f;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, Category="Vox Model")
	TObjectPtr<UMaterialInterface> Material;

private:
	void BuildGreedyMesh(const TArray<EBlock>& Blocks, const FIntVector& Dimensions);
};
