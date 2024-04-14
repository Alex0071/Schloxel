// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkMeshData.h"
#include "Enums.h"
#include "GreedyChunk.generated.h"

class AMeshThread;
class UFastNoiseWrapper;
class UProceduralMeshComponent;

UCLASS()
class AGreedyChunk : public AActor
{
	GENERATED_BODY()

public:
	struct FMask
	{
		EBlock Block;
		int Normal;
	};

	AGreedyChunk();

	UFUNCTION(BlueprintCallable, Category="Chunk")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Material;

	int VoxelSize;

	FIntVector ChunkSize;

	EBlock GetBlock(FIntVector Index) const;

	bool CompareMask(FMask M1, FMask M2) const;


	void ApplyMesh();


	TQueue<FChunkMeshData> MeshDataQueue;

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


	void ClearMesh();


	AMeshThread* MeshThread;
};
