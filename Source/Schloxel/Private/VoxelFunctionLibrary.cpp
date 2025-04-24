// Fill out your copyright notice in the Description page of Project Settings.


#include "VoxelFunctionLibrary.h"

FIntVector UVoxelFunctionLibrary::WorldToBlockPosition(const FVector& Position)
{
	return FIntVector(Position) / 10; // 10 is the voxel size
}

FIntVector UVoxelFunctionLibrary::WorldToLocalBlockPosition(const FVector& Position, const FIntVector Size)
{
	const auto ChunkPosition = WorldToChunkPosition(Position, Size);

	auto Result = WorldToBlockPosition(Position) - ChunkPosition * Size;

	// Negative Normalization
	if (ChunkPosition.X < 0) Result.X--;
	if (ChunkPosition.Y < 0) Result.Y--;
	if (ChunkPosition.Z < 0) Result.Z--;

	return Result;
}

FIntVector UVoxelFunctionLibrary::WorldToChunkPosition(const FVector& Position, const FIntVector Size)
{
	FIntVector Result;

	const FIntVector Factor = Size * 10; 
	const auto IntPosition = FIntVector(Position);

	if (IntPosition.X < 0) Result.X = static_cast<int>(Position.X / Factor.X) - 1;
	else Result.X = static_cast<int>(Position.X / Factor.X);

	if (IntPosition.X < 0) Result.Y = static_cast<int>(Position.Y / Factor.Y) - 1;
	else Result.Y = static_cast<int>(Position.Y / Factor.Y);

	if (IntPosition.Z < 0) Result.Z = static_cast<int>(Position.Z / Factor.Z) - 1;
	else Result.Z = static_cast<int>(Position.Z / Factor.Z);

	return Result;
}
