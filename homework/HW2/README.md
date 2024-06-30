# Dynamic Memory Allocation

- 이 Dynamic Memory Allocation 프로그램은 입력받은 크기의 Memory를 할당하는 함수로 모드에 따른 메모리 할당,메모리 해제, 메모리 재할당, 메모리 병합과 같은 기능을 제공합니다.


## Features
1. 메모리 할당(`smalloc(size)`)
- 입력받은 size의 memory를 할당합니다.
- 메모리 블록이 할당되지 않은 경우, 특정 크기의 새로운 메모리 블록을 할당합니다.
- 기존 메모리 블록에 여유 공간이 있는 경우, 해당하는 크기로 메모리 블록을 분할하여 할당합니다.

2. 모드별 메모리 할당 (`smalloc_mode(size, mode)`)
- 지정된 모드 (bestfit, worstfit, firstfit)에 따라 메모리를 할당합니다.
- 모드:
  - bestfit: 요청된 크기에 맞는 가장 작은 사용 가능한 메모리 블록을 선택합니다.
  - worstfit: 요청된 크기에 맞는 가장 큰 사용 가능한 메모리 블록을 선택합니다.
  - firstfit: 요청된 크기에 맞는 첫 번째 사용 가능한 메모리 블록을 선택합니다.

3. 메모리 해제 (`sfree(memory address)`)
- smalloc 또는 smalloc_mode에 의해 할당된 메모리를 해제합니다.
- 메모리 블록을 사용하지 않음으로 표시합니다.

4. 메모리 재할당 (`srealloc(memory address, size)`)
- 메모리 블록 크기를 입력받은 크기로 조정합니다.
- 크기가 증가하는 경우, 새로운 메모리 블록을 할당하고 이전 블록의 내용을 복사합니다.
- 크기가 감소하는 경우, 기존 블록의 크기를 업데이트하고 필요한 경우 분할합니다.

5. 메모리 병합 (`smcoalesce()`)
- 인접한 빈 메모리 블록을 결합하여 큰 메모리 블록으로 만듭니다.

6. 메모리 덤프 (`smdump()`)
- 사용 중인 메모리 슬롯 및 미사용 메모리 슬롯에 대한 정보를 표시합니다.
- 메모리 주소, 크기 및 메모리 내용의 16진수 표현을 표시합니다.


## Usage 
- Dynamic Memory Allocation을 사용하려면 아래의 내용을 따라주세요. (필요한 경우 Makefile 수정하면 됩니다.)
1. Makefile은 다음과 같은 내용을 포함하고 있습니다.
- all : smalloc.h smalloc.c (`소스파일명.c`)
  
  gcc -o (`빌드한 프로그램명`) (`소스파일명.c`) smalloc.

- clean : rm -rf (`빌드한 프로그램명`) (`오브젝트파일명.o`)
- makefile의 실행은 commandline에 `make`를 입력하여 수행합니다.
- 빌드가 완료되면 실행 파일을 실행하기 위해 `./(빌드한 프로그램명)`을 입력합니다.
