// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChunkMeshData.h"
#include "GameFramework/Actor.h"
#include "GreedyChunkSlow.generated.h"

struct FChunkMeshData;
class UFastNoiseWrapper;
class UProceduralMeshComponent;

UCLASS()
class AGreedyChunkSlow : public AActor
{
	GENERATED_BODY()

public:
	struct FMask
	{
		EBlock Block;
		int Normal;
	};

	AGreedyChunkSlow();

	UFUNCTION(BlueprintCallable, Category="Chunk")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Material;

	int VoxelSize;

	FIntVector ChunkSize;

	EBlock GetBlock(FIntVector Index) const;

	bool CompareMask(FMask M1, FMask M2) const;


	void ApplyMesh();
	

	int VertexCount = 0;

protected:
	virtual void BeginPlay() override;

private:
	TObjectPtr<UProceduralMeshComponent> Mesh;
	TObjectPtr<UFastNoiseWrapper> Noise;

	TArray<EBlock> Blocks;


	void GenerateBlocks();


	void GenerateMesh();


	int GetBlockIndex(int X, int Y, int Z) const;
	void CreateQuad(FMask Mask, FIntVector AxisMask, int Width, int Height, FIntVector V1, FIntVector V2, FIntVector V3, FIntVector V4);

	void ClearMesh();

	FChunkMeshData MeshData;
};
