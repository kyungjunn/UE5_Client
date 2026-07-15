#include "AuthUserWidgetBase.h"

#include "AccountSubsystem.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UAccountSubsystem* UAuthUserWidgetBase::GetAccountSubsystem() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UAccountSubsystem>();
		}
	}
	return nullptr;
}

void UAuthUserWidgetBase::SetStatus(const FString& Message, bool bIsError)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
		const FLinearColor Color = bIsError
			? FLinearColor(0.90f, 0.20f, 0.20f)
			: FLinearColor(0.20f, 0.80f, 0.30f);
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}
