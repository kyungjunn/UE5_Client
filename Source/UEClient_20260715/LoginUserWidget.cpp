#include "LoginUserWidget.h"

#include "AccountSubsystem.h"
#include "UEClientGameInstance.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void ULoginUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &ULoginUserWidget::HandleLoginClicked);
	}
	if (SignupButton)
	{
		SignupButton->OnClicked.AddDynamic(this, &ULoginUserWidget::HandleSignupClicked);
	}
	if (UAccountSubsystem* Account = GetAccountSubsystem())
	{
		Account->OnLoginCompleted.AddDynamic(this, &ULoginUserWidget::HandleLoginCompleted);
		Account->OnMeCompleted.AddDynamic(this, &ULoginUserWidget::HandleMeCompleted);
	}
}

void ULoginUserWidget::NativeDestruct()
{
	if (LoginButton)
	{
		LoginButton->OnClicked.RemoveDynamic(this, &ULoginUserWidget::HandleLoginClicked);
	}
	if (SignupButton)
	{
		SignupButton->OnClicked.RemoveDynamic(this, &ULoginUserWidget::HandleSignupClicked);
	}
	if (UAccountSubsystem* Account = GetAccountSubsystem())
	{
		Account->OnLoginCompleted.RemoveDynamic(this, &ULoginUserWidget::HandleLoginCompleted);
		Account->OnMeCompleted.RemoveDynamic(this, &ULoginUserWidget::HandleMeCompleted);
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
	SetStatus(Message, !bSuccess);
	OnLoginFinished(bSuccess, Message);

	if (!bSuccess)
	{
		if (LoginButton)
		{
			LoginButton->SetIsEnabled(true);
		}
		return;
	}

	// 로그인 성공 → 닉네임을 받아오기 위해 내 정보 조회. 버튼은 계속 비활성.
	SetStatus(TEXT("프로필 불러오는 중..."), false);
	if (UAccountSubsystem* Account = GetAccountSubsystem())
	{
		Account->GetMe();
	}
}

void ULoginUserWidget::HandleMeCompleted(bool bSuccess, int32 StatusCode, const FString& Message, const FAccountUser& User)
{
	if (LoginButton)
	{
		LoginButton->SetIsEnabled(true);
	}

	if (!bSuccess)
	{
		SetStatus(Message, true);
		return;
	}

	// 닉네임을 GameInstance에 저장 → Room 맵의 PlayerController가 읽어간다.
	if (UUEClientGameInstance* GI = Cast<UUEClientGameInstance>(GetGameInstance()))
	{
		GI->SetPlayerNickname(User.Nickname);
	}

	UGameplayStatics::OpenLevel(this, LobbyLevelName);
}

void ULoginUserWidget::HandleSignupClicked()
{
	if (!RegisterWidgetClass)
	{
		SetStatus(TEXT("회원가입 위젯 클래스가 설정되지 않았습니다."), true);
		return;
	}

	UUserWidget* RegisterWidget = nullptr;
	if (APlayerController* PC = GetOwningPlayer())
	{
		RegisterWidget = CreateWidget<UUserWidget>(PC, RegisterWidgetClass);
	}
	else if (UWorld* World = GetWorld())
	{
		RegisterWidget = CreateWidget<UUserWidget>(World, RegisterWidgetClass);
	}

	if (RegisterWidget)
	{
		RegisterWidget->AddToViewport();
	}
}
