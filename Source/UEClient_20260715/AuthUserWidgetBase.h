#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuthUserWidgetBase.generated.h"

class UAccountSubsystem;
class UTextBlock;

/**
 * 로그인 / 회원가입 위젯의 공통 베이스.
 * 계정 서브시스템 접근과 상태 메시지 출력을 제공한다.
 */
UCLASS(Abstract)
class UAuthUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 상태/오류 메시지 출력용 (BP에서 이름을 StatusText 로 지으면 자동 연결, 선택). */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* StatusText;

	UAccountSubsystem* GetAccountSubsystem() const;

	/** StatusText 에 메시지를 출력. 오류면 빨강, 아니면 초록. */
	void SetStatus(const FString& Message, bool bIsError);
};
