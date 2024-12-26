// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "VoxMeshData.h"
#include "VoxModel.h"

/**
 * 
 */
class AVoxMeshThread : FRunnable
{
public:
	AVoxMeshThread(AVoxModel* _VoxModel);

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

private:
	FVoxMeshData VoxMeshData;
	
	AVoxModel* VoxModel;

	void BuildGreedyMesh();
};
