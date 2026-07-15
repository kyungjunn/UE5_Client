#include "LoginUserWidget.h"

#include "AccountSubsystem.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void ULoginUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &ULoginUserWidget::HandleLoginClicked);
	}
	if (UAccountSubsystem* Account = GetAccountSubsystem())
	{
		Account->OnLoginCompleted.AddDynamic(this, &ULoginUserWidget::HandleLoginCompleted);
	}
}

void ULoginUserWidget::NativeDestruct()
{
	if (LoginButton)
	{
		LoginButton->OnClicked.RemoveDynamic(this, &ULoginUserWidget::HandleLoginClicked);
	}
	if (UAccountSubsystem* Account = GetAccountSubsystem())
	{
		Account->OnLoginCompleted.RemoveDynamic(this, &ULoginUserWidget::HandleLoginCompleted);
	}

	Super::NativeDestruct();
}

void ULoginUserWidget::HandleLoginClicked()
{
	const FString Email = EmailInput ? EmailInput->GetText().ToString() : FString();
	const FString Password = PasswordInput ? PasswordInput->GetText().ToString() : FString();

	if (Email.IsEmpty() || Password.IsEmpty())
	{
		SetStatus(TEXT("이메일과 비밀번호를 입력하세요."), true);
		return;
	}

	UAccountSubsystem* Account = GetAccountSubsystem();
	if (!Account)
	{
		SetStatus(TEXT("계정 시스템을 찾을 수 없습니다."), true);
		return;
	}

	if (LoginButton)
	{
		LoginButton->SetIsEnabled(false);
	}
	SetStatus(TEXT("로그인 중..."), false);
	Account->Login(Email, Password);
}

void ULoginUserWidget::HandleLoginCompleted(bool bSuccess, int32 StatusCode, const FString& Message)
{
	if (LoginButton)
	{
		LoginButton->SetIsEnabled(true);
	}
	SetStatus(Message, !bSuccess);
	OnLoginFinished(bSuccess, Message);
}
