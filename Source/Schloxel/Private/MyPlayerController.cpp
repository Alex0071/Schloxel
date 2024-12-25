// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"

#include "GreedyChunk.h"
#include "VoxelFunctionLibrary.h"
#include "VoxModel.h"

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Enable mouse input
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	// Lock mouse to game viewport
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	// Setup input mapping
	InputComponent->BindAction("RightMouseButton", IE_Pressed, this, &AMyPlayerController::OnRightMouseButtonPressed);
}

void AMyPlayerController::OnRightMouseButtonPressed()
{
	FHitResult HitResult;
	const FVector StartTrace = PlayerCameraManager->GetCameraLocation();
	const FRotator CameraRotation = PlayerCameraManager->GetCameraRotation();
	const FVector EndTrace = StartTrace + (CameraRotation.Vector() * 10000.0f);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility))
	{
		if (AGreedyChunk* Chunk = Cast<AGreedyChunk>(HitResult.GetActor()))
		{
			Chunk->ModifyVoxel(
				UVoxelFunctionLibrary::WorldToLocalBlockPosition(HitResult.Location - HitResult.Normal,
				                                                 Chunk->ChunkSize), EBlock::Air);
		}

		if (AVoxModel* VoxModel = Cast<AVoxModel>(HitResult.GetActor()))
		{
			VoxModel->ModifyVoxel(
				VoxModel->WorldToModelPosition(HitResult.Location - HitResult.Normal),
				EBlock::Air);
		}
	}
}
