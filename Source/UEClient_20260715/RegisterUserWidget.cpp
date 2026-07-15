#include "RegisterUserWidget.h"

#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void URegisterUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RegisterButton)
	{
		RegisterButton->OnClicked.AddDynamic(this, &URegisterUserWidget::HandleRegisterClicked);
	}
	if (UAccountSubsystem* Account = GetAccountSubsystem())
	{
		Account->OnRegisterCompleted.AddDynamic(this, &URegisterUserWidget::HandleRegisterCompleted);
	}
}

void URegisterUserWidget::NativeDestruct()
{
	if (RegisterButton)
	{
		RegisterButton->OnClicked.RemoveDynamic(this, &URegisterUserWidget::HandleRegisterClicked);
	}
	if (UAccountSubsystem* Account = GetAccountSubsystem())
	{
		Account->OnRegisterCompleted.RemoveDynamic(this, &URegisterUserWidget::HandleRegisterCompleted);
	}

	Super::NativeDestruct();
}

void URegisterUserWidget::HandleRegisterClicked()
{
	const FString Email = EmailInput ? EmailInput->GetText().ToString() : FString();
	const FString Nickname = NicknameInput ? NicknameInput->GetText().ToString() : FString();
	const FString Password = PasswordInput ? PasswordInput->GetText().ToString() : FString();

	if (Email.IsEmpty() || Nickname.IsEmpty() || Password.IsEmpty())
	{
		SetStatus(TEXT("이메일, 닉네임, 비밀번호를 모두 입력하세요."), true);
		return;
	}

	UAccountSubsystem* Account = GetAccountSubsystem();
	if (!Account)
	{
		SetStatus(TEXT("계정 시스템을 찾을 수 없습니다."), true);
		return;
	}

	if (RegisterButton)
	{
		RegisterButton->SetIsEnabled(false);
	}
	SetStatus(TEXT("회원가입 중..."), false);
	Account->Register(Email, Nickname, Password);
}

void URegisterUserWidget::HandleRegisterCompleted(bool bSuccess, int32 StatusCode, const FString& Message, const FAccountUser& User)
{
	if (RegisterButton)
	{
		RegisterButton->SetIsEnabled(true);
	}
	SetStatus(Message, !bSuccess);
	OnRegisterFinished(bSuccess, Message);
}
