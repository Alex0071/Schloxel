#include "AVoxMeshThread.h"
#include "Enums.h"
#include "VoxModel.h"

AVoxMeshThread::AVoxMeshThread(AVoxModel* _VoxModel)
{
	VoxModel = _VoxModel;
	FRunnableThread::Create(this, TEXT("VoxMeshThread"));
}

bool AVoxMeshThread::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("A Vox Mesh Thread has been initialized"));
	return true;
}

uint32 AVoxMeshThread::Run()
{
    // Visualize chunks first
    VisualizeChunks();
    
    // Then generate the mesh
    BuildGreedyMeshParallel();
    VoxModel->MeshDataQueue.Enqueue(VoxMeshData);
    
    return 0;
}


void AVoxMeshThread::BuildGreedyMeshParallel()
{
    const int32 NumThreads = FPlatformMisc::NumberOfCores();
    const int32 ChunkSize = FMath::Max(1, VoxModel->ModelDimensions.Z / NumThreads);
    
    struct FChunkResult
    {
        FCriticalSection Lock;
        FVoxMeshData Data;
    };
    
    TArray<FChunkResult> ChunkResults;
    ChunkResults.SetNum(NumThreads);

    ParallelFor(NumThreads, [&](int32 ThreadIndex)
    {
        const int32 StartZ = ThreadIndex * ChunkSize;
        const int32 EndZ = (ThreadIndex == NumThreads-1) ? 
            VoxModel->ModelDimensions.Z : 
            FMath::Min(StartZ + ChunkSize, VoxModel->ModelDimensions.Z);

        FVoxMeshData LocalChunkData;
        BuildGreedyMeshChunk(StartZ, EndZ, LocalChunkData);

        FScopeLock Lock(&ChunkResults[ThreadIndex].Lock);
        ChunkResults[ThreadIndex].Data = MoveTemp(LocalChunkData);
    });

    int32 TotalVertices = 0;
    for (const FChunkResult& Result : ChunkResults)
    {
        TotalVertices += Result.Data.Vertices.Num();
    }

    VoxMeshData.Vertices.Reserve(TotalVertices);
    VoxMeshData.Triangles.Reserve(TotalVertices * 1.5);
    VoxMeshData.Normals.Reserve(TotalVertices);
    VoxMeshData.UV0.Reserve(TotalVertices);

    for (FChunkResult& Result : ChunkResults)
    {
        const int32 VertexOffset = VoxMeshData.Vertices.Num();
        
        VoxMeshData.Vertices.Append(Result.Data.Vertices);
        VoxMeshData.Normals.Append(Result.Data.Normals);
        VoxMeshData.UV0.Append(Result.Data.UV0);
        
        for (int32 Index : Result.Data.Triangles)
        {
            VoxMeshData.Triangles.Add(Index + VertexOffset);
        }
    }
}

