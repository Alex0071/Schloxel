#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "PackedNormal.h"
#include "Enums.h"
#include "VoxMeshData.h"
#include "VoxModel.generated.h"

UCLASS(MinimalAPI)
class AVoxModel : public AActor
{
	GENERATED_BODY()

public:
	AVoxModel();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="Vox Model")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	FIntVector WorldToModelPosition(const FVector& WorldPosition) const
	{
		FVector LocalPosition = WorldPosition - GetActorLocation();
		// Add back half dimensions on X and Y since model is centered on those axes
		LocalPosition.X += (ModelDimensions.X * VoxelSize * 0.5f);
		LocalPosition.Y += (ModelDimensions.Y * VoxelSize * 0.5f);
		// Z stays as is since pivot is at bottom
		return FIntVector(
			(LocalPosition.X / VoxelSize),
			(LocalPosition.Y / VoxelSize),
			(LocalPosition.Z / VoxelSize)
		);
	}


	UPROPERTY(EditAnywhere, Category="Vox Model")
	FString VoxFilePath;

	UPROPERTY(EditAnywhere, Category="Vox Model")
	float VoxelSize = 10.0f;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, Category="Vox Model")
	TObjectPtr<UMaterialInterface> Material;

	FIntVector ModelDimensions;

	TQueue<FVoxMeshData> MeshDataQueue;

	TArray<EBlock> Blocks;

	void ApplyMesh();

	

protected:
	virtual void BeginPlay() override;

private:
	void GenerateMesh();
	void LoadVoxModel();
	void ClearMeshData();
};
