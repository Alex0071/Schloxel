#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "VoxMeshData.h"
#include "VoxModel.h"

class AVoxMeshThread : FRunnable
{
public:
	AVoxMeshThread(AVoxModel* _VoxModel);

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;
	void VisualizeChunks();

private:
	FVoxMeshData VoxMeshData;
	AVoxModel* VoxModel;

	// Main mesh generation methods
	void BuildGreedyMeshParallel();

	void BuildGreedyMeshChunk(int32 StartZ, int32 EndZ, FVoxMeshData& ChunkData);

	// Helper methods for geometry generation
	void GenerateFaceVertices(int32 Dir, const FVector& BasePos, const FVector& Size, 
		float Scale, const FVector& CenterOffset, TArray<FVector>& Vertices);
	void GenerateFaceIndices(int32 Dir, int32 StartVertex, TArray<int32>& Triangles);
};
