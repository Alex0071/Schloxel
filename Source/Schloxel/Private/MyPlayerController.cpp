// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"

#include "GreedyChunk.h"
#include "VoxelFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "VoxModel.h"

AMyPlayerController::AMyPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();


	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;


	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	InputComponent->BindAction("RightMouseButton", IE_Pressed, this, &AMyPlayerController::OnRightMouseButtonPressed);
	InputComponent->BindAction("LeftMouseButton", IE_Pressed, this, &AMyPlayerController::OnLeftMouseButtonPressed);
}

void AMyPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyPlayerController::OnRightMouseButtonPressed()
{
	FHitResult HitResult;
	const FVector StartTrace = PlayerCameraManager->GetCameraLocation();
	const FRotator CameraRotation = PlayerCameraManager->GetCameraRotation();
	const FVector EndTrace = StartTrace + (CameraRotation.Vector() * 10000.0f);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility))
	{
		UNiagaraSystem* NiagaraSystemAsset = nullptr;
		if (AGreedyChunk* Chunk = Cast<AGreedyChunk>(HitResult.GetActor()))
		{
			Chunk->ModifyVoxel(
				UVoxelFunctionLibrary::WorldToLocalBlockPosition(HitResult.Location - HitResult.Normal,
				                                                 Chunk->ChunkSize), EBlock::Air);

			NiagaraSystemAsset = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/FX/Niagara.Niagara"));
		}


		if (AVoxModel* VoxModel = Cast<AVoxModel>(HitResult.GetActor()))
		{
			VoxModel->ModifyVoxel(
				VoxModel->WorldToModelPosition(HitResult.Location - HitResult.Normal),
				EBlock::Air);

			NiagaraSystemAsset = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/FX/Niagara2.Niagara2"));
		}

		if (NiagaraSystemAsset)
		{
			FVector _spwnLoc = HitResult.Location;
			FRotator SpawnRotation = FRotator::ZeroRotator;

			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				NiagaraSystemAsset,
				_spwnLoc,
				SpawnRotation
			);
		}
	}
}

void AMyPlayerController::OnLeftMouseButtonPressed()
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
				                                                 Chunk->ChunkSize), EBlock::Stone);
		}

		if (AVoxModel* VoxModel = Cast<AVoxModel>(HitResult.GetActor()))
		{
			VoxModel->ModifyVoxel(
				VoxModel->WorldToModelPosition(HitResult.Location - HitResult.Normal),
				EBlock::Stone);
		}
	}
}