void AVoxMeshThread::BuildGreedyMeshChunk(int32 StartZ, int32 EndZ, FVoxMeshData& ChunkData)
{
    struct FaceMask {
        uint8 Active : 1;
        uint8 BlockType : 7;
    };

    static const FIntVector Normals[] = {
        FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
        FIntVector(0, 1, 0), FIntVector(0, -1, 0),
        FIntVector(0, 0, 1), FIntVector(0, 0, -1)
    };

    const FIntVector& Dims = VoxModel->ModelDimensions;
    const float Scale = VoxModel->VoxelSize;
    const FVector CenterOffset = FVector(
        -Dims.X * Scale * 0.5f,
        -Dims.Y * Scale * 0.5f,
        0.0f
    );

    const int32 EstimatedVertices = (EndZ - StartZ) * Dims.X * Dims.Y;
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> MeshNormals;
    TArray<FVector2D> UVs;
    
    Vertices.Reserve(EstimatedVertices * 4);
    Triangles.Reserve(EstimatedVertices * 6);
    MeshNormals.Reserve(EstimatedVertices * 4);
    UVs.Reserve(EstimatedVertices * 4);

    const int32 DimXY = Dims.X * Dims.Y;
    TArray<FaceMask> Mask;

    for (int32 Dir = 0; Dir < 6; Dir++)
    {
        const FIntVector Normal = Normals[Dir];
        const int32 Dim1 = Dims[(Dir / 2 + 1) % 3];
        const int32 Dim2 = Dims[(Dir / 2 + 2) % 3];
        const int32 MaskSize = Dim1 * Dim2;

        Mask.Empty(MaskSize);
        Mask.AddZeroed(MaskSize);

        const int32 StartD = (Dir / 2 == 2) ? StartZ : 0;
        const int32 EndD = (Dir / 2 == 2) ? EndZ : Dims[Dir / 2];

        for (int32 D = StartD; D < EndD; D++)
        {
            for (int32 V1 = 0; V1 < Dim1; V1++)
            {
                for (int32 V2 = 0; V2 < Dim2; V2++)
                {
                    FIntVector Pos;
                    Pos[Dir / 2] = D;
                    Pos[(Dir / 2 + 1) % 3] = V1;
                    Pos[(Dir / 2 + 2) % 3] = V2;

                    if (Dir / 2 != 2 && (Pos.Z < StartZ || Pos.Z >= EndZ))
                        continue;

                    const int32 Index = V2 + V1 * Dim2;
                    if (!Mask.IsValidIndex(Index)) continue;

                    const int32 BlockIndex = Pos.X + Pos.Y * Dims.X + Pos.Z * DimXY;
                    if (!VoxModel->Blocks.IsValidIndex(BlockIndex)) continue;

                    const EBlock CurrentBlock = VoxModel->Blocks[BlockIndex];
                    FIntVector NextPos = Pos + Normal;
                    
                    const bool InBounds = 
                        NextPos.X >= 0 && NextPos.X < Dims.X &&
                        NextPos.Y >= 0 && NextPos.Y < Dims.Y &&
                        NextPos.Z >= 0 && NextPos.Z < Dims.Z;

                    const EBlock NextBlock = InBounds
                        ? VoxModel->Blocks[NextPos.X + NextPos.Y * Dims.X + NextPos.Z * DimXY]
                        : EBlock::Air;

                    Mask[Index].Active = (CurrentBlock != EBlock::Air && NextBlock == EBlock::Air) ||
                        (CurrentBlock == EBlock::Air && NextBlock != EBlock::Air);
                    Mask[Index].BlockType = CurrentBlock != EBlock::Air ? 
                        static_cast<uint8>(CurrentBlock) : static_cast<uint8>(NextBlock);
                }
            }

            for (int32 V1 = 0; V1 < Dim1; V1++)
            {
                for (int32 V2 = 0; V2 < Dim2;)
                {
                    const int32 Index = V2 + V1 * Dim2;
                    if (!Mask.IsValidIndex(Index) || !Mask[Index].Active)
                    {
                        V2++;
                        continue;
                    }

                    int32 Width = 1;
                    const uint8 BlockType = Mask[Index].BlockType;
                    while (V2 + Width < Dim2 &&
                        Mask[V2 + Width + V1 * Dim2].Active &&
                        Mask[V2 + Width + V1 * Dim2].BlockType == BlockType)
                    {
                        Width++;
                    }

                    int32 Height = 1;
                    bool Done = false;
                    while (V1 + Height < Dim1 && !Done)
                    {
                        for (int32 W = 0; W < Width; W++)
                        {
                            const int32 MaskIndex = V2 + W + (V1 + Height) * Dim2;
                            if (!Mask.IsValidIndex(MaskIndex) ||
                                !Mask[MaskIndex].Active ||
                                Mask[MaskIndex].BlockType != BlockType)
                            {
                                Done = true;
                                break;
                            }
                        }
                        if (!Done) Height++;
                    }

                    FVector BasePos;
                    BasePos[Dir / 2] = D * Scale;
                    BasePos[(Dir / 2 + 1) % 3] = V1 * Scale;
                    BasePos[(Dir / 2 + 2) % 3] = V2 * Scale;

                    FVector Size;
                    Size[(Dir / 2 + 1) % 3] = Height * Scale;
                    Size[(Dir / 2 + 2) % 3] = Width * Scale;

                    const int32 StartVertex = Vertices.Num();
                    GenerateFaceVertices(Dir, BasePos, Size, Scale, CenterOffset, Vertices);
                    GenerateFaceIndices(Dir, StartVertex, Triangles);
                    
                    for (int32 i = 0; i < 4; i++)
                    {
                        MeshNormals.Add(FVector(Normal));
                        UVs.Add(FVector2D(i < 2 ? 0 : Width, i % 2 == 0 ? 0 : Height));
                    }

                    for (int32 H = 0; H < Height; H++)
                    {
                        for (int32 W = 0; W < Width; W++)
                        {
                            const int32 MaskIndex = V2 + W + (V1 + H) * Dim2;
                            if (Mask.IsValidIndex(MaskIndex))
                            {
                                Mask[MaskIndex].Active = 0;
                            }
                        }
                    }

                    V2 += Width;
                }
            }
        }
    }

    ChunkData.Vertices = MoveTemp(Vertices);
    ChunkData.Triangles = MoveTemp(Triangles);
    ChunkData.Normals = MoveTemp(MeshNormals);
    ChunkData.UV0 = MoveTemp(UVs);
}

