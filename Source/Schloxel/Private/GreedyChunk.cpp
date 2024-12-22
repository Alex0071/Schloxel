#include "GreedyChunk.h"
#include "ChunkWorld.h"
#include "Enums.h"
#include "MeshThread.h"
#include "Engine/Texture2D.h"
#include "PixelFormat.h"
#include "Math/UnrealMathUtility.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSimple.h"

// Sets default values
AGreedyChunk::AGreedyChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<URealtimeMeshComponent>("Mesh");
	SetRootComponent(Mesh);
	Mesh->SetCastShadow(true);
	Mesh->SetMobility(EComponentMobility::Movable);
	// Enable collision for the mesh section
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

	// Ensure the mesh component is set to receive dynamic lighting
	Mesh->bAffectDynamicIndirectLighting = true;
	Mesh->bAffectDistanceFieldLighting = true;
}

void AGreedyChunk::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Blocks.SetNum(ChunkSize.X * ChunkSize.Y * ChunkSize.Z);

	GenerateBlocks();
	GenerateMesh();
}

void AGreedyChunk::BeginPlay()
{
	Super::BeginPlay();

	Blocks.SetNum(ChunkSize.X * ChunkSize.Y * ChunkSize.Z);

	ClearMesh();
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

			const float Height = GetPrecomputedPixelBrightness(Xpos, Ypos);

			int32 HeightInt = FMath::Clamp(FMath::RoundToInt(Height), 0, ChunkSize.Z);

			for (int z = 0; z < HeightInt; z++)
			{
				Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
			}

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

	if (int* Brightness = CachedBrightnessMap->Find(PixelCoords))
	{
		return *Brightness;
	}

	UE_LOG(LogTemp, Warning, TEXT("Pixel (%d, %d) not found in CachedBrightnessMap!"), X, Y);
	return 0.0f;
}

void AGreedyChunk::ApplyMesh()
{
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		while (!MeshDataQueue.IsEmpty())
		{
			TQueue<FChunkMeshData>::FElementType data;
			if (MeshDataQueue.Dequeue(data))
			{
				URealtimeMeshSimple* RealtimeMesh = Mesh->InitializeRealtimeMesh<URealtimeMeshSimple>();

				FRealtimeMeshStreamSet StreamSet;
				TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

				Builder.EnableTangents();
				Builder.EnableTexCoords();
				Builder.EnableColors();
				Builder.EnablePolyGroups();

				for (int32 i = 0; i < data.Vertices.Num(); ++i)
				{
					int32 VertexIndex = Builder.AddVertex(FVector3f(data.Vertices[i]))
					                           .SetNormalAndTangent(FVector3f(data.Normals[i]),
					                                                FVector3f(0, 0, 0))
					                           .SetTexCoord(FVector2f(data.UV0[i]));
				}

				for (int32 i = 0; i < data.Triangles.Num(); i += 3)
				{
					Builder.AddTriangle(data.Triangles[i], data.Triangles[i + 1], data.Triangles[i + 2], 0);
				}

				RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial", Material);

				const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(
					0, FName("ChunkMesh"));
				const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);

				RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet,
				                                 FRealtimeMeshSectionGroupConfig(ERealtimeMeshSectionDrawType::Static));
				RealtimeMesh->UpdateSectionConfig(SectionKey, FRealtimeMeshSectionConfig(0), true);
			}
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
	MeshDataQueue.Empty();
}
