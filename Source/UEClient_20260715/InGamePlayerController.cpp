#include "InGamePlayerController.h"

#include "InGameChatWidget.h"
#include "NpcActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"

void AInGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (InteractMappingContext)
			{
				Subsys->AddMappingContext(InteractMappingContext, 1);
			}
		}
	}

	if (PromptWidgetClass)
	{
		PromptWidget = CreateWidget<UUserWidget>(this, PromptWidgetClass);
		if (PromptWidget)
		{
			PromptWidget->AddToViewport();
			PromptWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AInGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AInGamePlayerController::HandleInteract);
		}
	}
}

void AInGamePlayerController::SetNearbyNpc(ANpcActor* Npc)
{
	NearbyNpc = Npc;
	if (PromptWidget && !ChatWidget)
	{
		PromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void AInGamePlayerController::ClearNearbyNpc(ANpcActor* Npc)
{
	if (NearbyNpc != Npc)
	{
		return;
	}
	NearbyNpc = nullptr;

	if (PromptWidget)
	{
		PromptWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	// 반경을 벗어나면 열려 있던 채팅도 닫는다.
	if (ChatWidget)
	{
		CloseChat();
	}
}

void AInGamePlayerController::HandleInteract()
{
	if (ChatWidget)
	{
		CloseChat();
	}
	else if (NearbyNpc)
	{
		OpenChat();
	}
}

void AInGamePlayerController::OpenChat()
{
	if (!ChatWidgetClass || !NearbyNpc)
	{
		return;
	}

	ChatWidget = CreateWidget<UInGameChatWidget>(this, ChatWidgetClass);
	if (!ChatWidget)
	{
		return;
	}

	ChatWidget->SetNpcProfile(NearbyNpc->Profile);
	ChatWidget->OnCloseRequested.BindUObject(this, &AInGamePlayerController::CloseChat);
	ChatWidget->AddToViewport();

	if (PromptWidget)
	{
		PromptWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	// 위젯에 강제 포커스를 주지 않는다 → 입력창 포커스가 없을 때 F 가 게임 입력으로
	// 내려와 토글(닫기)이 동작한다. 입력창은 클릭해서 포커스 후 타이핑.
	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);

	SetMovementInputEnabled(false);
}

void AInGamePlayerController::CloseChat()
{
	if (ChatWidget)
	{
		ChatWidget->RemoveFromParent();
		ChatWidget = nullptr;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	SetMovementInputEnabled(true);

	if (PromptWidget && NearbyNpc)
	{
		PromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void AInGamePlayerController::SetMovementInputEnabled(bool bEnabled)
{
	ULocalPlayer* LP = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsys = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsys)
	{
		return;
	}

	// 템플릿 캐릭터가 쓰는 이동/시점 컨텍스트를 통째로 제거/복원해
	// 채팅 중에는 이동·점프·시점 조작이 모두 차단되게 한다.
	static const TCHAR* MovementContexts[] = {
		TEXT("/Game/Input/IMC_Default.IMC_Default"),
		TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"),
	};
	for (const TCHAR* Path : MovementContexts)
	{
		if (UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, Path))
		{
			if (bEnabled)
			{
				Subsys->AddMappingContext(IMC, 0);
			}
			else
			{
				Subsys->RemoveMappingContext(IMC);
			}
		}
	}
}
