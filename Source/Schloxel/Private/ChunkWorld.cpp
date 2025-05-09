// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkWorld.h"

#include "GreedyChunk.h"
#include "MyUserWidget.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AChunkWorld::AChunkWorld()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
void AChunkWorld::RegenerateChunks()
{
	ClearChunks();

	if (!HeightMap)
	{
		UE_LOG(LogTemp, Error, TEXT("HeightMap is null!"));
		return;
	}

	// Ensure the heightmap data is loaded
	HeightMap->WaitForStreaming();

	// Precompute brightness for the heightmap
	PrecomputeBrightness(HeightMap);

	SpawnChunks();
}
#endif

// Called when the game starts or when spawned
void AChunkWorld::BeginPlay()
{
	Super::BeginPlay();

	RegenerateChunks();
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

			AActor* NewChunk = nullptr;

			auto chunk = GetWorld()->SpawnActorDeferred<AGreedyChunk>(AGreedyChunk::StaticClass(), transform, this);
			chunk->Material = Material;
			chunk->ChunkSize = ChunkSize;
			chunk->VoxelSize = VoxelSize;
			chunk->CachedBrightnessMap = &CachedBrightnessMap;

			UGameplayStatics::FinishSpawningActor(chunk, transform);
			NewChunk = chunk;

			if (NewChunk)
			{
				SpawnedChunks.Add(NewChunk);
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


			uint8 BrightnessValue = FMath::RoundToInt(Brightness);

		
			int32 PixelIndex = (y * Width + x) * 4; 
			Data[PixelIndex + 0] = BrightnessValue; 
			Data[PixelIndex + 1] = BrightnessValue; 
			Data[PixelIndex + 2] = BrightnessValue; 
			Data[PixelIndex + 3] = 255; 
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


	if (HeightmapWidgetClass)
	{
		UMyUserWidget* HeightmapWidget = CreateWidget<UMyUserWidget>(GetWorld(), HeightmapWidgetClass);

		if (HeightmapWidget)
		{
			HeightmapWidget->textureMap = HeightmapTexture;
			
			HeightmapWidget->AddToViewport();
		}
	}
}

void AChunkWorld::ClearChunks()
{
	for (AActor* ChildActor : SpawnedChunks)
	{
		if (ChildActor)
		{
			ChildActor->Destroy();
		}
	}

	CachedBrightnessMap.Empty();
	SpawnedChunks.Empty();
}
