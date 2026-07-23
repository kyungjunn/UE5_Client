#include "LobbyAssetGenerator.h"

#if WITH_EDITOR

#include "LobbyWidget.h"
#include "RoomWidget.h"
#include "RoomListEntryWidget.h"
#include "NpcChatWidget.h"
#include "InGameChatWidget.h"
#include "LobbyPlayerController.h"
#include "LobbyGameMode.h"
#include "InGamePlayerController.h"
#include "InGameGameMode.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Factories/BlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"

namespace
{
	const TCHAR* WidgetDir = TEXT("/Game/Lobby/Widget");

	const FLinearColor ColorBg(0.05f, 0.06f, 0.08f, 1.f);       // 화면 배경(어두움)
	const FLinearColor ColorPanel(0.12f, 0.14f, 0.18f, 1.f);    // 리스트 엔트리 배경

	IAssetTools& GetAssetTools()
	{
		return FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	}

	// ---- 위젯 생성 헬퍼 ---------------------------------------------------
	UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FString& Text, int32 Size, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		T->SetText(FText::FromString(Text));
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		T->SetFont(F);
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	}

	UButton* MakeButton(UWidgetTree* Tree, FName Name, const FString& Label)
	{
		UButton* B = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* L = MakeText(Tree, NAME_None, Label, 22, FLinearColor(0.05f, 0.05f, 0.05f, 1.f));
		L->SetJustification(ETextJustify::Center);
		B->AddChild(L);
		return B;
	}

	// ---- 슬롯 헬퍼 -------------------------------------------------------
	void FillCanvas(UWidget* W)
	{
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(W->Slot))
		{
			S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			S->SetOffsets(FMargin(0.f));
		}
	}

	UVerticalBoxSlot* VAdd(UVerticalBox* Box, UWidget* Child, ESlateSizeRule::Type Rule,
		EHorizontalAlignment H, EVerticalAlignment V, const FMargin& Pad)
	{
		UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Child);
		FSlateChildSize Sz;
		Sz.SizeRule = Rule;
		Sz.Value = (Rule == ESlateSizeRule::Fill) ? 1.f : 0.f;
		S->SetSize(Sz);
		S->SetHorizontalAlignment(H);
		S->SetVerticalAlignment(V);
		S->SetPadding(Pad);
		return S;
	}

	UHorizontalBoxSlot* HAdd(UHorizontalBox* Box, UWidget* Child, ESlateSizeRule::Type Rule,
		EVerticalAlignment V, const FMargin& Pad)
	{
		UHorizontalBoxSlot* S = Box->AddChildToHorizontalBox(Child);
		FSlateChildSize Sz;
		Sz.SizeRule = Rule;
		Sz.Value = (Rule == ESlateSizeRule::Fill) ? 1.f : 0.f;
		S->SetSize(Sz);
		S->SetVerticalAlignment(V);
		S->SetPadding(Pad);
		return S;
	}

	// 화면 전체를 채우는 어두운 배경 Border 를 루트로 만들고, 그 안의 세로 박스를 돌려준다.
	UVerticalBox* BuildScreenRoot(UWidgetTree* Tree, const FLinearColor& Bg, float Margin)
	{
		UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		Tree->RootWidget = Canvas;

		UBorder* BgBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BgBorder"));
		BgBorder->SetBrushColor(Bg);
		BgBorder->SetHorizontalAlignment(HAlign_Fill);
		BgBorder->SetVerticalAlignment(VAlign_Fill);
		BgBorder->SetPadding(FMargin(Margin));
		Canvas->AddChild(BgBorder);
		FillCanvas(BgBorder);

		UVerticalBox* Root = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		BgBorder->AddChild(Root);
		return Root;
	}

	// 기존 위젯 트리를 비운다(이름 충돌 방지를 위해 트랜지언트로 이동).
	void WipeTree(UWidgetTree* Tree)
	{
		TArray<UWidget*> All;
		Tree->GetAllWidgets(All);
		Tree->RootWidget = nullptr;
		for (UWidget* Wd : All)
		{
			Tree->RemoveWidget(Wd);
			Wd->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}
	}

	// 있으면 로드, 없으면 생성(같은 UClass 유지 → 외부 참조 보존).
	UWidgetBlueprint* LoadOrCreateWidgetBP(const FString& Name, UClass* Parent)
	{
		const FString ObjPath = FString(WidgetDir) / Name + TEXT(".") + Name;
		if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, *ObjPath))
		{
			return Existing;
		}
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->ParentClass = Parent;
		return Cast<UWidgetBlueprint>(GetAssetTools().CreateAsset(Name, WidgetDir, UWidgetBlueprint::StaticClass(), Factory));
	}

	// 일반 Blueprint 를 있으면 로드, 없으면 생성.
	UBlueprint* LoadOrCreateBP(const TCHAR* Dir, const FString& Name, UClass* Parent)
	{
		const FString ObjPath = FString(Dir) / Name + TEXT(".") + Name;
		if (UBlueprint* Existing = LoadObject<UBlueprint>(nullptr, *ObjPath))
		{
			return Existing;
		}
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = Parent;
		return Cast<UBlueprint>(GetAssetTools().CreateAsset(Name, Dir, UBlueprint::StaticClass(), Factory));
	}

	void Compile(UBlueprint* BP)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);
	}

	void SetClassDefault(UBlueprint* BP, FName PropName, UClass* Value)
	{
		UObject* CDO = BP->GeneratedClass->GetDefaultObject();
		if (FClassProperty* Prop = CastField<FClassProperty>(CDO->GetClass()->FindPropertyByName(PropName)))
		{
			Prop->SetPropertyValue_InContainer(CDO, Value);
		}
	}

	// SetClassDefault 의 오브젝트 참조(FObjectProperty) 버전 (UInputAction* 등).
	void SetObjectDefault(UBlueprint* BP, FName PropName, UObject* Value)
	{
		UObject* CDO = BP->GeneratedClass->GetDefaultObject();
		if (FObjectProperty* Prop = CastField<FObjectProperty>(CDO->GetClass()->FindPropertyByName(PropName)))
		{
			Prop->SetPropertyValue_InContainer(CDO, Value);
		}
	}
}

