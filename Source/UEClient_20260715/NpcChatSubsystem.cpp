#include "NpcChatSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"

void UNpcChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OnChatResponseReceived.AddDynamic(this, &UNpcChatSubsystem::LogResult);
	RegisterConsoleCommands();
}

void UNpcChatSubsystem::Deinitialize()
{
	IConsoleManager& CM = IConsoleManager::Get();
	for (IConsoleObject* Cmd : ConsoleCommands)
	{
		CM.UnregisterConsoleObject(Cmd);
	}
	ConsoleCommands.Empty();

	Super::Deinitialize();
}

FNpcProfile UNpcChatSubsystem::MakeDefaultProfile()
{
	FNpcProfile P;
	P.Name = TEXT("세라");
	P.Personality = TEXT("밝고 친절하며 약간 수다스러운 게임 로비 안내원입니다. 플레이어에게 존댓말을 쓰고, 방 만들기·방 참가 등 로비 이용을 즐겁게 도와줍니다.");
	P.StatusLine = TEXT("오늘 기분: 아주 좋음");
	P.PortraitColor = FLinearColor(0.55f, 0.35f, 0.75f, 1.f);
	return P;
}

FString UNpcChatSubsystem::BuildRequestBody(const FNpcProfile& Npc, const FString& UserMessage) const
{
	// 성격 지침 프롬프트: 이름 + 성격 + 상태를 시스템 메시지로 조립한다.
	const FString SystemPrompt = FString::Printf(
		TEXT("당신은 '%s'입니다. %s\n현재 상태: %s\n답변은 한국어로 2~3문장 이내로 짧게 하세요."),
		*Npc.Name, *Npc.Personality, *Npc.StatusLine);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> Messages;
	{
		TSharedRef<FJsonObject> Sys = MakeShared<FJsonObject>();
		Sys->SetStringField(TEXT("role"), TEXT("system"));
		Sys->SetStringField(TEXT("content"), SystemPrompt);
		Messages.Add(MakeShared<FJsonValueObject>(Sys));

		TSharedRef<FJsonObject> User = MakeShared<FJsonObject>();
		User->SetStringField(TEXT("role"), TEXT("user"));
		User->SetStringField(TEXT("content"), UserMessage);
		Messages.Add(MakeShared<FJsonValueObject>(User));
	}
	Root->SetArrayField(TEXT("messages"), Messages);

	// thinking 모델이 reasoning 에 토큰을 소모하므로 넉넉히 잡는다.
	Root->SetNumberField(TEXT("max_tokens"), 1024);
	Root->SetNumberField(TEXT("temperature"), 0.7);

	FString BodyStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Root, Writer);
	return BodyStr;
}

FString UNpcChatSubsystem::ExtractReply(const FString& Body)
{
	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		return FString();
	}

	const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
	if (!Json->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
	{
		return FString();
	}

	const TSharedPtr<FJsonObject>* Choice = nullptr;
	if (!(*Choices)[0]->TryGetObject(Choice))
	{
		return FString();
	}

	const TSharedPtr<FJsonObject>* Message = nullptr;
	if (!(*Choice)->TryGetObjectField(TEXT("message"), Message))
	{
		return FString();
	}

	FString Content;
	(*Message)->TryGetStringField(TEXT("content"), Content);
	return Content.TrimStartAndEnd();
}

FString UNpcChatSubsystem::ExtractError(int32 StatusCode, const FString& Body)
{
	// llama.cpp 에러 형식: {"error": {"message": "..."}}
	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
	{
		const TSharedPtr<FJsonObject>* Error = nullptr;
		if (Json->TryGetObjectField(TEXT("error"), Error))
		{
			FString Msg;
			if ((*Error)->TryGetStringField(TEXT("message"), Msg))
			{
				return Msg;
			}
		}
	}
	return FString::Printf(TEXT("요청 실패 (HTTP %d)"), StatusCode);
}

