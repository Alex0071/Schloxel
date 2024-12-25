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

	UFUNCTION(BlueprintCallable, Category="Vox Model")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	FIntVector WorldToModelPosition(const FVector& WorldPosition) const
	{
		FVector LocalPosition = WorldPosition - GetActorLocation();
		return FIntVector(
			(LocalPosition.X / VoxelScale),
			(LocalPosition.Y / VoxelScale),
			(LocalPosition.Z / VoxelScale)
		);
	}


	UPROPERTY(EditAnywhere, Category="Vox Model")
	FString VoxFilePath;

	UPROPERTY(EditAnywhere, Category="Vox Model")
	float VoxelScale = 100.0f;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, Category="Vox Model")
	TObjectPtr<UMaterialInterface> Material;

	FIntVector ModelDimensions;

protected:
	virtual void BeginPlay() override;

private:
	TArray<EBlock> Blocks;
	void BuildGreedyMesh(const TArray<EBlock>& InBlocks, const FIntVector& Dimensions);
	void LoadVoxModel();
};
