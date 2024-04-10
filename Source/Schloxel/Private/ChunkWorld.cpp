// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkWorld.h"

#include "GreedyChunk.h"
#include "GreedyChunkSlow.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AChunkWorld::AChunkWorld()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AChunkWorld::BeginPlay()
{
	Super::BeginPlay();

	for (int x = -DrawDistance; x <= DrawDistance; x++)
	{
		for (int y = -DrawDistance; y <= DrawDistance; y++)
		{
			auto transform = FTransform(
				FRotator::ZeroRotator,
				FVector(x * ChunkSize.X * VoxelSize, y * ChunkSize.Y * VoxelSize, 0),
				FVector::OneVector
			);
			if (Cast<AGreedyChunk>(Chunk.GetDefaultObject()))
			{
				const auto chunk = GetWorld()->SpawnActorDeferred<AGreedyChunk>(Chunk, transform, this);

				chunk->Material = Material;
				chunk->ChunkSize = ChunkSize;
				chunk->VoxelSize = VoxelSize;

				UGameplayStatics::FinishSpawningActor(chunk, transform);
			}
			if (Cast<AGreedyChunkSlow>(Chunk.GetDefaultObject()))
			{
				const auto chunk = GetWorld()->SpawnActorDeferred<AGreedyChunkSlow>(Chunk, transform, this);

				chunk->Material = Material;
				chunk->ChunkSize = ChunkSize;
				chunk->VoxelSize = VoxelSize;

				UGameplayStatics::FinishSpawningActor(chunk, transform);
			}
		}
	}
}
