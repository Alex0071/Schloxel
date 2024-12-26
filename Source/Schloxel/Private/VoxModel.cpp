#include "VoxModel.h"

#include "AVoxMeshThread.h"
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
	ClearMeshData();
	LoadVoxModel();
}

void AVoxModel::BeginPlay()
{
	Super::BeginPlay();
	ClearMeshData();
	LoadVoxModel();
}

void AVoxModel::LoadVoxModel()
{
	if (!VoxFilePath.IsEmpty())
	{
		UVoxImporter* Importer = NewObject<UVoxImporter>();
		if (Importer->LoadVoxFile(VoxFilePath))
		{
			ModelDimensions = Importer->GetModelSize();
			Blocks = Importer->GetVoxelData();
			GenerateMesh();
		}
	}
}

void AVoxModel::GenerateMesh()
{
	new AVoxMeshThread(this);
}

void AVoxModel::ModifyVoxel(const FIntVector Position, const EBlock Block)
{
	if (Position.X >= ModelDimensions.X || Position.Y >= ModelDimensions.Y ||
		Position.Z >= ModelDimensions.Z || Position.X < 0 || Position.Y < 0 || Position.Z < 0)
		return;

	constexpr int Radius = 8;

	for (int x = -Radius + 1; x <= Radius - 1; ++x)
	{
		for (int y = -Radius + 1; y <= Radius - 1; ++y)
		{
			for (int z = -Radius + 1; z <= Radius - 1; ++z)
			{
				const FIntVector CurrentPos(Position.X + x, Position.Y + y, Position.Z + z);

				if (CurrentPos.X >= ModelDimensions.X || CurrentPos.Y >= ModelDimensions.Y ||
					CurrentPos.Z >= ModelDimensions.Z || CurrentPos.X < 0 || CurrentPos.Y < 0 || CurrentPos.Z < 0)
					continue;

				float distance = FVector::Dist(FVector(x, y, z), FVector::ZeroVector);
				if (distance <= Radius)
				{
					const int index = CurrentPos.X + CurrentPos.Y * ModelDimensions.X +
						CurrentPos.Z * (ModelDimensions.X * ModelDimensions.Y);
					Blocks[index] = Block;
				}
			}
		}
	}

	GenerateMesh();
}


void AVoxModel::ApplyMesh()
{
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		while (!MeshDataQueue.IsEmpty())
		{
			TQueue<FVoxMeshData>::FElementType data;
			if (MeshDataQueue.Dequeue(data))
			{
				MeshComponent->CreateMeshSection(0, data.Vertices, data.Triangles, data.Normals, data.UV0,
				                                 TArray<FColor>(), TArray<FProcMeshTangent>(), true);
				MeshComponent->SetMaterial(0, Material);
			}
		}
	});
}


void AVoxModel::ClearMeshData()
{
	Blocks.Empty();
	ModelDimensions = FIntVector::ZeroValue;
	
	MeshDataQueue.Empty();
}
