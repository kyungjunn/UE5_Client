#pragma once

#include "CoreMinimal.h"
#include "AuthUserWidgetBase.h"
#include "AccountSubsystem.h"
#include "RegisterUserWidget.generated.h"

class UEditableTextBox;
class UButton;

/**
 * 회원가입 위젯 베이스. BP 위젯에서 아래 이름의 자식 위젯을 배치하면 자동 연결된다:
 *   EmailInput, NicknameInput, PasswordInput (EditableTextBox), RegisterButton (Button)
 */
UCLASS()
class URegisterUserWidget : public UAuthUserWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* EmailInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* NicknameInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* PasswordInput;

	UPROPERTY(meta = (BindWidget))
	UButton* RegisterButton;

	/** 회원가입 화면을 닫고 Login 화면으로 돌아가는 버튼. */
	UPROPERTY(meta = (BindWidget))
	UButton* ExitButton;

	/** 회원가입 완료 시 호출. 화면 전환 등은 BP에서 처리. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Account")
	void OnRegisterFinished(bool bSuccess, const FString& Message);

private:
	UFUNCTION()
	void HandleRegisterClicked();

	UFUNCTION()
	void HandleExitClicked();

	UFUNCTION()
	void HandleRegisterCompleted(bool bSuccess, int32 StatusCode, const FString& Message, const FAccountUser& User);
};
