// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkWorld.h"

#include "GreedyChunk.h"
#include "GreedyChunkSlow.h"
#include "MyUserWidget.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AChunkWorld::AChunkWorld()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AChunkWorld::BeginPlay()
{
	Super::BeginPlay();

	if (!HeightMap)
	{
		UE_LOG(LogTemp, Error, TEXT("HeightMap is null!"));
		return;
	}

	HeightMap->WaitForStreaming();

	PrecomputeBrightness(HeightMap);

	//UTexture2D* GeneratedHeightmapTexture = CreateHeightmapTextureFromBrightness();
	//ShowHeightmap(GeneratedHeightmapTexture);


	SpawnChunks();
}

void AChunkWorld::SpawnChunks()
{
	for (int x = 0; x < DrawDistance; x++)
	{
		for (int y = 0; y < DrawDistance; y++)
		{
			auto transform = FTransform(
				FRotator::ZeroRotator,
				FVector(x * ChunkSize.X * VoxelSize, y * ChunkSize.Y * VoxelSize, 0),
				FVector::OneVector
			);
			if (Cast<AGreedyChunk>(Chunk.GetDefaultObject()))
			{
				const auto chunk = GetWorld()->SpawnActorDeferred<AGreedyChunk>(Chunk, transform, this);

				chunk->Material = Material;
				chunk->ChunkSize = ChunkSize;
				chunk->VoxelSize = VoxelSize;
				chunk->CachedBrightnessMap = &CachedBrightnessMap;

				UGameplayStatics::FinishSpawningActor(chunk, transform);
			}
			if (Cast<AGreedyChunkSlow>(Chunk.GetDefaultObject()))
			{
				const auto chunk = GetWorld()->SpawnActorDeferred<AGreedyChunkSlow>(Chunk, transform, this);

				chunk->Material = Material;
				chunk->ChunkSize = ChunkSize;
				chunk->VoxelSize = VoxelSize;

				UGameplayStatics::FinishSpawningActor(chunk, transform);
			}
		}
	}
}

void AChunkWorld::PrecomputeBrightness(UTexture2D* Texture)
{
	if (!Texture || !Texture->GetPlatformData())
		return;

	FTexture2DMipMap* MipMap = &Texture->GetPlatformData()->Mips[0];
	if (!MipMap)
		return;

	FByteBulkData* RawImageData = &MipMap->BulkData;
	void* RawData = RawImageData->Lock(LOCK_READ_ONLY);

	uint8* PixelData = static_cast<uint8*>(RawData);
	int32 Width = MipMap->SizeX;
	int32 Height = MipMap->SizeY;


	// Loop over each pixel and compute brightness
	for (int32 y = 0; y < Height; ++y)
	{
		for (int32 x = 0; x < Width; ++x)
		{
			int32 Index = (y * Width + x) * 4; // RGBA
			uint8 B = PixelData[Index + 0];
			uint8 G = PixelData[Index + 1];
			uint8 R = PixelData[Index + 2];

			// Calculate brightness using the standard formula for luminance
			float Brightness = (0.2126f * R + 0.7152f * G + 0.0722f * B);

			// Map brightness from 0-255 to a more reasonable voxel height range (e.g., 0-128)
			// Adjust this scale as necessary for your needs (height should be less than ChunkSize.Z)


			// Store the brightness value
			CachedBrightnessMap.Add(FIntPoint(x, y), Brightness);
		}
	}

	RawImageData->Unlock();
}

UTexture2D* AChunkWorld::CreateHeightmapTextureFromBrightness()
{
	const int32 Width = 1024; // Heightmap width
	const int32 Height = 1024; // Heightmap height

	// Create a new texture
	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height);
	if (!NewTexture)
		return nullptr;

	// Lock the texture to get access to its pixel data
	FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
	uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));

	// Iterate over each pixel
	for (int32 y = 0; y < Height; ++y)
	{
		for (int32 x = 0; x < Width; ++x)
		{
			FIntPoint PixelCoords(x, y);
			float Brightness = CachedBrightnessMap[PixelCoords];

			// Convert brightness to color (e.g., grayscale)
			uint8 BrightnessValue = FMath::RoundToInt(Brightness);

			// Write the color to the texture (RGBA format)
			int32 PixelIndex = (y * Width + x) * 4; // RGBA
			Data[PixelIndex + 0] = BrightnessValue; // Red
			Data[PixelIndex + 1] = BrightnessValue; // Green
			Data[PixelIndex + 2] = BrightnessValue; // Blue
			Data[PixelIndex + 3] = 255; // Alpha
		}
	}

	// Unlock the texture
	Mip.BulkData.Unlock();

	// Update the texture resource
	NewTexture->UpdateResource();

	return NewTexture;
}

void AChunkWorld::ShowHeightmap(UTexture2D* HeightmapTexture)
{
	if (!HeightmapTexture)
		return;

	// Ensure the Widget Class is set in the editor or code
	if (HeightmapWidgetClass)
	{
		// Create the widget
		UMyUserWidget* HeightmapWidget = CreateWidget<UMyUserWidget>(GetWorld(), HeightmapWidgetClass);

		if (HeightmapWidget)
		{
			// Set the heightmap texture
			HeightmapWidget->textureMap = HeightmapTexture;

			// Add the widget to the viewport
			HeightmapWidget->AddToViewport();
		}
	}
}
