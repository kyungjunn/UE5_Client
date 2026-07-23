#include "NpcActor.h"

#include "InGamePlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"

ANpcActor::ANpcActor()
{
	// 스폰/빙의 타이밍과 무관하게 동작하도록 오버랩 대신 저빈도 거리 체크를 쓴다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(Mesh);
	InteractSphere->SetSphereRadius(250.f);
	InteractSphere->SetCollisionProfileName(TEXT("NoCollision"));
}

void ANpcActor::BeginPlay()
{
	Super::BeginPlay();

	if (IdleAnim && Mesh)
	{
		Mesh->PlayAnimation(IdleAnim, true);
	}
}

void ANpcActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 각 인스턴스(호스트/클라)에서 자신의 로컬 플레이어만 검사한다.
	AInGamePlayerController* PC = Cast<AInGamePlayerController>(GetWorld()->GetFirstPlayerController());
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	const float RadiusSq = FMath::Square(InteractSphere->GetScaledSphereRadius());
	const bool bInRange = FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= RadiusSq;
	if (bInRange != bPlayerInRange)
	{
		bPlayerInRange = bInRange;
		if (bInRange)
		{
			PC->SetNearbyNpc(this);
		}
		else
		{
			PC->ClearNearbyNpc(this);
		}
	}
}
