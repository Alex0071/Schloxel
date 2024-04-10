// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshThread.h"


AMeshThread::AMeshThread(AGreedyChunk* greedyChunk)
{
	GreedyChunk = greedyChunk;
	Thread = FRunnableThread::Create(this, TEXT("MeshThread"));
}

bool AMeshThread::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("My custom thread has been initialized"));

	return true;
}

uint32 AMeshThread::Run()
{
	for (int Axis = 0; Axis < 3; ++Axis)
	{
		const int Axis1 = (Axis + 1) % 3;
		const int Axis2 = (Axis + 2) % 3;

		const int MainAxisLimit = GreedyChunk->ChunkSize[Axis];
		int Axis1Limit = GreedyChunk->ChunkSize[Axis1];
		int Axis2Limit = GreedyChunk->ChunkSize[Axis2];

		auto DeltaAxis1 = FIntVector::ZeroValue;
		auto DeltaAxis2 = FIntVector::ZeroValue;

		auto ChunkItr = FIntVector::ZeroValue;
		auto AxisMask = FIntVector::ZeroValue;

		AxisMask[Axis] = 1;

		TArray<AGreedyChunk::FMask> Mask;
		Mask.SetNum(Axis1Limit * Axis2Limit);

		//check each slice
		for (ChunkItr[Axis] = -1; ChunkItr[Axis] < MainAxisLimit;)
		{
			int N = 0;

			for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
			{
				for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
				{
					const auto CurrentBlock = GreedyChunk->GetBlock(ChunkItr);
					const auto CompareBlock = GreedyChunk->GetBlock(ChunkItr + AxisMask);

					const bool CurrentBlockOpaque = CurrentBlock != EBlock::Air;
					const bool CompareBlockOpaque = CompareBlock != EBlock::Air;

					if (CurrentBlockOpaque == CompareBlockOpaque)
					{
						Mask[N++] = AGreedyChunk::FMask{EBlock::Null, 0};
					}
					else if (CurrentBlockOpaque)
					{
						Mask[N++] = AGreedyChunk::FMask{CurrentBlock, 1};
					}
					else
					{
						Mask[N++] = AGreedyChunk::FMask{CompareBlock, -1};
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

						for (width = 1; i + width < Axis1Limit && GreedyChunk->CompareMask(Mask[N + width], CurrentMask); ++width)
						{
						}

						int height;
						bool done = false;

						for (height = 1; j + height < Axis2Limit; ++height)
						{
							for (int k = 0; k < width; ++k)
							{
								if (GreedyChunk->CompareMask(Mask[N + k + height * Axis1Limit], CurrentMask)) continue;

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
								Mask[N + k + l * Axis1Limit] = AGreedyChunk::FMask{EBlock::Null, 0};
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

	GreedyChunk->MeshDataQueue.Enqueue(ChunkMeshData);

	return 0;
}

void AMeshThread::CreateQuad(AGreedyChunk::FMask Mask, FIntVector AxisMask, int Width, int Height, FIntVector V1, FIntVector V2, FIntVector V3, FIntVector V4)
{
	const auto Normal = FVector(AxisMask * Mask.Normal);


	ChunkMeshData.Vertices.Append({
		FVector(V1) * GreedyChunk->VoxelSize,
		FVector(V2) * GreedyChunk->VoxelSize,
		FVector(V3) * GreedyChunk->VoxelSize,
		FVector(V4) * GreedyChunk->VoxelSize
	});

	ChunkMeshData.Triangles.Append({
		ChunkMeshData.VertexCount,
		ChunkMeshData.VertexCount + 2 + Mask.Normal,
		ChunkMeshData.VertexCount + 2 - Mask.Normal,
		ChunkMeshData.VertexCount + 3,
		ChunkMeshData.VertexCount + 1 - Mask.Normal,
		ChunkMeshData.VertexCount + 1 + Mask.Normal
	});

	ChunkMeshData.Normals.Append({
		Normal,
		Normal,
		Normal,
		Normal
	});

	if (Normal.X == 1 || Normal.X == -1)
	{
		ChunkMeshData.UV0.Append({
			FVector2D(Width, Height),
			FVector2D(0, Height),
			FVector2D(Width, 0),
			FVector2D(0, 0),
		});
	}
	else
	{
		ChunkMeshData.UV0.Append({
			FVector2D(Height, Width),
			FVector2D(Height, 0),
			FVector2D(0, Width),
			FVector2D(0, 0),
		});
	}


	ChunkMeshData.VertexCount += 4;
}

void AMeshThread::Exit()
{
	GreedyChunk->ApplyMesh();
}
