/* 
   2024 운영체제 홍신 교수님 HW3 
   - 멀티스레딩을 이용하여 병합정렬 구현

   - 소프트웨어학과 2021041043 박태영 
   - 소프트웨어학과 2021041091 윤별
   - 소프트웨어학과 이동규

*/

/* 개선사항

1. 기존 코드에서 아래와 같이 메인스레드가 다른 스레드의 작업이 종료되었는지 계속 확인하던 것을 signal을 이용해서 구현하였습니다.
	while (n_done < max_tasks) {
		pthread_mutex_unlock(&m) ;
		pthread_mutex_lock(&m) ;
	}
2. 병합을 멀티스레딩으로 처리하기 위해 인접한 메모리끼리 병합시켜서 유실되는 자료의 값이나 버퍼 오버플로우가 발생하지 않게 만들었습니다.
   또한 n_tasks값을 키워나가면서 worker에게 signal을 주지 않고 n_tasks값을 구한 다음 siganl을 주어 스레드에게 불필요한 신호를 주지 않습니다.
*/ 

/* 프로그램 진행 과정
생성 정렬 병합 
======================================================================================================================
구현
1. 스레드 생성
2. 스레드별로 초기화할 메모리공간 지정 및 멀티스레딩으로 데이터 생성
3. (2)가 모두 끝난 경우 멀티스레딩으로 정렬
4. 멀티스레딩으로 정렬된 배열 병합
5. 결과 출력
======================================================================================================================
기타: 
   -  OS의 스케줄링으로 멀티스레딩이 제대로 동작하는지 확인하기 힘들었는데 
      102번줄의 출력을 시켜야지 제대로 병합에서 멀티스레딩이 동작하는 것을 확인할 수 있었습니다.
      102:: printf("[Thread %ld] is starts Task %d\n", pthread_self(), i) ;
   -  -DDEBUG 옵션으로 컴파일하면 정렬결과가 제대로 되었는지 확인하며 정렬 결과를 출력합니다.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define max_tasks 200 

double *data ; //정렬할 데이터

enum task_status { UNDONE, PROCESS, DONE } ;
enum task_name {CREATING, SORTING, MERGING }; //스레드가 실행할 작업의 종류

struct task {
   int t_status; //creating_task, merging_task, sorting_task
   double * a ; //데이터 시작위치
   int n_a ; //데이터 수 
   int status ; //스레드 상태
} ;

struct task tasks[max_tasks] ;
int n_tasks = 0 ;
int n_undone = 0 ;
int n_done = 0 ;
int mrgdiv = 1; //병합할 task를 관리하는 변수
int n_merge =0; //merge task 수
int n_data =0; //data 수
int keep_running = 1; // 스레드가 계속 실행될지 여부를 나타내는 플래그-> pthread_join을 쓰기위해 사용

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ; // thread synchonization에 사용
pthread_cond_t cv = PTHREAD_COND_INITIALIZER ; // 스레드 제어 신호
pthread_cond_t mainThread = PTHREAD_COND_INITIALIZER ; // 메인 스레드 제어 신호

void merge_lists (double * a1, int n_a1, double * a2, int n_a2) ;
void merge_sort (double * a, int n_a) ;
void create_list (void); //data를 위한 메모리할당

void *worker (void * ptr) {
    while (keep_running) {//무한루프 -> 종료시켰다 깨우려고 사용
        pthread_mutex_lock(&m);
        while (n_undone == 0 && keep_running) {
            pthread_cond_wait(&cv, &m);
        }
        if (!keep_running) {
            pthread_mutex_unlock(&m);
            break;
        }

        int i;
        for (i = 0; i < n_tasks; i++) {//실행할 작업 찾기
            if (tasks[i].status == UNDONE) 
                break;
        }
        if (i == n_tasks) { //못찾았으면 대기
            pthread_mutex_unlock(&m);
            continue;
        }

        tasks[i].status = PROCESS;
        n_undone--;
        pthread_mutex_unlock(&m);
         // printf("[Thread %ld] is starts Task %d\n", pthread_self(), i) ;
        if(tasks[i].t_status == CREATING) {
//생성
            printf("[Thread %ld] Creating starts Task %d\n", pthread_self(), i) ;
            for (int j = 0; j < tasks[i].n_a; j++) {
                int num = rand();
                int den = rand();
                if (den != 0.0)         
                    *(tasks[i].a + j) = ((double) num) / ((double) den);
                else
                    *(tasks[i].a + j) = ((double) num);
            }
            printf("[Thread %ld] Creating completed Task %d\n", pthread_self(), i) ;
        } 
        else if(tasks[i].t_status == SORTING) {
//정렬
            printf("[Thread %ld] Sorting starts Task %d\n", pthread_self(), i) ;
            merge_sort(tasks[i].a, tasks[i].n_a);
            printf("[Thread %ld] Sorting completed Task %d\n", pthread_self(), i) ;
        } 
        else if(tasks[i].t_status == MERGING) {  
//병합
            pthread_mutex_lock(&m);
            printf("[Thread %ld] starts %d merging\n", pthread_self(), i) ;
            merge_lists(tasks[i].a, tasks[i].n_a, tasks[i+mrgdiv].a, tasks[i+mrgdiv].n_a);//mrgdiv가 몇 개씩 병합할지 정함
            tasks[i+mrgdiv].status = DONE;
            tasks[i].n_a += tasks[i+mrgdiv].n_a;    
            printf("[Thread %ld] completed %d merging\n", pthread_self(), i) ;
            pthread_mutex_unlock(&m);
        }

        //작업 종료시 상태바꾸고 main에 신호
        if(tasks[i].t_status == MERGING && n_done == n_merge) {//병합은 n_merge가 모두 종료된 후에 다음 작업으로 넘아가야함.
            pthread_mutex_lock(&m);
            tasks[i].status = DONE;
            n_done++;
            pthread_cond_signal(&mainThread); //작업 받을 준비 됐음알림
            pthread_mutex_unlock(&m);
        } 
        else {
            pthread_mutex_lock(&m);
            tasks[i].status = DONE;
            n_done++;
            pthread_cond_signal(&mainThread); //작업 받을 준비 됐음알림
            pthread_mutex_unlock(&m);
        }
    }
    return NULL;
}

int main (int argc, char** argv) {
    struct timeval start, end, crtST, sortST, mrgST;

    gettimeofday(&start, NULL);
    srand(start.tv_usec * start.tv_sec);

    int c, n_threads;
//입력받기
    while ((c = getopt(argc, argv, ":d:t:")) != -1) {
        switch (c) {
            case 'd':
                n_data = atoi(optarg);
                break;
            case 't':
                n_threads = atoi(optarg);
                break;
            case ':':
                printf("Usage: ./pmergesort -d <# data elements> -t <# threads>\n");
                exit(1);
            case '?':
                printf("Usage: ./pmergesort -d <# data elements> -t <# threads>\n");
                exit(1);
            default:
                printf("Usage: ./pmergesort -d <# data elements> -t <# threads>\n");
                exit(1);
        }
    }

    // 옵션확인
    if (n_data == 0 || n_threads == 0) {
        printf("Usage: ./pmergesort -d <# data elements> -t <# threads>\n");
        exit(1);
    }

    // 이상한 옵션확인
    if (optind < argc) {
        printf("option arguments are not allowed: ");
        for (int i = optind; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
        printf("Usage: ./pmergesort -d <# data elements> -t <# threads>\n");
        exit(1);
    }

    create_list();  // data 메모리 할당

//worker함수를 실행하는 스레드 생성
    pthread_t threads[n_threads];
    for (int i = 0; i < n_threads; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

//숫자 생성 작업 할당
    gettimeofday(&crtST, NULL);
    for (int i = 0; i < max_tasks; i++) {
        pthread_mutex_lock(&m); //여기 다음은 하나의 스레드만 지나 갈 수 있다.
        tasks[n_tasks].a = data + (n_data / max_tasks) * n_tasks; //배열 시작 위치
        tasks[n_tasks].n_a = n_data / max_tasks; //데이터 수
        if (n_tasks == max_tasks - 1) 
            tasks[n_tasks].n_a += n_data % max_tasks; //예외 처리
        tasks[n_tasks].t_status = CREATING;
        tasks[n_tasks].status = UNDONE; //상태 초기화
        n_undone++; //처리할 작업 수 증가 및 스레드 제어
        n_tasks++; //총 작업 수 증가
        pthread_cond_signal(&cv); //스레드가 실행중인 worker에게 작업이 준비되었음을 알림 
        //n_undone !=0 이므로 대기하고 있는 스레드가 있으면 작업 실행
        pthread_mutex_unlock(&m); // 다른 스레드들도 작업 할당
    }

    //대기 시작
    pthread_mutex_lock(&m);// 생성한 작업을 모두 끝낼때 까지 대기
    while (n_done < n_tasks) { // 생성중 정렬하면 안됨
        pthread_cond_wait(&mainThread, &m); //신호를 받을 때(작업이 끝날 때)까지 대기
    }
    pthread_mutex_unlock(&m);

//생성 모두 종료 => n_undone =0

//정렬 작업 할당
    gettimeofday(&sortST, NULL);
    n_done = 0;
    pthread_mutex_lock(&m); //여기 다음은 하나의 스레드만 지나 갈 수 있다.
    
    for (int i = 0; i < max_tasks; i++) {
        //정렬 위치와 데이터 수는 초기화에서 지정함
        tasks[i].t_status = SORTING;
        tasks[i].status = UNDONE; //상태 초기화
        n_undone++; //처리 시작한 작업 수 증가
    }
    pthread_mutex_unlock(&m); // n_undone을 한 번에 키워서 스레드에게 작업 전달
    
    for (int i = 0; i < n_threads; i++) {//신호 전달
        pthread_mutex_lock(&m); //한 번에 하나의 스레드만 깨운다.
        pthread_cond_signal(&cv); //스레드가 실행중인 worker에게 작업이 준비되었음을 알림 
        pthread_mutex_unlock(&m); // 다른 스레드들에게도 작업 할당
    }

   //대기 시작
    while (n_done < n_tasks) { // 생성한 작업을 모두 끝낼때 까지 대기
        pthread_mutex_lock(&m);
        pthread_cond_wait(&mainThread, &m);
        pthread_mutex_unlock(&m);
    }
//정렬 종료

//병합 작업 할당
    gettimeofday(&mrgST, NULL);
    //n_merge 200 201 -> 100 101 -> 100 -> 50
    //병합방식 11 22 44 88 
    n_merge = n_tasks;
    mrgdiv = 1;
    while (n_merge > 1) { //3이나 2면 종료
        printf("Merge %d at a time\n", mrgdiv);
        if (n_merge % 2 == 1) { //병합 작업이 홀수 개면 짝수로 바꿈
            //50-> 25 -> 24 -> 12 
            int k1 = mrgdiv * (n_merge - 2);
            int k2 = mrgdiv * (n_merge - 1);
            printf("[MainThread %ld] starts %d merging\n", pthread_self(), k1);
            merge_lists(tasks[k1].a, tasks[k1].n_a, tasks[k2].a, tasks[k2].n_a);
            printf("%d +++ %d\n", k1, k2);
            tasks[k1].n_a += tasks[k2].n_a;
            tasks[k2].status = DONE;
            tasks[k1].status = DONE;
            printf("[MainThread %ld] completed %d merging\n", pthread_self(), k1);
        }
        n_merge = n_merge / 2; //병합 작업 시행횟수: n_merge
        n_done = 0;

        for (int i = 0; i < n_merge; i++) {
            tasks[i * mrgdiv * 2].t_status = MERGING;
            tasks[i * mrgdiv * 2].status = UNDONE; //상태 초기화    
            n_undone++; //처리할 작업 수 증가
        }

        for (int i = 0; i < n_threads; i++) {//각 스레드별로 신호보냄
            pthread_mutex_lock(&m);
            pthread_cond_signal(&cv); //병합준비 마친후 신호 보냄   
            pthread_mutex_unlock(&m);
        }

        pthread_mutex_lock(&m);
        while (n_done < n_merge) {
            pthread_cond_wait(&mainThread, &m); //신호를 받을 때(작업이 끝날 때)까지 대기
        }
        pthread_mutex_unlock(&m);
        mrgdiv *= 2;
    }
//병합 종료
    gettimeofday(&end, NULL);

//정렬 됐는지 확인하면서 출력
#ifdef DEBUG   
    int k = 0;
    for (; k < n_data - 1;) {
        if (data[k] <= data[k + 1]) {
            printf("%lf ", data[k++]);
        }
    }
    printf("%lf ", data[n_data - 1]);
    printf("\n++%d DATA IS SORTED.++\n", ++k);
#endif
//결과출력
    free(data);
    printf("Creating Time: %lfms\n", (sortST.tv_sec - crtST.tv_sec) * 1000.0 + (sortST.tv_usec - crtST.tv_usec) / 1000.0);
    printf("Sorting Time: %lfms\n", (mrgST.tv_sec - sortST.tv_sec) * 1000.0 + (mrgST.tv_usec - sortST.tv_usec) / 1000.0);
    printf("Merging Time: %lfms\n", (end.tv_sec - mrgST.tv_sec) * 1000.0 + (end.tv_usec - mrgST.tv_usec) / 1000.0);
    printf("Total Time: %lfms\n", (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0);

    keep_running = 0;
    pthread_mutex_lock(&m);
    for (int i = 0; i < n_threads; i++) {
       pthread_cond_signal(&cv); // 하나씩 스레드를 종료시키기 위해 깨움
    }
    pthread_mutex_unlock(&m);

    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }
//종료
    return EXIT_SUCCESS;
}

void merge_lists(double *a1, int n_a1, double *a2, int n_a2) {
    double *a_m = (double *)calloc(n_a1 + n_a2, sizeof(double));
    int i = 0;
    int top_a1 = 0;
    int top_a2 = 0;

    for (i = 0; i < n_a1 + n_a2; i++) {
        if (top_a2 >= n_a2) {
            a_m[i] = a1[top_a1];
            top_a1++;
        } else if (top_a1 >= n_a1) {
            a_m[i] = a2[top_a2];
            top_a2++;
        } else if (a1[top_a1] < a2[top_a2]) {
            a_m[i] = a1[top_a1];
            top_a1++;
        } else {
            a_m[i] = a2[top_a2];
            top_a2++;
        }
    }
    memcpy(a1, a_m, (n_a1 + n_a2) * sizeof(double));
    free(a_m);
}

void merge_sort(double *a, int n_a) {
    if (n_a < 2)
        return;

    double *a1;
    int n_a1;
    double *a2;
    int n_a2;

    a1 = a;
    n_a1 = n_a / 2;
    a2 = a + n_a1;
    n_a2 = n_a - n_a1;

    merge_sort(a1, n_a1);
    merge_sort(a2, n_a2);
    merge_lists(a1, n_a1, a2, n_a2);
}

void create_list() {
    data = (double *)malloc(n_data * sizeof(double));

    if (data == NULL) {
        printf("malloc error\n");
        exit(0);
    }
}
