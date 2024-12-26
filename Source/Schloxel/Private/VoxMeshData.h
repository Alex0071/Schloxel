#pragma once

#include "CoreMinimal.h"
#include "VoxMeshData.generated.h"

USTRUCT()
struct FVoxMeshData
{
	GENERATED_BODY()
	;


	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
};