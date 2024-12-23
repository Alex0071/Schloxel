#include "VoxImporter.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"

bool UVoxImporter::LoadVoxFile(const FString& FilePath)
{
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
	{
		return false;
	}

	const ogt_vox_scene* Scene = ogt_vox_read_scene(FileData.GetData(), FileData.Num());
	if (!Scene || Scene->num_models == 0)
	{
		return false;
	}

	const ogt_vox_model* Model = Scene->models[0];
	ModelDimensions = FIntVector(Model->size_x, Model->size_y, Model->size_z);

	const int32 TotalVoxels = ModelDimensions.X * ModelDimensions.Y * ModelDimensions.Z;
	VoxelData.SetNum(TotalVoxels, EAllowShrinking::No);

	// Initialize all to Air
	for (int32 i = 0; i < TotalVoxels; i++)
	{
		VoxelData[i] = EBlock::Air;
	}

	// Fill in the solid voxels
	const uint8_t* VoxelPtr = Model->voxel_data;
	for (uint32 z = 0; z < static_cast<uint32>(Model->size_z); z++)
	{
		for (uint32 y = 0; y < static_cast<uint32>(Model->size_y); y++)
		{
			for (uint32 x = 0; x < static_cast<uint32>(Model->size_x); x++)
			{
				if (*VoxelPtr != 0)
				{
					const int32 Index = x + y * ModelDimensions.X + z * ModelDimensions.X * ModelDimensions.Y;
					VoxelData[Index] = EBlock::Stone;
				}
				VoxelPtr++;
			}
		}
	}

	ogt_vox_destroy_scene(Scene);
	return true;
}

TArray<EBlock> UVoxImporter::GetVoxelData() const
{
	return VoxelData;
}

FIntVector UVoxImporter::GetModelSize() const
{
	return ModelDimensions;
}
