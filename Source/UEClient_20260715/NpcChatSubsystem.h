#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NpcChatSubsystem.generated.h"

/** LLM 에 성격 지침과 함께 전달되는 NPC 정의 (이름 / 성격 / 상태). */
USTRUCT(BlueprintType)
struct FNpcProfile
{
	GENERATED_BODY()

	/** NPC 이름 (예: 세라) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Npc")
	FString Name;

	/** 성격 지침 — 시스템 프롬프트 본문으로 들어간다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Npc")
	FString Personality;

	/** 상태 정보 (예: "오늘 기분: 좋음") — 시스템 프롬프트에 함께 조립된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Npc")
	FString StatusLine;

	/** 플레이스홀더 초상화 배경색. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Npc")
	FLinearColor PortraitColor = FLinearColor(0.55f, 0.35f, 0.75f, 1.f);
};

/** 채팅 응답 결과. 실패 시 Message 에 에러 설명이 들어간다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNpcChatResult,
	bool, bSuccess, const FString&, NpcName, const FString&, Message);

/**
 * 로컬 LLM(llama.cpp OpenAI 호환 서버, 기본 http://127.0.0.1:8080)과 통신하는 NPC 채팅 클라이언트.
 * 요청은 비동기이며, 완료되면 OnChatResponseReceived 델리게이트로 알린다.
 */
UCLASS()
class UNpcChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** NPC 성격 프롬프트 + 유저 메시지를 조립해 LLM 에 전송. 완료 시 OnChatResponseReceived 발생. */
	UFUNCTION(BlueprintCallable, Category = "NpcChat")
	void SendChatMessage(const FNpcProfile& Npc, const FString& UserMessage);

	/** LLM 서버 주소 변경 (기본값: http://127.0.0.1:8080) */
	UFUNCTION(BlueprintCallable, Category = "NpcChat")
	void SetLlmBaseUrl(const FString& Url);

	/** 기본 NPC(세라) 프로필 — 위젯 기본값과 콘솔 명령이 공용으로 사용. */
	static FNpcProfile MakeDefaultProfile();

	UPROPERTY(BlueprintAssignable, Category = "NpcChat")
	FNpcChatResult OnChatResponseReceived;

private:
	/** OpenAI 호환 /v1/chat/completions 요청 본문(JSON) 조립: system(성격/상태) + user(질문). */
	FString BuildRequestBody(const FNpcProfile& Npc, const FString& UserMessage) const;

	/** 응답 JSON 에서 choices[0].message.content 추출 (없으면 빈 문자열). */
	static FString ExtractReply(const FString& Body);
	static FString ExtractError(int32 StatusCode, const FString& Body);

	void RegisterConsoleCommands();

	/** 결과를 콘솔 로그로 출력하기 위한 델리게이트 핸들러. */
	UFUNCTION()
	void LogResult(bool bSuccess, const FString& NpcName, const FString& Message);

	FString LlmBaseUrl = TEXT("http://127.0.0.1:8080");

	/** 진행 중 요청이 있으면 새 요청을 무시한다. */
	bool bRequestInFlight = false;

	TArray<IConsoleObject*> ConsoleCommands;
};
