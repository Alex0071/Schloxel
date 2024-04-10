// Fill out your copyright notice in the Description page of Project Settings.


#include "GreedyChunkSlow.h"

#include "ChunkMeshData.h"
#include "Enums.h"
#include "FastNoiseWrapper.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AGreedyChunkSlow::AGreedyChunkSlow()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Noise = CreateDefaultSubobject<UFastNoiseWrapper>("Noise");

	Mesh->SetCastShadow(true);

	SetRootComponent(Mesh);
}

// Called when the game starts or when spawned
void AGreedyChunkSlow::BeginPlay()
{
	Super::BeginPlay();

	Blocks.SetNum(ChunkSize.X * ChunkSize.Y * ChunkSize.Z);

	//	const float frequency = FMath::RandRange(0.003f, 0.03f);
	Noise->SetupFastNoise(EFastNoise_NoiseType::Perlin, 1337, 0.005f, EFastNoise_Interp::Quintic, EFastNoise_FractalType::FBM);


	GenerateBlocks();

	GenerateMesh();
	ApplyMesh();
}

void AGreedyChunkSlow::GenerateBlocks()
{
	const auto Location = GetActorLocation();

	for (int x = 0; x < ChunkSize.X; x++)
	{
		for (int y = 0; y < ChunkSize.Y; y++)
		{
			const float Xpos = (x * VoxelSize + Location.X) / VoxelSize;
			const float Ypos = (y * VoxelSize + Location.Y) / VoxelSize;

			const int Height = FMath::Clamp(FMath::RoundToInt((Noise->GetNoise2D(Xpos, Ypos) + 1) * ChunkSize.Z / 2), 0, ChunkSize.Z);

			for (int z = 0; z < Height; z++)
			{
				Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
			}


			for (int z = Height; z < ChunkSize.Z; z++)
			{
				Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
			}
		}
	}
}

void AGreedyChunkSlow::ApplyMesh()
{
	Mesh->CreateMeshSection(0, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UV0, MeshData.Colors, TArray<FProcMeshTangent>(), false);

	UE_LOG(LogTemp, Warning, TEXT("Slow Chunk: %i"), Blocks.Num());
}


void AGreedyChunkSlow::GenerateMesh()
{
	for (int Axis = 0; Axis < 3; ++Axis)
	{
		const int Axis1 = (Axis + 1) % 3;
		const int Axis2 = (Axis + 2) % 3;

		const int MainAxisLimit = ChunkSize[Axis];
		int Axis1Limit = ChunkSize[Axis1];
		int Axis2Limit = ChunkSize[Axis2];

		auto DeltaAxis1 = FIntVector::ZeroValue;
		auto DeltaAxis2 = FIntVector::ZeroValue;

		auto ChunkItr = FIntVector::ZeroValue;
		auto AxisMask = FIntVector::ZeroValue;

		AxisMask[Axis] = 1;

		TArray<FMask> Mask;
		Mask.SetNum(Axis1Limit * Axis2Limit);

		//check each slice
		for (ChunkItr[Axis] = -1; ChunkItr[Axis] < MainAxisLimit;)
		{
			int N = 0;

			for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
			{
				for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
				{
					const auto CurrentBlock = GetBlock(ChunkItr);
					const auto CompareBlock = GetBlock(ChunkItr + AxisMask);

					const bool CurrentBlockOpaque = CurrentBlock != EBlock::Air;
					const bool CompareBlockOpaque = CompareBlock != EBlock::Air;

					if (CurrentBlockOpaque == CompareBlockOpaque)
					{
						Mask[N++] = FMask{EBlock::Null, 0};
					}
					else if (CurrentBlockOpaque)
					{
						Mask[N++] = FMask{CurrentBlock, 1};
					}
					else
					{
						Mask[N++] = FMask{CompareBlock, -1};
					}
				}
			}

			++ChunkItr[Axis];
			N = 0;

			// Generate mesh from the mask
			for (int j = 0; j < Axis2Limit; ++j)
			{
				for (int i = 0; i < Axis1Limit;)
				{
					if (Mask[N].Normal != 0)
					{
						const auto CurrentMask = Mask[N];
						ChunkItr[Axis1] = i;
						ChunkItr[Axis2] = j;

						int width;

						for (width = 1; i + width < Axis1Limit && CompareMask(Mask[N + width], CurrentMask); ++width)
						{
						}

						int height;
						bool done = false;

						for (height = 1; j + height < Axis2Limit; ++height)
						{
							for (int k = 0; k < width; ++k)
							{
								if (CompareMask(Mask[N + k + height * Axis1Limit], CurrentMask)) continue;

								done = true;
								break;
							}

							if (done) break;
						}

						DeltaAxis1[Axis1] = width;
						DeltaAxis2[Axis2] = height;


						CreateQuad(CurrentMask, AxisMask, width, height,
						           ChunkItr,
						           ChunkItr + DeltaAxis1,
						           ChunkItr + DeltaAxis2,
						           ChunkItr + DeltaAxis1 + DeltaAxis2
						);


						DeltaAxis1 = FIntVector::ZeroValue;
						DeltaAxis2 = FIntVector::ZeroValue;

						for (int l = 0; l < height; ++l)
						{
							for (int k = 0; k < width; ++k)
							{
								Mask[N + k + l * Axis1Limit] = FMask{EBlock::Null, 0};
							}
						}

						i += width;
						N += width;
					}
					else
					{
						i++;
						N++;
					}
				}
			}
		}
	}
}