void UNpcChatSubsystem::SendChatMessage(const FNpcProfile& Npc, const FString& UserMessage)
{
	const FString Trimmed = UserMessage.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || bRequestInFlight)
	{
		return;
	}
	bRequestInFlight = true;

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(LlmBaseUrl + TEXT("/v1/chat/completions"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	// 로컬 LLM 은 생성이 끝날 때까지(수십 초) 아무 데이터도 보내지 않으므로
	// 총 타임아웃과 무활동(activity) 타임아웃을 모두 넉넉히 잡는다.
	Request->SetTimeout(180.f);
	Request->SetActivityTimeout(180.f);
	Request->SetContentAsString(BuildRequestBody(Npc, Trimmed));

	const FString NpcName = Npc.Name;
	TWeakObjectPtr<UNpcChatSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, NpcName](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
		{
			UNpcChatSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			Self->bRequestInFlight = false;

			if (!bConnected || !Response.IsValid())
			{
				Self->OnChatResponseReceived.Broadcast(false, NpcName, TEXT("LLM 서버에 연결할 수 없습니다."));
				return;
			}

			const int32 Code = Response->GetResponseCode();
			const FString Body = Response->GetContentAsString();
			if (Code < 200 || Code >= 300)
			{
				Self->OnChatResponseReceived.Broadcast(false, NpcName, ExtractError(Code, Body));
				return;
			}

			const FString Reply = ExtractReply(Body);
			if (Reply.IsEmpty())
			{
				// thinking 이 max_tokens 를 소진하면 content 가 빌 수 있다.
				Self->OnChatResponseReceived.Broadcast(false, NpcName, TEXT("응답이 비어 있습니다. (서버를 --reasoning-budget 0 으로 실행해 보세요)"));
				return;
			}

			Self->OnChatResponseReceived.Broadcast(true, NpcName, Reply);
		});
	Request->ProcessRequest();
}

void UNpcChatSubsystem::SetLlmBaseUrl(const FString& Url)
{
	LlmBaseUrl = Url;
	while (LlmBaseUrl.EndsWith(TEXT("/")))
	{
		LlmBaseUrl = LlmBaseUrl.LeftChop(1);
	}
	UE_LOG(LogTemp, Log, TEXT("[NpcChat] LlmBaseUrl = %s"), *LlmBaseUrl);
}

void UNpcChatSubsystem::LogResult(bool bSuccess, const FString& NpcName, const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("[NpcChat] %s (%s) %s"),
		bSuccess ? TEXT("성공") : TEXT("실패"), *NpcName, *Message);
}

void UNpcChatSubsystem::RegisterConsoleCommands()
{
	IConsoleManager& CM = IConsoleManager::Get();

	// PIE 다중 인스턴스에서 같은 이름을 중복 등록하면 종료 시 이중 해제로 크래시하므로
	// 이미 등록돼 있으면(다른 인스턴스가 먼저 등록) 건너뛴다. (AccountSubsystem 과 동일 패턴)
	if (CM.FindConsoleObject(TEXT("Npc.Chat")))
	{
		return;
	}

	TWeakObjectPtr<UNpcChatSubsystem> WeakThis(this);

	ConsoleCommands.Add(CM.RegisterConsoleCommand(
		TEXT("Npc.Chat"),
		TEXT("Npc.Chat <message> — 기본 NPC(세라)에게 메시지 전송"),
		FConsoleCommandWithArgsDelegate::CreateLambda([WeakThis](const TArray<FString>& Args)
		{
			if (!WeakThis.IsValid()) { return; }
			if (Args.Num() < 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("사용법: Npc.Chat <message>"));
				return;
			}
			WeakThis->SendChatMessage(MakeDefaultProfile(), FString::Join(Args, TEXT(" ")));
		}),
		ECVF_Default));

	ConsoleCommands.Add(CM.RegisterConsoleCommand(
		TEXT("Npc.SetLlmUrl"),
		TEXT("Npc.SetLlmUrl <url>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([WeakThis](const TArray<FString>& Args)
		{
			if (!WeakThis.IsValid()) { return; }
			if (Args.Num() < 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("사용법: Npc.SetLlmUrl <url>"));
				return;
			}
			WeakThis->SetLlmBaseUrl(Args[0]);
		}),
		ECVF_Default));
}
