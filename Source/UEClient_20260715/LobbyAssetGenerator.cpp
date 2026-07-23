#include "LobbyAssetGenerator.h"

#if WITH_EDITOR

#include "LobbyWidget.h"
#include "RoomWidget.h"
#include "RoomListEntryWidget.h"
#include "LobbyPlayerController.h"
#include "LobbyGameMode.h"

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

		VAdd(Root, MakeButton(Tree, TEXT("LeaveButton"), TEXT("나가기")), ESlateSizeRule::Automatic, HAlign_Center, VAlign_Bottom, FMargin(0));

		Compile(Room);
	}

	// 3) WBP_Lobby — 로비: 프로필 + 방 만들기 + 방 목록
	UWidgetBlueprint* Lobby = LoadOrCreateWidgetBP(TEXT("WBP_Lobby"), ULobbyWidget::StaticClass());
	{
		UWidgetTree* Tree = Lobby->WidgetTree;
		WipeTree(Tree);
		UVerticalBox* Root = BuildScreenRoot(Tree, ColorBg, 60.f);

		VAdd(Root, MakeText(Tree, TEXT("TitleText"), TEXT("로비 (Lobby)"), 48), ESlateSizeRule::Automatic, HAlign_Left, VAlign_Top, FMargin(0, 0, 0, 8));
		VAdd(Root, MakeText(Tree, TEXT("NicknameText"), TEXT("닉네임"), 24, FLinearColor(0.6f, 0.85f, 1.f, 1.f)), ESlateSizeRule::Automatic, HAlign_Left, VAlign_Top, FMargin(0, 0, 0, 24));

		// 상단 바: 방 이름 입력 + 방 만들기 + 새로고침
		UHorizontalBox* TopBar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
		VAdd(Root, TopBar, ESlateSizeRule::Automatic, HAlign_Fill, VAlign_Top, FMargin(0, 0, 0, 24));

		UEditableTextBox* Input = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RoomNameInput"));
		Input->SetHintText(FText::FromString(TEXT("방 이름을 입력하세요")));
		HAdd(TopBar, Input, ESlateSizeRule::Fill, VAlign_Center, FMargin(0, 0, 16, 0));
		HAdd(TopBar, MakeButton(Tree, TEXT("CreateRoomButton"), TEXT("방 만들기")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(8, 0));
		HAdd(TopBar, MakeButton(Tree, TEXT("RefreshButton"), TEXT("새로고침")), ESlateSizeRule::Automatic, VAlign_Center, FMargin(8, 0, 0, 0));

		// 방 목록 (남은 공간 전부)
		UScrollBox* RoomList = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RoomListBox"));
		VAdd(Root, RoomList, ESlateSizeRule::Fill, HAlign_Fill, VAlign_Fill, FMargin(0));

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

#else

void ULobbyAssetGenerator::GenerateLobbyAssets()
{
}

void ULobbyAssetGenerator::GenerateRoomWiring()
{
}

#endif // WITH_EDITOR
