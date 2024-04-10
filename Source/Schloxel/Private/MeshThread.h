// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GreedyChunk.h"
#include "HAL/Runnable.h"

class FRunnableThread;
class AGreedyChunk;

class AMeshThread : public FRunnable
{
public:
	
	AMeshThread(AGreedyChunk* greedyChunk);

	AGreedyChunk* GreedyChunk;
	FRunnableThread* Thread;

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	void CreateQuad(AGreedyChunk::FMask Mask, FIntVector AxisMask, int Width, int Height, FIntVector V1, FIntVector V2, FIntVector V3, FIntVector V4);

private:
	FChunkMeshData ChunkMeshData;
};
