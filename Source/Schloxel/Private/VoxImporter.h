﻿#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "VoxImporter.generated.h"

UCLASS()
class UVoxImporter : public UObject
{
	GENERATED_BODY()

public:
	bool LoadVoxFile(const FString& FilePath);
	TArray<EBlock> GetVoxelData() const;
	FIntVector GetModelSize() const;
	
	TArray<TArray<EBlock>> VoxelDataArray;
	TArray<FIntVector> ModelDimensionsArray;

	
private:
	TArray<EBlock> VoxelData;
	FIntVector ModelDimensions;
};
