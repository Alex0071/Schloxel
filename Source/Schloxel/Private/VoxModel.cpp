#include "VoxModel.h"

#include "AVoxMeshThread.h"
#include "VoxImporter.h"
#include "RealtimeMeshSimple.h"

AVoxModel::AVoxModel()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<URealtimeMeshComponent>("VoxMesh");
	SetRootComponent(MeshComponent);
	MeshComponent->SetCastShadow(true);
	MeshComponent->SetMobility(EComponentMobility::Static);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
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

					if (index >= 0 && index < Blocks.Num())
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
				URealtimeMeshSimple* RealtimeMesh = MeshComponent->InitializeRealtimeMesh<URealtimeMeshSimple>();

				RealtimeMesh::FRealtimeMeshStreamSet StreamSet;
				RealtimeMesh::TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

				Builder.EnableTangents();
				Builder.EnableTexCoords();
				Builder.EnableColors();
				Builder.EnablePolyGroups();

				for (int i = 0; i < data.Vertices.Num(); i++)
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
					0, FName("VoxMesh"));
				const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);

				RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet,
				                                 FRealtimeMeshSectionGroupConfig(ERealtimeMeshSectionDrawType::Static));
				RealtimeMesh->UpdateSectionConfig(SectionKey, FRealtimeMeshSectionConfig(0), true);
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
