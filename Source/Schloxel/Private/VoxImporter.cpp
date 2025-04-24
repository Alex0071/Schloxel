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


	FIntVector MinBounds(MAX_int32, MAX_int32, MAX_int32);
	FIntVector MaxBounds(MIN_int32, MIN_int32, MIN_int32);

	for (uint32 i = 0; i < Scene->num_instances; i++)
	{
		const ogt_vox_instance& Instance = Scene->instances[i];
		const ogt_vox_model* Model = Scene->models[Instance.model_index];


		FVector Position(Instance.transform.m30, Instance.transform.m31, Instance.transform.m32);


		MinBounds.X = FMath::Min(MinBounds.X, FMath::FloorToInt(Position.X));
		MinBounds.Y = FMath::Min(MinBounds.Y, FMath::FloorToInt(Position.Y));
		MinBounds.Z = FMath::Min(MinBounds.Z, FMath::FloorToInt(Position.Z));

		MaxBounds.X = FMath::Max(MaxBounds.X, FMath::CeilToInt(Position.X + Model->size_x));
		MaxBounds.Y = FMath::Max(MaxBounds.Y, FMath::CeilToInt(Position.Y + Model->size_y));
		MaxBounds.Z = FMath::Max(MaxBounds.Z, FMath::CeilToInt(Position.Z + Model->size_z));
	}


	ModelDimensions = MaxBounds - MinBounds;
	const int32 TotalVoxels = ModelDimensions.X * ModelDimensions.Y * ModelDimensions.Z;
	VoxelData.SetNum(TotalVoxels, EAllowShrinking::No);

	// Initialize all to Air
	for (int32 i = 0; i < TotalVoxels; i++)
	{
		VoxelData[i] = EBlock::Air;
	}


	for (uint32 i = 0; i < Scene->num_instances; i++)
	{
		const ogt_vox_instance& Instance = Scene->instances[i];
		const ogt_vox_model* Model = Scene->models[Instance.model_index];

		// Extract rotation matrix
		FMatrix RotationMatrix(
			FVector(Instance.transform.m00, Instance.transform.m01, Instance.transform.m02),
			FVector(Instance.transform.m10, Instance.transform.m11, Instance.transform.m12),
			FVector(Instance.transform.m20, Instance.transform.m21, Instance.transform.m22),
			FVector::ZeroVector
		);

		FVector Position(Instance.transform.m30, Instance.transform.m31, Instance.transform.m32);
		Position -= FVector(MinBounds);


		for (uint32 z = 0; z < Model->size_z; z++)
		{
			for (uint32 y = 0; y < Model->size_y; y++)
			{
				for (uint32 x = 0; x < Model->size_x; x++)
				{
					const uint8_t VoxelValue = Model->voxel_data[x + y * Model->size_x + z * Model->size_x * Model->
						size_y];
					if (VoxelValue != 0)
					{
						FVector LocalPos(x, y, z);
						FVector RotatedPos = RotationMatrix.TransformVector(LocalPos);

						const int32 DestX = FMath::FloorToInt(Position.X + RotatedPos.X);
						const int32 DestY = FMath::FloorToInt(Position.Y + RotatedPos.Y);
						const int32 DestZ = FMath::FloorToInt(Position.Z + RotatedPos.Z);

						if (DestX >= 0 && DestX < ModelDimensions.X &&
							DestY >= 0 && DestY < ModelDimensions.Y &&
							DestZ >= 0 && DestZ < ModelDimensions.Z)
						{
							const int32 Index = DestX + DestY * ModelDimensions.X + DestZ * ModelDimensions.X *
								ModelDimensions.Y;
							VoxelData[Index] = EBlock::Stone;
						}
					}
				}
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
