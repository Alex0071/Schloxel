// Fill out your copyright notice in the Description page of Project Settings.


#include "GreedyChunk.h"

#include "ChunkWorld.h"
#include "Enums.h"
#include "FastNoiseWrapper.h"
#include "MeshThread.h"
#include "Engine/Texture2D.h"
#include "PixelFormat.h"
#include "Math/UnrealMathUtility.h"

#include "ProceduralMeshComponent.h"

// Sets default values
AGreedyChunk::AGreedyChunk()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>("Noise");

	Mesh->SetCastShadow(true);

	SetRootComponent(Mesh);
}

// Called when the game starts or when spawned
void AGreedyChunk::BeginPlay()
{
	Super::BeginPlay();

	Blocks.SetNum(ChunkSize.X * ChunkSize.Y * ChunkSize.Z);

	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 1337, 0.003f, EFastNoise_Interp::Quintic,
	                      EFastNoise_FractalType::FBM, 3, 2, 0.2, 0.2);


	GenerateBlocks();

	GenerateMesh();
}

void AGreedyChunk::GenerateBlocks()
{
	const auto Location = GetActorLocation();

	for (int x = 0; x < ChunkSize.X; x++)
	{
		for (int y = 0; y < ChunkSize.Y; y++)
		{
			const float Xpos = (x * VoxelSize + Location.X) / VoxelSize;
			const float Ypos = (y * VoxelSize + Location.Y) / VoxelSize;

			// Get the height for the current voxel column
			const float Height = GetPrecomputedPixelBrightness(x, y);

			// Ensure that height is within the bounds of ChunkSize.Z
			int32 HeightInt = FMath::Clamp(FMath::RoundToInt(Height), 0, ChunkSize.Z);

			// Fill blocks below the height with solid blocks (e.g., Stone)
			for (int z = 0; z < HeightInt; z++)
			{
				Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
			}

			// Fill the rest of the chunk with Air
			for (int z = HeightInt; z < ChunkSize.Z; z++)
			{
				Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
			}
		}
	}
}


float AGreedyChunk::GetPrecomputedPixelBrightness(int X, int Y) const
{
	FIntPoint PixelCoords(X, Y);

	// Attempt to find the value directly
	if (int* Brightness = CachedBrightnessMap->Find(PixelCoords))
	{
		return *Brightness; // Dereference and return the value
	}

	// Return a default value if the pixel is out of bounds or not found
	UE_LOG(LogTemp, Warning, TEXT("Pixel (%d, %d) not found in CachedBrightnessMap!"), X, Y);
	return 0.0f;
}


void AGreedyChunk::ApplyMesh()
{
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		TQueue<FChunkMeshData>::FElementType data;
		if (!MeshDataQueue.IsEmpty() && MeshDataQueue.Dequeue(data))
		{
			Mesh->SetMaterial(0, Material);
			Mesh->CreateMeshSection(0, data.Vertices, data.Triangles, data.Normals, data.UV0, data.Colors,
			                        TArray<FProcMeshTangent>(), true);
		}
	});
}


void AGreedyChunk::GenerateMesh()
{
	MeshThread = new AMeshThread(this);
}


void AGreedyChunk::ModifyVoxel(const FIntVector Position, const EBlock Block)
{
	if (Position.X >= ChunkSize.X || Position.Y >= ChunkSize.Y || Position.Z >= ChunkSize.Z || Position.X < 0 ||
		Position.Y < 0 || Position.Z < 0)
		return;

	constexpr int Radius = 8;

	// Iterate through all positions within a limited range
	for (int x = -Radius + 1; x <= Radius - 1; ++x)
	{
		for (int y = -Radius + 1; y <= Radius - 1; ++y)
		{
			for (int z = -Radius + 1; z <= Radius - 1; ++z)
			{
				float distance = FVector::Dist(FVector(x, y, z), FVector::ZeroVector);
				if (distance <= Radius)
				{
					const int index = GetBlockIndex(Position.X + x, Position.Y + y, Position.Z + z);
					Blocks[index] = Block;
				}
			}
		}
	}

	ClearMesh();

	GenerateMesh();

	ApplyMesh();
}

int AGreedyChunk::GetBlockIndex(int X, int Y, int Z) const
{
	return Z * ChunkSize.X * ChunkSize.Y + Y * ChunkSize.X + X;
}

EBlock AGreedyChunk::GetBlock(FIntVector Index) const
{
	if (Index.X >= ChunkSize.X || Index.Y >= ChunkSize.Y || Index.Z >= ChunkSize.Z || Index.X < 0 || Index.Y < 0 ||
		Index.Z < 0)
		return EBlock::Air;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)];
}

bool AGreedyChunk::CompareMask(FMask M1, FMask M2) const
{
	return M1.Block == M2.Block && M1.Normal == M2.Normal;
}


void AGreedyChunk::ClearMesh()
{
	TQueue<FChunkMeshData>::FElementType data;
	if (!MeshDataQueue.IsEmpty() && MeshDataQueue.Dequeue(data))
	{
		data.Clear();
	}
}
