#include "VoxModel.h"
#include "VoxImporter.h"
#include "ProceduralMeshComponent.h"

AVoxModel::AVoxModel()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>("VoxMesh");
    SetRootComponent(MeshComponent);
    MeshComponent->SetCastShadow(true);
    MeshComponent->SetMobility(EComponentMobility::Static);
}

void AVoxModel::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    if (!VoxFilePath.IsEmpty())
    {
        UVoxImporter* Importer = NewObject<UVoxImporter>();
        if (Importer->LoadVoxFile(VoxFilePath))
        {
            BuildGreedyMesh(Importer->GetVoxelData(), Importer->GetModelSize());
        }
    }
}

void AVoxModel::BuildGreedyMesh(const TArray<EBlock>& Blocks, const FIntVector& Dimensions)
{
    struct FaceMask 
    {
        bool Active;
        EBlock BlockType;
    };

    const FIntVector Normals[] = {
        FIntVector(1,0,0), FIntVector(-1,0,0),
        FIntVector(0,1,0), FIntVector(0,-1,0),
        FIntVector(0,0,1), FIntVector(0,0,-1)
    };

    const int32 EstimatedVertices = Dimensions.X * Dimensions.Y * Dimensions.Z;
    TArray<FVector> Vertices; Vertices.Reserve(EstimatedVertices * 4);
    TArray<int32> Triangles; Triangles.Reserve(EstimatedVertices * 6);
    TArray<FVector> MeshNormals; MeshNormals.Reserve(EstimatedVertices * 4);
    TArray<FVector2D> UVs; UVs.Reserve(EstimatedVertices * 4);

    const int32 DimXY = Dimensions.X * Dimensions.Y;
    TArray<FaceMask> Mask;
    const float VScale = VoxelScale;

    for (int32 Dir = 0; Dir < 6; Dir++)
    {
        const FIntVector Normal = Normals[Dir];
        const int32 Dim1 = Dimensions[(Dir/2 + 1)%3];
        const int32 Dim2 = Dimensions[(Dir/2 + 2)%3];
        const int32 MaskSize = Dim1 * Dim2;
        
        Mask.Reset();
        Mask.SetNum(MaskSize);

        for (int32 D = 0; D < Dimensions[Dir/2]; D++)
        {
            for (int32 V1 = 0; V1 < Dim1; V1++)
            {
                for (int32 V2 = 0; V2 < Dim2; V2++)
                {
                    FIntVector Pos = FIntVector::ZeroValue;
                    Pos[Dir/2] = D;
                    Pos[(Dir/2 + 1)%3] = V1;
                    Pos[(Dir/2 + 2)%3] = V2;

                    const int32 Index = V2 + V1 * Dim2;
                    if (!Mask.IsValidIndex(Index)) continue;

                    const int32 BlockIndex = Pos.X + Pos.Y * Dimensions.X + Pos.Z * DimXY;
                    if (!Blocks.IsValidIndex(BlockIndex)) continue;

                    const EBlock CurrentBlock = Blocks[BlockIndex];
                    
                    FIntVector NextPos = Pos + Normal;
                    const bool InBounds = NextPos.X >= 0 && NextPos.X < Dimensions.X &&
                                        NextPos.Y >= 0 && NextPos.Y < Dimensions.Y &&
                                        NextPos.Z >= 0 && NextPos.Z < Dimensions.Z;
                    
                    const EBlock NextBlock = InBounds ? 
                        Blocks[NextPos.X + NextPos.Y * Dimensions.X + NextPos.Z * DimXY] : 
                        EBlock::Air;

                    Mask[Index].Active = (CurrentBlock != EBlock::Air && NextBlock == EBlock::Air) || 
                                       (CurrentBlock == EBlock::Air && NextBlock != EBlock::Air);
                    Mask[Index].BlockType = CurrentBlock != EBlock::Air ? CurrentBlock : NextBlock;
                }
            }

            for (int32 V1 = 0; V1 < Dim1; V1++)
            {
                for (int32 V2 = 0; V2 < Dim2;)
                {
                    const int32 Index = V2 + V1 * Dim2;
                    if (!Mask.IsValidIndex(Index))
                    {
                        V2++;
                        continue;
                    }

                    if (Mask[Index].Active)
                    {
                        int32 Width = 1;
                        const EBlock BlockType = Mask[Index].BlockType;
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
                        BasePos[Dir/2] = D * VScale;
                        BasePos[(Dir/2 + 1)%3] = V1 * VScale;
                        BasePos[(Dir/2 + 2)%3] = V2 * VScale;

                        FVector Size;
                        Size[(Dir/2 + 1)%3] = Height * VScale;
                        Size[(Dir/2 + 2)%3] = Width * VScale;

                        const int32 StartVertex = Vertices.Num();

                        switch (Dir)
                        {
                        case 0: // Right
                            Vertices.Add(BasePos + FVector(VScale, 0.0f, 0.0f));
                            Vertices.Add(BasePos + FVector(VScale, Size.Y, 0.0f));
                            Vertices.Add(BasePos + FVector(VScale, 0.0f, Size.Z));
                            Vertices.Add(BasePos + FVector(VScale, Size.Y, Size.Z));
                            break;
                            
                        case 1: // Left
                            Vertices.Add(BasePos);
                            Vertices.Add(BasePos + FVector(0.0f, Size.Y, 0.0f));
                            Vertices.Add(BasePos + FVector(0.0f, 0.0f, Size.Z));
                            Vertices.Add(BasePos + FVector(0.0f, Size.Y, Size.Z));
                            break;
                            
                        case 2: // Forward
                            Vertices.Add(BasePos + FVector(0.0f, VScale, 0.0f));
                            Vertices.Add(BasePos + FVector(Size.X, VScale, 0.0f));
                            Vertices.Add(BasePos + FVector(0.0f, VScale, Size.Z));
                            Vertices.Add(BasePos + FVector(Size.X, VScale, Size.Z));
                            break;
                            
                        case 3: // Back
                            Vertices.Add(BasePos);
                            Vertices.Add(BasePos + FVector(Size.X, 0.0f, 0.0f));
                            Vertices.Add(BasePos + FVector(0.0f, 0.0f, Size.Z));
                            Vertices.Add(BasePos + FVector(Size.X, 0.0f, Size.Z));
                            break;
                            
                        case 4: // Up
                            Vertices.Add(BasePos + FVector(0.0f, 0.0f, VScale));
                            Vertices.Add(BasePos + FVector(Size.X, 0.0f, VScale));
                            Vertices.Add(BasePos + FVector(0.0f, Size.Y, VScale));
                            Vertices.Add(BasePos + FVector(Size.X, Size.Y, VScale));
                            break;
                            
                        case 5: // Down
                            Vertices.Add(BasePos);
                            Vertices.Add(BasePos + FVector(0.0f, Size.Y, 0.0f));
                            Vertices.Add(BasePos + FVector(Size.X, 0.0f, 0.0f));
                            Vertices.Add(BasePos + FVector(Size.X, Size.Y, 0.0f));
                            break;
                        }

                        if (Dir == 5) // Bottom faces need different winding
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
                                    Mask[MaskIndex].Active = false;
                                }
                            }
                        }

                        V2 += Width;
                    }
                    else
                    {
                        V2++;
                    }
                }
            }
        }
    }

    MeshComponent->CreateMeshSection(0, Vertices, Triangles, MeshNormals, UVs, 
        TArray<FColor>(), TArray<FProcMeshTangent>(), true);
    MeshComponent->SetMaterial(0, Material);
}
