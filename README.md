# RxSampleForUnreal
Welcome to the RxSample for Unreal Engine source code!

## Introduction
RxCpp를 Unreal Engine에서 사용해보는 샘플 프로젝트입니다.  
Unreal Engine 환경에서 가능한 다양하게 Reactive programming을 적용하는 것이 목표입니다.  
다음과 같은 이유에서 Unreal Engine의 일반적인 사용 방식과는 맞지 않거나 매우 비효율적인 부분이 다소 있을 수 있습니다.

1. Rx 및 RxCpp 사용 경험 부족
2. 다양한 상황에서 익숙한 방식을 떠나 Rx 방식을 적용
3. 다른 언어의 Rx 라이브러리 대비 RxCpp의 부족한 구현 및 언어적 한계

## 목표
샘플 프로젝트 제작을 통해 실사용 레벨에서 적용하기 위해 필요한 부분들을 연구하고 최종적으로 UniRx 같은 UE 전용 Rx 라이브러리를 만드려고 합니다.

## Installation

### Build
1. UE5를 Epic Games Launcher 혹은 소스를 직접 빌드하여 설치합니다.
2. RxSample.uproject 을 우클릭하여 컨텍스트 메뉴를 띄우고 Switch Unreal Engine Version... 을 선택합니다.
3. 위에서 받은 UE 버전을 선택합니다.
4. RxSample.uproject 을 우클릭하여 컨텍스트 메뉴를 띄우고 Generate Visual Studio project files 를 선택합니다.
5. RxSample.sln 를 열고 빌드하여 실행합니다.
