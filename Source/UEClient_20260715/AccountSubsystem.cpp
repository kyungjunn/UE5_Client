#include "AccountSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"

void UAccountSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 콘솔/BP 어디서 호출하든 결과를 로그로 남긴다.
	OnRegisterCompleted.AddDynamic(this, &UAccountSubsystem::LogUser);
	OnLoginCompleted.AddDynamic(this, &UAccountSubsystem::LogSimple);
	OnMeCompleted.AddDynamic(this, &UAccountSubsystem::LogUser);
	OnWithdrawCompleted.AddDynamic(this, &UAccountSubsystem::LogSimple);

	RegisterConsoleCommands();
}

void UAccountSubsystem::Deinitialize()
{
	IConsoleManager& CM = IConsoleManager::Get();
	for (IConsoleObject* Cmd : ConsoleCommands)
	{
		CM.UnregisterConsoleObject(Cmd);
	}
	ConsoleCommands.Empty();

	Super::Deinitialize();
}

FHttpRequestRef UAccountSubsystem::MakeRequest(const FString& Verb, const FString& Path, bool bAuth)
{
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + Path);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	if (bAuth)
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
	}
	return Request;
}

UAccountSubsystem::FParsedResponse UAccountSubsystem::ParseResponse(FHttpResponsePtr Response, bool bConnected)
{
	FParsedResponse Out;
	if (!bConnected || !Response.IsValid())
	{
		Out.bSuccess = false;
		Out.StatusCode = 0;
		Out.Message = TEXT("서버에 연결할 수 없습니다.");
		return Out;
	}

	Out.StatusCode = Response->GetResponseCode();
	Out.Body = Response->GetContentAsString();

	if (Out.StatusCode >= 200 && Out.StatusCode < 300)
	{
		Out.bSuccess = true;
	}
	else
	{
		Out.bSuccess = false;
		Out.Message = ExtractError(Out.StatusCode, Out.Body);
	}
	return Out;
}

bool UAccountSubsystem::ParseUser(const FString& Body, FAccountUser& OutUser)
{
	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		return false;
	}

	Json->TryGetNumberField(TEXT("id"), OutUser.Id);
	Json->TryGetStringField(TEXT("email"), OutUser.Email);
	Json->TryGetStringField(TEXT("nickname"), OutUser.Nickname);
	Json->TryGetStringField(TEXT("created_at"), OutUser.CreatedAt);
	return true;
}

FString UAccountSubsystem::ExtractError(int32 StatusCode, const FString& Body)
{
	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
	{
		// FastAPI HTTPException: detail 이 문자열
		FString DetailStr;
		if (Json->TryGetStringField(TEXT("detail"), DetailStr))
		{
			return DetailStr;
		}

		// 검증 실패(422): detail 이 배열 → 첫 항목의 msg
		const TArray<TSharedPtr<FJsonValue>>* DetailArr = nullptr;
		if (Json->TryGetArrayField(TEXT("detail"), DetailArr) && DetailArr->Num() > 0)
		{
			const TSharedPtr<FJsonObject>* First = nullptr;
			if ((*DetailArr)[0]->TryGetObject(First))
			{
				FString Msg;
				if ((*First)->TryGetStringField(TEXT("msg"), Msg))
				{
					return Msg;
				}
			}
		}
	}
	return FString::Printf(TEXT("요청 실패 (HTTP %d)"), StatusCode);
}

void UAccountSubsystem::Register(const FString& Email, const FString& Nickname, const FString& Password)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("email"), Email);
	Body->SetStringField(TEXT("nickname"), Nickname);
	Body->SetStringField(TEXT("password"), Password);

	FString BodyStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body, Writer);

	FHttpRequestRef Request = MakeRequest(TEXT("POST"), TEXT("/auth/register"), false);
	Request->SetContentAsString(BodyStr);

	TWeakObjectPtr<UAccountSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
		{
			UAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			FParsedResponse P = ParseResponse(Response, bConnected);
			FAccountUser User;
			if (P.bSuccess)
			{
				ParseUser(P.Body, User);
				P.Message = TEXT("회원가입 성공");
			}
			Self->OnRegisterCompleted.Broadcast(P.bSuccess, P.StatusCode, P.Message, User);
		});
	Request->ProcessRequest();
}

void UAccountSubsystem::Login(const FString& Email, const FString& Password)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("email"), Email);
	Body->SetStringField(TEXT("password"), Password);

	FString BodyStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body, Writer);

	FHttpRequestRef Request = MakeRequest(TEXT("POST"), TEXT("/auth/login"), false);
	Request->SetContentAsString(BodyStr);

	TWeakObjectPtr<UAccountSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
		{
			UAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			FParsedResponse P = ParseResponse(Response, bConnected);
			if (P.bSuccess)
			{
				TSharedPtr<FJsonObject> Json;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(P.Body);
				if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
				{
					Json->TryGetStringField(TEXT("access_token"), Self->AuthToken);
				}
				P.Message = TEXT("로그인 성공");
			}
			Self->OnLoginCompleted.Broadcast(P.bSuccess, P.StatusCode, P.Message);
		});
	Request->ProcessRequest();
}