void AGreedyChunkSlow::CreateQuad(FMask Mask, FIntVector AxisMask, int Width, int Height, FIntVector V1, FIntVector V2, FIntVector V3, FIntVector V4)
{
	const auto Normal = FVector(AxisMask * Mask.Normal);

	TArray<FColor> Colors = {FColor(147, 217, 79), FColor(128, 181, 181), FColor(106, 190, 190), FColor(111, 183, 61)};
	FColor Color = Colors[FMath::RandRange(0, Colors.Num() - 1)];


	MeshData.Vertices.Add(FVector(V1) * VoxelSize);
	MeshData.Vertices.Add(FVector(V2) * VoxelSize);
	MeshData.Vertices.Add(FVector(V3) * VoxelSize);
	MeshData.Vertices.Add(FVector(V4) * VoxelSize);

	MeshData.Triangles.Add(MeshData.VertexCount);
	MeshData.Triangles.Add(MeshData.VertexCount + 2 + Mask.Normal);
	MeshData.Triangles.Add(MeshData.VertexCount + 2 - Mask.Normal);
	MeshData.Triangles.Add(MeshData.VertexCount + 3);
	MeshData.Triangles.Add(MeshData.VertexCount + 1 - Mask.Normal);
	MeshData.Triangles.Add(MeshData.VertexCount + 1 + Mask.Normal);

	MeshData.UV0.Append({
		FVector2D(Height, Width),
		FVector2D(Height, 0),
		FVector2D(0, Width),
		FVector2D(0, 0),
	});

	MeshData.Normals.Add(Normal);
	MeshData.Normals.Add(Normal);
	MeshData.Normals.Add(Normal);
	MeshData.Normals.Add(Normal);

	MeshData.Colors.Add(Color);
	MeshData.Colors.Add(Color);
	MeshData.Colors.Add(Color);
	MeshData.Colors.Add(Color);


	MeshData.VertexCount += 4;
}


void AGreedyChunkSlow::ModifyVoxel(const FIntVector Position, const EBlock Block)
{
	if (Position.X >= ChunkSize.X || Position.Y >= ChunkSize.Y || Position.Z >= ChunkSize.Z || Position.X < 0 || Position.Y < 0 || Position.Z < 0) return;

	const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);

	Blocks[Index] = Block;

	ClearMesh();

	//GenerateMesh();

	ApplyMesh();
}

int AGreedyChunkSlow::GetBlockIndex(int X, int Y, int Z) const
{
	return Z * ChunkSize.X * ChunkSize.Y + Y * ChunkSize.X + X;
}


EBlock AGreedyChunkSlow::GetBlock(FIntVector Index) const
{
	if (Index.X >= ChunkSize.X || Index.Y >= ChunkSize.Y || Index.Z >= ChunkSize.Z || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return EBlock::Air;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)];
}

bool AGreedyChunkSlow::CompareMask(FMask M1, FMask M2) const
{
	return M1.Block == M2.Block && M1.Normal == M2.Normal;
}


void AGreedyChunkSlow::ClearMesh()
{
	MeshData.VertexCount = 0;
	MeshData.Clear();
}
