# Multithreaded N-Queen Solver
- 이 Multithreaded N-Queen Solver 프로그램은 n개의 퀸의 개수와 t개의 thread의 개수를 입력받아 n*n 체스판에서 서로 공격할 수 없도록 배치하는 경우의 수를 t개의 thread가 병렬로 계산하는 기능을 제공합니다. 


## Features
1. thread가 실행할 작업을 생성하는 함수(`producer`)
- `producer`는 Single-Thread가 작업을 생성합니다.
  - board size에서 먼저 4개의 퀸을 두고 서로 공격하지 않는 경우의 수를 작업큐에 넣는 역할을 수행합니다.
  - bounded buffer에 값을 넣고 buffer의 크기가 수용가능한 용량보다 크면 `queue_cv`의 시그널을 기다립니다. 
  - buffer의 크기가 수용가능한 용량보다 작다면 `dequeue_cv` 시그널을 보냅니다.
  - `producer_done`을 활용하여 모든 작업 생성이 완료되었는지 확인합니다. 완료 되었으면 `pthread_cond_broadcast`를 사용하여 대기중인 thread가 작업을 시작하도록 합니다.

2. 생성된 작업을 thread가 수행하는 함수(`consumer`)
- `consumer`은 `producer`가 생성한 작업을 큐에서 빼면서 t개의 thread가 각각의 작업을 수행하면 정해진 경우의 수를 찾는 역할을 수행합니다.
  - 작업큐로부터 가져와서 저장되어 있던 경우의 수를 바탕으로 board size의 퀸이 서로 공격하지 않는 경우의 수를 찾습니다. 이때 thread 간 중복된 값을 계산하지 않도록 start를 활용하여 `latest_queen`의 값과 `start`의 값이 같으면 반복문이 종료됩니다.
  - buffer의 작업수가 0보다 작거나 `producer`의 작업 생성이 끝나지 않았다면 `dequeue_cv` 시그널을 기다립니다.
  - 그렇지 않다면 작업을 수행하기 위해 `queue_cv` 시그널을 보내고 스택을 반환합니다.

3. 유효성 검사(`is_feasible`)
- 체스판 위에 놓인 퀸들의 배치가 유효한지(퀸들이 서로를 공격할 수 없는지) 확인합니다

4. signal(ctrl+c)을 처리하는 함수 (`handle_signal`)
- 실행중인 `ctrl+ c` 를 입력하면 현재까지 찾은 `N-Queen의 경우의 수`와 `실행된 시간`을 출력합니다.


## Usage 
- Multithreaded N-Queen Solver를 사용하려면 아래의 내용을 따라주세요. 
  - Makefile은 다음과 같은 내용을 포함하고 있습니다.
  - all : stack.h stack.c nqueens.c

    gcc -o nqueens nqueens.c stack.c -pthread

  - clean : 

    rm -rf nqueens stack.o

- makefile의 실행은 commandline에 `make`를 입력하여 수행합니다.
- 빌드가 완료되면 실행 파일을 실행하기 위해 `./<progeam name 빌드한 프로그램명> -n <# number of queens queens의 개수> -t <# number of threads 스레드의 수>`을 입력합니다.

- 빌드한 프로그램을 삭제하려면 commandline에 `make clean` 을 입력하면 됩니다.