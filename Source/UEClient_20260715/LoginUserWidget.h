#pragma once

#include "CoreMinimal.h"
#include "AuthUserWidgetBase.h"
#include "LoginUserWidget.generated.h"

class UEditableTextBox;
class UButton;

/**
 * 로그인 위젯 베이스. BP 위젯에서 아래 이름의 자식 위젯을 배치하면 자동 연결된다:
 *   EmailInput, PasswordInput (EditableTextBox), LoginButton (Button)
 */
UCLASS()
class ULoginUserWidget : public UAuthUserWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* EmailInput;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* PasswordInput;

	UPROPERTY(meta = (BindWidget))
	UButton* LoginButton;

	/** 회원가입 화면으로 이동하는 버튼. */
	UPROPERTY(meta = (BindWidget))
	UButton* SignupButton;

	/** Signup 버튼으로 띄울 회원가입 위젯 클래스 (BP 기본값에서 WBP_Register 지정). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
	TSubclassOf<UUserWidget> RegisterWidgetClass;

	/** 로그인 완료 시 호출. 화면 전환 등은 BP에서 처리. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Account")
	void OnLoginFinished(bool bSuccess, const FString& Message);

private:
	UFUNCTION()
	void HandleLoginClicked();

	UFUNCTION()
	void HandleSignupClicked();

	UFUNCTION()
	void HandleLoginCompleted(bool bSuccess, int32 StatusCode, const FString& Message);
};