void ULobbyAssetGenerator::GenerateLobbyAssets()
{
	// 1) WBP_RoomEntry — 방 목록 한 줄 (가로 배치)
	UWidgetBlueprint* Entry = LoadOrCreateWidgetBP(TEXT("WBP_RoomEntry"), URoomListEntryWidget::StaticClass());
	{
		UWidgetTree* Tree = Entry->WidgetTree;
		WipeTree(Tree);

		// 루트를 Border 로 둔다. ScrollBox 안에 들어갈 때 CanvasPanel(desired size 0)이면
		// 높이 0으로 렌더되어 보이지 않으므로, 콘텐츠 크기를 갖는 Border 를 루트로 사용한다.
		UBorder* Row = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EntryBorder"));
		Row->SetBrushColor(ColorPanel);
		Row->SetHorizontalAlignment(HAlign_Fill);
		Row->SetVerticalAlignment(VAlign_Fill);
		Row->SetPadding(FMargin(16.f, 12.f));
		Tree->RootWidget = Row;

		UHorizontalBox* HB = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowBox"));
		Row->AddChild(HB);

		HAdd(HB, MakeText(Tree, TEXT("RoomNameText"), TEXT("방 이름"), 24), ESlateSizeRule::Fill, VAlign_Center, FMargin(0, 0, 16, 0));
		HAdd(HB, MakeText(Tree, TEXT("HostNameText"), TEXT("방장"), 20, FLinearColor(0.7f, 0.75f, 0.8f, 1.f)), ESlateSizeRule::Automatic, VAlign_Center, FMargin(16, 0));
		HAdd(HB, MakeText(Tree, TEXT("CountText"), TEXT("0/4"), 20, FLinearColor(0.7f, 0.75f, 0.8f, 1.f)), ESlateSizeRule::Automatic, VAlign_Center, FMargin(16, 0));
		HAdd(HB, MakeButton(Tree, TEXT("JoinButton"), TEXT("참가")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(16, 0, 0, 0));

		Compile(Entry);
	}

	// 2) WBP_Room — 방 안: 접속자 목록 + 나가기
	UWidgetBlueprint* Room = LoadOrCreateWidgetBP(TEXT("WBP_Room"), URoomWidget::StaticClass());
	{
		UWidgetTree* Tree = Room->WidgetTree;
		WipeTree(Tree);
		UVerticalBox* Root = BuildScreenRoot(Tree, ColorBg, 60.f);

		VAdd(Root, MakeText(Tree, TEXT("TitleText"), TEXT("방 (Room)"), 48), ESlateSizeRule::Automatic, HAlign_Left, VAlign_Top, FMargin(0, 0, 0, 24));

		UVerticalBox* PlayerList = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerListBox"));
		VAdd(Root, PlayerList, ESlateSizeRule::Fill, HAlign_Fill, VAlign_Fill, FMargin(0, 0, 0, 24));

		// 하단 버튼 바: 준비(클라 전용) / 시작(호스트 전용) / 나가기 — 표시 여부는 RoomWidget 이 제어
		UHorizontalBox* BottomBar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomBar"));
		VAdd(Root, BottomBar, ESlateSizeRule::Automatic, HAlign_Center, VAlign_Bottom, FMargin(0));
		HAdd(BottomBar, MakeButton(Tree, TEXT("ReadyButton"), TEXT("준비")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(8, 0));
		HAdd(BottomBar, MakeButton(Tree, TEXT("StartButton"), TEXT("시작")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(8, 0));
		HAdd(BottomBar, MakeButton(Tree, TEXT("LeaveButton"), TEXT("나가기")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(8, 0));

		Compile(Room);
	}

	// 3) WBP_Lobby — 로비: 프로필 + 방 만들기 + 방 목록
	UWidgetBlueprint* Lobby = LoadOrCreateWidgetBP(TEXT("WBP_Lobby"), ULobbyWidget::StaticClass());
	{
		UWidgetTree* Tree = Lobby->WidgetTree;
		WipeTree(Tree);
		UVerticalBox* Root = BuildScreenRoot(Tree, ColorBg, 60.f);

		// 본문: 왼쪽 로비 컬럼(fill) + 오른쪽 NPC 채팅 패널(고정폭)
		UHorizontalBox* Body = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BodyBox"));
		VAdd(Root, Body, ESlateSizeRule::Fill, HAlign_Fill, VAlign_Fill, FMargin(0));

		UVerticalBox* Left = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftColumn"));
		HAdd(Body, Left, ESlateSizeRule::Fill, VAlign_Fill, FMargin(0, 0, 24, 0));

		VAdd(Left, MakeText(Tree, TEXT("TitleText"), TEXT("로비 (Lobby)"), 48), ESlateSizeRule::Automatic, HAlign_Left, VAlign_Top, FMargin(0, 0, 0, 8));
		VAdd(Left, MakeText(Tree, TEXT("NicknameText"), TEXT("닉네임"), 24, FLinearColor(0.6f, 0.85f, 1.f, 1.f)), ESlateSizeRule::Automatic, HAlign_Left, VAlign_Top, FMargin(0, 0, 0, 24));

		// 상단 바: 방 이름 입력 + 방 만들기 + 새로고침
		UHorizontalBox* TopBar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
		VAdd(Left, TopBar, ESlateSizeRule::Automatic, HAlign_Fill, VAlign_Top, FMargin(0, 0, 0, 24));

		UEditableTextBox* Input = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RoomNameInput"));
		Input->SetHintText(FText::FromString(TEXT("방 이름을 입력하세요")));
		HAdd(TopBar, Input, ESlateSizeRule::Fill, VAlign_Center, FMargin(0, 0, 16, 0));
		HAdd(TopBar, MakeButton(Tree, TEXT("CreateRoomButton"), TEXT("방 만들기")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(8, 0));
		HAdd(TopBar, MakeButton(Tree, TEXT("RefreshButton"), TEXT("새로고침")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(8, 0, 0, 0));

		// 방 목록 (남은 공간 전부)
		UScrollBox* RoomList = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RoomListBox"));
		VAdd(Left, RoomList, ESlateSizeRule::Fill, HAlign_Fill, VAlign_Fill, FMargin(0));

		// 우측: NPC 채팅 패널 슬롯 (내용은 ULobbyWidget 이 NpcChatWidgetClass 로 채운다)
		USizeBox* ChatSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChatPanelSize"));
		ChatSize->SetWidthOverride(420.f);
		HAdd(Body, ChatSize, ESlateSizeRule::Automatic, VAlign_Fill, FMargin(0));

		UBorder* ChatSlot = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChatPanelSlot"));
		ChatSlot->SetBrushColor(FLinearColor::Transparent);
		ChatSlot->SetHorizontalAlignment(HAlign_Fill);
		ChatSlot->SetVerticalAlignment(VAlign_Fill);
		ChatSlot->SetPadding(FMargin(0.f));
		ChatSize->AddChild(ChatSlot);

		Compile(Lobby);
	}

	// WBP_Lobby 가 목록 한 줄로 WBP_RoomEntry 를 쓰도록 재확인
	SetClassDefault(Lobby, TEXT("RoomEntryWidgetClass"), Entry->GeneratedClass);

	UE_LOG(LogTemp, Display, TEXT("[LobbyGen] Rebuilt WBP_RoomEntry / WBP_Room / WBP_Lobby layouts (1920x1080 full-screen)"));
}

void ULobbyAssetGenerator::GenerateRoomWiring()
{
	// BP_RoomPC : ALobbyPlayerController + LobbyWidgetClass = WBP_Room
	UWidgetBlueprint* RoomWidget = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Lobby/Widget/WBP_Room.WBP_Room"));
	if (!RoomWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[LobbyGen] WBP_Room not found; run GenerateLobbyAssets first."));
		return;
	}

	UBlueprint* RoomPC = LoadOrCreateBP(TEXT("/Game/Lobby"), TEXT("BP_RoomPC"), ALobbyPlayerController::StaticClass());
	Compile(RoomPC);
	SetClassDefault(RoomPC, TEXT("LobbyWidgetClass"), RoomWidget->GeneratedClass);

	// BP_RoomGM : ALobbyGameMode + PlayerControllerClass = BP_RoomPC
	UBlueprint* RoomGM = LoadOrCreateBP(TEXT("/Game/Lobby"), TEXT("BP_RoomGM"), ALobbyGameMode::StaticClass());
	Compile(RoomGM);
	SetClassDefault(RoomGM, TEXT("PlayerControllerClass"), RoomPC->GeneratedClass);

	UE_LOG(LogTemp, Display, TEXT("[LobbyGen] Created BP_RoomPC (WBP_Room) / BP_RoomGM (BP_RoomPC)"));
}

void ULobbyAssetGenerator::GenerateNpcChatAssets()
{
	// WBP_NpcChat — 상단(초상화+이름) / 중앙(대화 로그) / 하단(입력+전송)
	UWidgetBlueprint* Chat = LoadOrCreateWidgetBP(TEXT("WBP_NpcChat"), UNpcChatWidget::StaticClass());
	{
		UWidgetTree* Tree = Chat->WidgetTree;
		WipeTree(Tree);

		// ChatPanelSlot(Border) 안에 들어가므로 콘텐츠 크기를 갖는 Border 를 루트로 사용.
		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChatRoot"));
		Panel->SetBrushColor(ColorPanel);
		Panel->SetHorizontalAlignment(HAlign_Fill);
		Panel->SetVerticalAlignment(VAlign_Fill);
		Panel->SetPadding(FMargin(16.f));
		Tree->RootWidget = Panel;

		UVerticalBox* VB = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChatVBox"));
		Panel->AddChild(VB);

		// 상단: 초상화(색 배경 + 이니셜) + 이름 — 화면 우측 상단 끝에 위치
		UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NpcHeader"));
		VAdd(VB, Header, ESlateSizeRule::Automatic, HAlign_Right, VAlign_Top, FMargin(0, 0, 0, 16));

		USizeBox* PortraitSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PortraitSize"));
		PortraitSize->SetWidthOverride(64.f);
		PortraitSize->SetHeightOverride(64.f);
		UBorder* Portrait = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitBorder"));
		Portrait->SetBrushColor(FLinearColor(0.55f, 0.35f, 0.75f, 1.f)); // 런타임에 프로필 색으로 갱신됨
		Portrait->SetHorizontalAlignment(HAlign_Center);
		Portrait->SetVerticalAlignment(VAlign_Center);
		Portrait->AddChild(MakeText(Tree, TEXT("NpcInitialText"), TEXT("세"), 28));
		PortraitSize->AddChild(Portrait);
		HAdd(Header, PortraitSize, ESlateSizeRule::Automatic, VAlign_Center, FMargin(0, 0, 12, 0));
		HAdd(Header, MakeText(Tree, TEXT("NpcNameText"), TEXT("세라"), 30), ESlateSizeRule::Automatic, VAlign_Center, FMargin(0));

		// 중앙: 대화 로그
		UScrollBox* Log = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatLogBox"));
		VAdd(VB, Log, ESlateSizeRule::Fill, HAlign_Fill, VAlign_Fill, FMargin(0, 0, 0, 16));

		// 하단: 입력 + 전송
		UHorizontalBox* InputRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ChatInputRow"));
		VAdd(VB, InputRow, ESlateSizeRule::Automatic, HAlign_Fill, VAlign_Bottom, FMargin(0));

		UEditableTextBox* Input = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ChatInput"));
		Input->SetHintText(FText::FromString(TEXT("메시지를 입력하세요")));
		HAdd(InputRow, Input, ESlateSizeRule::Fill, VAlign_Center, FMargin(0, 0, 8, 0));
		HAdd(InputRow, MakeButton(Tree, TEXT("SendButton"), TEXT("전송")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(0));

		Compile(Chat);
	}

	// WBP_Lobby 의 우측 패널에 이 위젯을 쓰도록 CDO 배선
	UWidgetBlueprint* Lobby = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Lobby/Widget/WBP_Lobby.WBP_Lobby"));
	if (!Lobby)
	{
		UE_LOG(LogTemp, Error, TEXT("[LobbyGen] WBP_Lobby not found; run GenerateLobbyAssets first."));
		return;
	}
	SetClassDefault(Lobby, TEXT("NpcChatWidgetClass"), Chat->GeneratedClass);

	UE_LOG(LogTemp, Display, TEXT("[LobbyGen] Rebuilt WBP_NpcChat and wired WBP_Lobby.NpcChatWidgetClass"));
}

void ULobbyAssetGenerator::GenerateInGameAssets()
{
	// 1) EnhancedInput 에셋: IA_Interact (F 키) + IMC_Interact
	UInputAction* IA = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Interact.IA_Interact"));
	if (!IA)
	{
		UPackage* Pkg = CreatePackage(TEXT("/Game/Input/Actions/IA_Interact"));
		IA = NewObject<UInputAction>(Pkg, TEXT("IA_Interact"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(IA);
	}
	IA->ValueType = EInputActionValueType::Boolean;
	IA->MarkPackageDirty();

	UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Interact.IMC_Interact"));
	if (!IMC)
	{
		UPackage* Pkg = CreatePackage(TEXT("/Game/Input/IMC_Interact"));
		IMC = NewObject<UInputMappingContext>(Pkg, TEXT("IMC_Interact"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(IMC);
	}
	// 주의: MapKey()/UnmapAll() 은 여기서 호출해도 저장에 반영되지 않았다(검증됨).
	// F 키 매핑은 호출측 Python 드라이버가 mappings 프로퍼티를 직접 설정한다.
	IMC->MarkPackageDirty();

	// 2) WBP_InGameChat — 화면 하단 검정 반투명 채팅 바 (부모 UInGameChatWidget)
	UWidgetBlueprint* Chat = LoadOrCreateWidgetBP(TEXT("WBP_InGameChat"), UInGameChatWidget::StaticClass());
	{
		UWidgetTree* Tree = Chat->WidgetTree;
		WipeTree(Tree);

		UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ChatCanvas"));
		Tree->RootWidget = Canvas;

		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChatRoot"));
		Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.65f));
		Panel->SetHorizontalAlignment(HAlign_Fill);
		Panel->SetVerticalAlignment(VAlign_Fill);
		Panel->SetPadding(FMargin(20.f, 14.f));
		Canvas->AddChild(Panel);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			// 하단 도킹: 좌우 40 여백. Y 앵커가 비스트레치(1→1)이므로
			// Top = 위치(-360 = 바닥에서 360 위), Bottom = "높이"(320) 로 해석된다.
			S->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
			S->SetOffsets(FMargin(40.f, -360.f, 40.f, 320.f));
		}

		UVerticalBox* VB = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChatVBox"));
		Panel->AddChild(VB);

		// 헤더: 소형 초상화 + 이름
		UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NpcHeader"));
		VAdd(VB, Header, ESlateSizeRule::Automatic, HAlign_Left, VAlign_Top, FMargin(0, 0, 0, 8));

		USizeBox* PortraitSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PortraitSize"));
		PortraitSize->SetWidthOverride(40.f);
		PortraitSize->SetHeightOverride(40.f);
		UBorder* Portrait = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitBorder"));
		Portrait->SetBrushColor(FLinearColor(0.55f, 0.35f, 0.75f, 1.f)); // 런타임에 프로필 색으로 갱신
		Portrait->SetHorizontalAlignment(HAlign_Center);
		Portrait->SetVerticalAlignment(VAlign_Center);
		Portrait->AddChild(MakeText(Tree, TEXT("NpcInitialText"), TEXT("N"), 18));
		PortraitSize->AddChild(Portrait);
		HAdd(Header, PortraitSize, ESlateSizeRule::Automatic, VAlign_Center, FMargin(0, 0, 10, 0));
		HAdd(Header, MakeText(Tree, TEXT("NpcNameText"), TEXT("NPC"), 22), ESlateSizeRule::Automatic, VAlign_Center, FMargin(0));

		// 대화 로그
		UScrollBox* Log = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatLogBox"));
		VAdd(VB, Log, ESlateSizeRule::Fill, HAlign_Fill, VAlign_Fill, FMargin(0, 0, 0, 10));

		// 입력 행
		UHorizontalBox* InputRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ChatInputRow"));
		VAdd(VB, InputRow, ESlateSizeRule::Automatic, HAlign_Fill, VAlign_Bottom, FMargin(0));
		UEditableTextBox* Input = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ChatInput"));
		Input->SetHintText(FText::FromString(TEXT("클릭해서 입력... (ESC 닫기)")));
		HAdd(InputRow, Input, ESlateSizeRule::Fill, VAlign_Center, FMargin(0, 0, 8, 0));
		HAdd(InputRow, MakeButton(Tree, TEXT("SendButton"), TEXT("전송")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(0));

		Compile(Chat);
	}

	// 3) WBP_InteractPrompt — "[F] 대화 시작" 프롬프트 (하단 중앙)
	UWidgetBlueprint* Prompt = LoadOrCreateWidgetBP(TEXT("WBP_InteractPrompt"), UUserWidget::StaticClass());
	{
		UWidgetTree* Tree = Prompt->WidgetTree;
		WipeTree(Tree);

		UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PromptCanvas"));
		Tree->RootWidget = Canvas;

		UBorder* Box = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptBorder"));
		Box->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Box->SetPadding(FMargin(24.f, 12.f));
		Box->AddChild(MakeText(Tree, TEXT("PromptText"), TEXT("[F] 대화 시작"), 26));
		Canvas->AddChild(Box);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Box->Slot))
		{
			S->SetAnchors(FAnchors(0.5f, 1.f));
			S->SetAlignment(FVector2D(0.5f, 1.f));
			S->SetPosition(FVector2D(0.f, -140.f));
			S->SetAutoSize(true);
		}

		Compile(Prompt);
	}

	// 4) BP_InGamePC — AInGamePlayerController + 에셋 배선
	UBlueprint* InGamePC = LoadOrCreateBP(TEXT("/Game/InGame"), TEXT("BP_InGamePC"), AInGamePlayerController::StaticClass());
	Compile(InGamePC);
	SetObjectDefault(InGamePC, TEXT("InteractAction"), IA);
	SetObjectDefault(InGamePC, TEXT("InteractMappingContext"), IMC);
	SetClassDefault(InGamePC, TEXT("ChatWidgetClass"), Chat->GeneratedClass);
	SetClassDefault(InGamePC, TEXT("PromptWidgetClass"), Prompt->GeneratedClass);

	// 5) BP_InGameGM — AInGameGameMode, Pawn=BP_ThirdPersonCharacter, PC=BP_InGamePC
	UBlueprint* ThirdPersonChar = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter"));
	if (!ThirdPersonChar)
	{
		UE_LOG(LogTemp, Error, TEXT("[LobbyGen] BP_ThirdPersonCharacter not found."));
		return;
	}
	UBlueprint* InGameGM = LoadOrCreateBP(TEXT("/Game/InGame"), TEXT("BP_InGameGM"), AInGameGameMode::StaticClass());
	Compile(InGameGM);
	SetClassDefault(InGameGM, TEXT("DefaultPawnClass"), ThirdPersonChar->GeneratedClass);
	SetClassDefault(InGameGM, TEXT("PlayerControllerClass"), InGamePC->GeneratedClass);

	UE_LOG(LogTemp, Display, TEXT("[LobbyGen] Built IA/IMC_Interact, WBP_InGameChat, WBP_InteractPrompt, BP_InGamePC, BP_InGameGM"));
}

#else

void ULobbyAssetGenerator::GenerateLobbyAssets()
{
}

void ULobbyAssetGenerator::GenerateRoomWiring()
{
}

void ULobbyAssetGenerator::GenerateNpcChatAssets()
{
}

void ULobbyAssetGenerator::GenerateInGameAssets()
{
}

#endif // WITH_EDITOR
