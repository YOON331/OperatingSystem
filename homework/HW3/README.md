# Multithreaded Mergesort

- 이 Multithreaded Mergesort 프로그램은 n개의 데이터 수와 thread의 수를 입력받아 Mergesort를 수행하는 기능을 제공합니다.


## Features
1. thread가 실행할 함수(`worker`)
- task의 t_status(CREATING, SORTING, MERGING)에 따라 해당하는 작업을 수행합니다.
  - task의 t_status가 `CREATING` 이면서 status가 `UNDONE` 이면, 각 thread가 병렬적으로 `랜덤한 실수의 값으로 데이터를 초기화`합니다.
  - task의 t_status가 `SORTING` 이면서 status가 `UNDONE` 이면, 각 thread가 병렬적으로 `merge_sort 함수를 실행`합니다.
  - task의 t_status가 `MERGING` 이면서 status가 `UNDONE` 이면, 각 thread가 병렬적으로 `merge_lists 함수를 실행`합니다. n_merge값이 홀수이면 merge_lists를 한 번 수행하고 
    짝수이면 t_status와 status를 초기화하여 스레드가 반복적으로 작업을 수행하도록 합니다.


## Usage 
- Multithreaded Mergesort를 사용하려면 아래의 내용을 따라주세요. (정렬결과를 확인하시려면 -DDEBUG옵션을 추가하시면 됩니다.)
  - Makefile은 다음과 같은 내용을 포함하고 있습니다.
  - all : pmergesort.c `소스파일명.c`

    gcc -o pmergesort pmergesort.c -pthread

    gcc -o `빌드한 프로그램명` `소스파일명.c` -pthread


  - clean : 

    rm -rf pmergesort

    rm -rf `빌드한 프로그램명`

- makefile의 실행은 commandline에 `make`를 입력하여 수행합니다.
- 빌드가 완료되면 실행 파일을 실행하기 위해 `./<빌드한 프로그램명> -d <# data elements 데이터의 수> -t <# threads 스레드의 수>`을 입력합니다.
- 빌드한 프로그램을 삭제하려면 commandline에 `make clean` 을 입력하면 됩니다.