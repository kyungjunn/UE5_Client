#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "AccountSubsystem.generated.h"

/** 서버 UserResponse 에 대응하는 계정 정보. */
USTRUCT(BlueprintType)
struct FAccountUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Account")
	int32 Id = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Account")
	FString Email;

	UPROPERTY(BlueprintReadOnly, Category = "Account")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "Account")
	FString CreatedAt;
};

/** 로그인 / 탈퇴 결과. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAccountSimpleResult,
	bool, bSuccess, int32, StatusCode, const FString&, Message);

/** 회원가입 / 내 정보 결과 (계정 정보 포함). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FAccountUserResult,
	bool, bSuccess, int32, StatusCode, const FString&, Message, const FAccountUser&, User);

/**
 * 계정 백엔드 서버(FastAPI)와 통신하는 클라이언트.
 * 회원가입 / 로그인 / 내 정보 / 회원탈퇴를 제공하며, 로그인 시 받은 JWT 를 메모리에 보관한다.
 */
UCLASS()
class UAccountSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** POST /auth/register */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void Register(const FString& Email, const FString& Nickname, const FString& Password);

	/** POST /auth/login → 성공 시 토큰 저장 */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void Login(const FString& Email, const FString& Password);

	/** GET /auth/me (Bearer) */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void GetMe();

	/** DELETE /auth/withdraw (Bearer) → 성공 시 토큰 삭제 */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void Withdraw();

	/** 서버 주소 변경 (기본값: http://127.0.0.1:8000) */
	UFUNCTION(BlueprintCallable, Category = "Account")
	void SetBaseUrl(const FString& Url);

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FAccountUserResult OnRegisterCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FAccountSimpleResult OnLoginCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FAccountUserResult OnMeCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Account")
	FAccountSimpleResult OnWithdrawCompleted;

private:
	/** 파싱된 HTTP 응답 공통 결과. */
	struct FParsedResponse
	{
		bool bSuccess = false;
		int32 StatusCode = 0;
		FString Message;
		FString Body;
	};

	FHttpRequestRef MakeRequest(const FString& Verb, const FString& Path, bool bAuth);

	static FParsedResponse ParseResponse(FHttpResponsePtr Response, bool bConnected);
	static bool ParseUser(const FString& Body, FAccountUser& OutUser);
	static FString ExtractError(int32 StatusCode, const FString& Body);

	void RegisterConsoleCommands();

	/** 결과를 콘솔 로그로 출력하기 위한 델리게이트 핸들러. */
	UFUNCTION()
	void LogSimple(bool bSuccess, int32 StatusCode, const FString& Message);

	UFUNCTION()
	void LogUser(bool bSuccess, int32 StatusCode, const FString& Message, const FAccountUser& User);

	FString BaseUrl = TEXT("http://127.0.0.1:8000");
	FString AuthToken;

	TArray<IConsoleObject*> ConsoleCommands;
};