void UAccountSubsystem::GetMe()
{
	if (AuthToken.IsEmpty())
	{
		OnMeCompleted.Broadcast(false, 0, TEXT("로그인이 필요합니다."), FAccountUser());
		return;
	}

	FHttpRequestRef Request = MakeRequest(TEXT("GET"), TEXT("/auth/me"), true);

	TWeakObjectPtr<UAccountSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
		{
			UAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			FParsedResponse P = ParseResponse(Response, bConnected);
			FAccountUser User;
			if (P.bSuccess)
			{
				ParseUser(P.Body, User);
				P.Message = TEXT("내 정보 조회 성공");
			}
			Self->OnMeCompleted.Broadcast(P.bSuccess, P.StatusCode, P.Message, User);
		});
	Request->ProcessRequest();
}

void UAccountSubsystem::Withdraw()
{
	if (AuthToken.IsEmpty())
	{
		OnWithdrawCompleted.Broadcast(false, 0, TEXT("로그인이 필요합니다."));
		return;
	}

	FHttpRequestRef Request = MakeRequest(TEXT("DELETE"), TEXT("/auth/withdraw"), true);

	TWeakObjectPtr<UAccountSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
		{
			UAccountSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			FParsedResponse P = ParseResponse(Response, bConnected);
			if (P.bSuccess)
			{
				Self->AuthToken.Empty();
				P.Message = TEXT("회원탈퇴 완료");
			}
			Self->OnWithdrawCompleted.Broadcast(P.bSuccess, P.StatusCode, P.Message);
		});
	Request->ProcessRequest();
}

void UAccountSubsystem::SetBaseUrl(const FString& Url)
{
	BaseUrl = Url;
	while (BaseUrl.EndsWith(TEXT("/")))
	{
		BaseUrl = BaseUrl.LeftChop(1);
	}
	UE_LOG(LogTemp, Log, TEXT("[Account] BaseUrl = %s"), *BaseUrl);
}

void UAccountSubsystem::LogSimple(bool bSuccess, int32 StatusCode, const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("[Account] %s (HTTP %d) %s"),
		bSuccess ? TEXT("성공") : TEXT("실패"), StatusCode, *Message);
}

void UAccountSubsystem::LogUser(bool bSuccess, int32 StatusCode, const FString& Message, const FAccountUser& User)
{
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[Account] 성공 (HTTP %d) %s | id=%d email=%s nick=%s created=%s"),
			StatusCode, *Message, User.Id, *User.Email, *User.Nickname, *User.CreatedAt);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Account] 실패 (HTTP %d) %s"), StatusCode, *Message);
	}
}

void UAccountSubsystem::RegisterConsoleCommands()
{
	IConsoleManager& CM = IConsoleManager::Get();

	// PIE 다중 인스턴스(리슨서버 + 클라이언트) 실행 시 GameInstance 가 여러 개 생겨
	// 같은 이름의 콘솔 명령을 중복 등록하게 된다. 이 경우 콘솔 매니저가 기존 오브젝트를
	// 공유시키므로, 종료 시 양쪽 Deinitialize 가 같은 포인터를 이중 해제하며 크래시한다.
	// → 이미 등록돼 있으면(다른 인스턴스가 먼저 등록) 등록을 건너뛴다.
	if (CM.FindConsoleObject(TEXT("Account.Register")))
	{
		return;
	}

	TWeakObjectPtr<UAccountSubsystem> WeakThis(this);

	ConsoleCommands.Add(CM.RegisterConsoleCommand(
		TEXT("Account.Register"),
		TEXT("Account.Register <email> <nickname> <password>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([WeakThis](const TArray<FString>& Args)
		{
			if (!WeakThis.IsValid()) { return; }
			if (Args.Num() < 3)
			{
				UE_LOG(LogTemp, Warning, TEXT("사용법: Account.Register <email> <nickname> <password>"));
				return;
			}
			WeakThis->Register(Args[0], Args[1], Args[2]);
		}),
		ECVF_Default));

	ConsoleCommands.Add(CM.RegisterConsoleCommand(
		TEXT("Account.Login"),
		TEXT("Account.Login <email> <password>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([WeakThis](const TArray<FString>& Args)
		{
			if (!WeakThis.IsValid()) { return; }
			if (Args.Num() < 2)
			{
				UE_LOG(LogTemp, Warning, TEXT("사용법: Account.Login <email> <password>"));
				return;
			}
			WeakThis->Login(Args[0], Args[1]);
		}),
		ECVF_Default));

	ConsoleCommands.Add(CM.RegisterConsoleCommand(
		TEXT("Account.Me"),
		TEXT("Account.Me"),
		FConsoleCommandWithArgsDelegate::CreateLambda([WeakThis](const TArray<FString>&)
		{
			if (!WeakThis.IsValid()) { return; }
			WeakThis->GetMe();
		}),
		ECVF_Default));

	ConsoleCommands.Add(CM.RegisterConsoleCommand(
		TEXT("Account.Withdraw"),
		TEXT("Account.Withdraw"),
		FConsoleCommandWithArgsDelegate::CreateLambda([WeakThis](const TArray<FString>&)
		{
			if (!WeakThis.IsValid()) { return; }
			WeakThis->Withdraw();
		}),
		ECVF_Default));

	ConsoleCommands.Add(CM.RegisterConsoleCommand(
		TEXT("Account.SetBaseUrl"),
		TEXT("Account.SetBaseUrl <url>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([WeakThis](const TArray<FString>& Args)
		{
			if (!WeakThis.IsValid()) { return; }
			if (Args.Num() < 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("사용법: Account.SetBaseUrl <url>"));
				return;
			}
			WeakThis->SetBaseUrl(Args[0]);
		}),
		ECVF_Default));
}
