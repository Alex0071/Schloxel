#include "DebugSphere.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ADebugSphere::ADebugSphere()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a sphere mesh to represent the height
	UStaticMeshComponent* SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
	RootComponent = SphereMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (SphereAsset.Succeeded())
	{
		SphereMesh->SetStaticMesh(SphereAsset.Object);
		SphereMesh->SetRelativeScale3D(FVector(0.1f)); // Adjust the size of the sphere
	}
}

// Called when the game starts or when spawned
void ADebugSphere::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ADebugSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}