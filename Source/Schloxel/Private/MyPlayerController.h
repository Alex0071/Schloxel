// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

UCLASS()
class AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMyPlayerController();

protected:
	virtual void BeginPlay() override;


	virtual void Tick(float DeltaTime) override;
    

	void OnRightMouseButtonPressed();
	void OnLeftMouseButtonPressed();
};
