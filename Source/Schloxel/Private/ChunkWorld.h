// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "GameFramework/Actor.h"
#include "ChunkWorld.generated.h"

UCLASS()
class AChunkWorld : public AActor
{
	GENERATED_BODY()

public:
	AChunkWorld();

	void ClearChunks();

protected:
	virtual void BeginPlay() override;

private:
	void SpawnChunks();

	UPROPERTY(EditAnywhere, Category="Chunk World")
	int DrawDistance = 5;

	UPROPERTY(EditAnywhere, Category="Chunk World")
	FIntVector ChunkSize;

	UPROPERTY(EditAnywhere, Category="Chunk World")
	int VoxelSize = 100;


	UPROPERTY(EditInstanceOnly, Category="Chunk World")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditInstanceOnly, Category="Chunk World")
	TObjectPtr<UTexture2D> HeightMap;

	TMap<FIntPoint, int> CachedBrightnessMap;

	void PrecomputeBrightness(UTexture2D* Texture);

	UTexture2D* CreateHeightmapTextureFromBrightness();

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> HeightmapWidgetClass;

	void ShowHeightmap(UTexture2D* HeightmapTexture);

	UPROPERTY()
	TArray<AActor*> SpawnedChunks;

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Chunk World")
	void RegenerateChunks();
#endif
};
