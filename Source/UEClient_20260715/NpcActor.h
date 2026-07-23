#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NpcChatSubsystem.h"
#include "NpcActor.generated.h"

class USkeletalMeshComponent;
class USphereComponent;

/**
 * 필드에 배치되는 대화 NPC. 가만히 서 있고, 반경(InteractSphere) 안에
 * 로컬 플레이어 폰이 들어오면 PC 에 알려 "[F] 대화 시작" 프롬프트를 띄운다.
 */
UCLASS()
class ANpcActor : public AActor
{
	GENERATED_BODY()

public:
	ANpcActor();

	/** LLM 에 전달되는 이 NPC 의 이름/성격/상태 (맵 인스턴스별로 설정). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Npc")
	FNpcProfile Profile;

	/** 지정 시 BeginPlay 에서 루프 재생 (아이들 애니메이션). */
	UPROPERTY(EditAnywhere, Category = "Npc")
	UAnimationAsset* IdleAnim = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Npc")
	USkeletalMeshComponent* Mesh;

	/** 대화 가능 반경 (구체 반지름 사용). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Npc")
	USphereComponent* InteractSphere;

private:
	/** 로컬 플레이어가 반경 안에 있는지 (상태 변화 시에만 PC 에 알림). */
	bool bPlayerInRange = false;
};
