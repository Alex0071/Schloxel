// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkWorld.generated.h"

UCLASS()
class AChunkWorld : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AChunkWorld();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category="Chunk World")
	TSubclassOf<AActor> Chunk;

	UPROPERTY(EditAnywhere, Category="Chunk World")
	int DrawDistance = 5;

	UPROPERTY(EditAnywhere, Category="Chunk World")
	FIntVector ChunkSize;

	UPROPERTY(EditAnywhere, Category="Chunk World")
	int VoxelSize = 100;


	UPROPERTY(EditInstanceOnly, Category="Chunk World")
	TObjectPtr<UMaterialInterface> Material;
};
