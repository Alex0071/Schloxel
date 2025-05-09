#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PackedNormal.h"
#include "Enums.h"
#include "RealtimeMeshComponent.h"
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

		LocalPosition.X += (ModelDimensions.X * VoxelSize * 0.5f);
		LocalPosition.Y += (ModelDimensions.Y * VoxelSize * 0.5f);

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
	TObjectPtr<URealtimeMeshComponent> MeshComponent;

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
