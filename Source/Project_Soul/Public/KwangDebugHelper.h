#pragma once

// Debug라는 NameSpace를 통해 다른 함수와 충돌을 방지
namespace Debug 
{
	// static : 객체를 생성하지 않고도 메모리에 상주하여 어디서든 Debug::Print로 즉시 호출 가능
	static void Print(const FString& Msg, const FColor& Color = FColor::MakeRandomColor(), int32 InKey = -1)
	{
		// GEninge : 언리얼 엔진의 코어 시스템을 가리키는 전역 포인터
		// 엔진이 정상적으로 로드되었는지 확인하는 안정장치
		if (GEngine)
		{
			// 게임 화면 좌측 상단에 텍스트를 출력하는 함수
			// InKey(-1: 매번 새 줄에 출력, 양수 : 해당 키 값의 줄을 덮어쓰기), 7.f(화면에 7초동안 머뭄), 색상, 메세지
			GEngine->AddOnScreenDebugMessage(InKey, 7.f, Color, Msg);

			// 에디터 하단에 출력 로그 창에 텍스트를 기록하는 매크로
			// LogTemp : 임시 카테고리, Warning : 노란색 경고 텍스트로 표시, TEXT("%s") : 문자열 포멧
			// *Msg: FString 객체를 UE_LOG가 읽을 수 있는 C스타일 문자열(TCHAR 배열)로 변환하는 역참조 연산자
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		}
	}

	static void Print(const FString& FloatTitle, float FloatValueToPrint, int32 InKey = -1, const FColor& Color = FColor::MakeRandomColor())
	{
		if (GEngine)
		{
			const FString FinalMsg = FloatTitle + TEXT(": ") + FString::SanitizeFloat(FloatValueToPrint);
			GEngine->AddOnScreenDebugMessage(InKey, 7.f, Color, FinalMsg);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *FinalMsg);
		}
	}
}