void AVoxMeshThread::GenerateFaceVertices(int32 Dir, const FVector& BasePos, const FVector& Size, 
    float Scale, const FVector& CenterOffset, TArray<FVector>& Vertices)
{
    switch (Dir)
    {
    case 0: // Right
        Vertices.Add(BasePos + FVector(Scale, 0.0f, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(Scale, Size.Y, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(Scale, 0.0f, Size.Z) + CenterOffset);
        Vertices.Add(BasePos + FVector(Scale, Size.Y, Size.Z) + CenterOffset);
        break;
    case 1: // Left
        Vertices.Add(BasePos + CenterOffset);
        Vertices.Add(BasePos + FVector(0.0f, Size.Y, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(0.0f, 0.0f, Size.Z) + CenterOffset);
        Vertices.Add(BasePos + FVector(0.0f, Size.Y, Size.Z) + CenterOffset);
        break;
    case 2: // Forward
        Vertices.Add(BasePos + FVector(0.0f, Scale, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, Scale, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(0.0f, Scale, Size.Z) + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, Scale, Size.Z) + CenterOffset);
        break;
    case 3: // Back
        Vertices.Add(BasePos + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, 0.0f, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(0.0f, 0.0f, Size.Z) + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, 0.0f, Size.Z) + CenterOffset);
        break;
    case 4: // Up
        Vertices.Add(BasePos + FVector(0.0f, 0.0f, Scale) + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, 0.0f, Scale) + CenterOffset);
        Vertices.Add(BasePos + FVector(0.0f, Size.Y, Scale) + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, Size.Y, Scale) + CenterOffset);
        break;
    case 5: // Down
        Vertices.Add(BasePos + CenterOffset);
        Vertices.Add(BasePos + FVector(0.0f, Size.Y, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, 0.0f, 0.0f) + CenterOffset);
        Vertices.Add(BasePos + FVector(Size.X, Size.Y, 0.0f) + CenterOffset);
        break;
    }
}

void AVoxMeshThread::GenerateFaceIndices(int32 Dir, int32 StartVertex, TArray<int32>& Triangles)
{
    if (Dir == 5) // Bottom faces
    {
        Triangles.Add(StartVertex);
        Triangles.Add(StartVertex + 2);
        Triangles.Add(StartVertex + 1);
        Triangles.Add(StartVertex + 1);
        Triangles.Add(StartVertex + 2);
        Triangles.Add(StartVertex + 3);
    }
    else if (Dir == 1 || Dir == 3) // Left and Back faces
    {
        Triangles.Add(StartVertex);
        Triangles.Add(StartVertex + 1);
        Triangles.Add(StartVertex + 2);
        Triangles.Add(StartVertex + 1);
        Triangles.Add(StartVertex + 3);
        Triangles.Add(StartVertex + 2);
    }
    else // Right, Forward, Up faces
    {
        Triangles.Add(StartVertex);
        Triangles.Add(StartVertex + 2);
        Triangles.Add(StartVertex + 1);
        Triangles.Add(StartVertex + 1);
        Triangles.Add(StartVertex + 2);
        Triangles.Add(StartVertex + 3);
    }
}

void AVoxMeshThread::VisualizeChunks()
{
    const int32 NumThreads = FPlatformMisc::NumberOfCores();
    const int32 ChunkSize = FMath::Max(1, VoxModel->ModelDimensions.Z / NumThreads);
    const float Scale = VoxModel->VoxelSize;
    
    // Calculate model bounds
    const FVector ModelExtent(
        VoxModel->ModelDimensions.X * Scale,
        VoxModel->ModelDimensions.Y * Scale,
        VoxModel->ModelDimensions.Z * Scale
    );
    
    const FVector ModelOrigin = VoxModel->GetActorLocation() + FVector(
        -ModelExtent.X * 0.5f,
        -ModelExtent.Y * 0.5f,
        0.0f
    );

    // Draw chunk boundaries
    for (int32 ThreadIndex = 0; ThreadIndex < NumThreads; ThreadIndex++)
    {
        const int32 StartZ = ThreadIndex * ChunkSize;
        const int32 EndZ = (ThreadIndex == NumThreads-1) ? 
            VoxModel->ModelDimensions.Z : 
            FMath::Min(StartZ + ChunkSize, VoxModel->ModelDimensions.Z);

        FVector ChunkMin = ModelOrigin + FVector(0, 0, StartZ * Scale);
        FVector ChunkMax = ModelOrigin + FVector(ModelExtent.X, ModelExtent.Y, EndZ * Scale);

        // Different colors for each chunk
        const FColor ChunkColors[] = {
            FColor::Red,
            FColor::Green,
            FColor::Blue,
            FColor::Yellow,
            FColor::Magenta,
            FColor::Cyan,
            FColor::Orange,
            FColor::Purple
        };

        FColor ChunkColor = ChunkColors[ThreadIndex % 8];

        // Draw chunk bounds
        DrawDebugBox(
            VoxModel->GetWorld(),
            (ChunkMin + ChunkMax) * 0.5f, // Box center
            (ChunkMax - ChunkMin) * 0.5f, // Box extent
            FQuat::Identity,
            ChunkColor,
            true,  // Persistent
            5.0f,  // Duration
            0,     // DepthPriority
            2.0f   // Thickness
        );
    }
}


void AVoxMeshThread::Exit()
{
	VoxModel->ApplyMesh();
	FRunnable::Exit();
}
