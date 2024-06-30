#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt
#include <pthread.h>
#include <sys/time.h>   // for gettimeofday
#include <signal.h> // for signal handling
#include "stack.h"

int BOARD_SIZE = 0;
int cnt = 0;
int threadNum = 0;
struct timeval start, end;

typedef struct {
    pthread_cond_t queue_cv;
    pthread_cond_t dequeue_cv;
    pthread_mutex_t lock;
    struct stack_t **queue;
    int capacity;
    int num;
    int front;
    int rear;
    int done;
} bounded_buffer;

bounded_buffer *buf;

int row(int cell) {
    return cell / BOARD_SIZE;
}

int col(int cell) {
    return cell % BOARD_SIZE;
}

int is_feasible(struct stack_t *queens) {
    int board[BOARD_SIZE][BOARD_SIZE] ;
	int c, r ;

	for (r = 0 ; r < BOARD_SIZE ; r++) {
		for (c = 0 ; c < BOARD_SIZE ; c++) {
			board[r][c] = 0 ;
		}
	}

	for (int i = 0 ; i < get_size(queens) ; i++) {
		int cell ;
		get_elem(queens, i, &cell) ;
		
		int r = row(cell) ;
		int c = col(cell) ;
	
		if (board[r][c] != 0) {
			return 0 ;
		}

		int x, y ;
		for (y = 0 ; y < BOARD_SIZE ; y++) {
			board[y][c] = 1 ;
		}
		for (x = 0 ; x < BOARD_SIZE ; x++) {
			board[r][x] = 1 ;
		}

		y = r + 1 ; x = c + 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y += 1 ; x += 1 ;
		}

		y = r + 1 ; x = c - 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y += 1 ; x -= 1 ;
		}

		y = r - 1 ; x = c + 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y -= 1 ; x += 1 ;
		}

		y = r - 1 ; x = c - 1 ;
		while (0 <= y && y < BOARD_SIZE && 0 <= x && x < BOARD_SIZE) {
			board[y][x] = 1 ;
			y -= 1 ; x -= 1 ;
		}
	}

	return 1;
}

void print_placement(struct stack_t *queens) {
    for (int i = 0; i < get_size(queens); i++) {
        int queen;
        get_elem(queens, i, &queen);
        printf("[%d,%d] ", row(queen), col(queen));
    }
    printf("\n");
}

void bounded_buffer_init(bounded_buffer *buf, int capacity) {
    pthread_cond_init(&(buf->queue_cv), NULL);
    pthread_cond_init(&(buf->dequeue_cv), NULL);
    pthread_mutex_init(&(buf->lock), NULL);
    buf->capacity = capacity;
    buf->queue = malloc(sizeof(struct stack_t *) * capacity);
    buf->num = 0;
    buf->front = 0;
    buf->rear = 0;
    buf->done = 0; 
}

void bounded_buffer_queue(bounded_buffer *buf, struct stack_t *queens) {
    pthread_mutex_lock(&(buf->lock));

    while (!(buf->num < buf->capacity)) {
        pthread_cond_wait(&(buf->queue_cv), &(buf->lock));
    }

    buf->queue[buf->rear] = queens;
    buf->rear = (buf->rear + 1) % buf->capacity;
    buf->num += 1;

    pthread_cond_signal(&(buf->dequeue_cv));
    pthread_mutex_unlock(&(buf->lock));
}

struct stack_t *bounded_buffer_dequeue(bounded_buffer *buf) {
    struct stack_t *r = NULL;

    pthread_mutex_lock(&(buf->lock));
    while (!(buf->num > 0) && !buf->done) { 
        pthread_cond_wait(&(buf->dequeue_cv), &(buf->lock));
    }

    if (buf->done) { 
        pthread_mutex_unlock(&(buf->lock));
        return NULL;
    }

    r = buf->queue[buf->front];
    buf->front = (buf->front + 1) % buf->capacity;
    buf->num -= 1;
    pthread_cond_signal(&(buf->queue_cv));
    pthread_mutex_unlock(&(buf->lock));

    return r;
}

void *producer(void *ptr) {
    pthread_t tid = pthread_self();
    int N = 4;
    struct stack_t *queens = create_stack(BOARD_SIZE);
    printf("Producer %ld created a task.\n\n", (long)tid);
    if (BOARD_SIZE == 1) {
        push(queens, 0);
        if (is_feasible(queens)) {
            cnt++; 
            printf("[thread %ld] : ", (long)pthread_self());
            print_placement(queens);
        }
        delete_stack(queens);

        pthread_mutex_lock(&(buf->lock));
        buf->done = 1;
        pthread_cond_broadcast(&(buf->dequeue_cv));
        pthread_mutex_unlock(&(buf->lock));

        return NULL;
    }
    push(queens, 0);
    while (!is_empty(queens)) {
        int latest_queen;
        top(queens, &latest_queen);

        if (latest_queen == BOARD_SIZE * BOARD_SIZE) {
            pop(queens, &latest_queen);
            if (!is_empty(queens)) {
                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                break;
            }
            continue;
        }

        if (is_feasible(queens)) {
            if (get_size(queens) == N) {
                struct stack_t *new_queens = create_stack(BOARD_SIZE);
                for (int i = 0; i < get_size(queens); i++) {
                    int queen;
                    get_elem(queens, i, &queen);
                    push(new_queens, queen);
                }
                bounded_buffer_queue(buf, new_queens);
                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            } else {
                push(queens, latest_queen + 1);
            }
        } else {
            pop(queens, &latest_queen);
            push(queens, latest_queen + 1);
        }
    }
    
    pthread_mutex_lock(&(buf->lock));
    buf->done = 1;
    pthread_cond_broadcast(&(buf->dequeue_cv));
    pthread_mutex_unlock(&(buf->lock));

    delete_stack(queens);
    return NULL;
}

void *consumer(void *ptr) {
    pthread_t tid = pthread_self();
    int N = BOARD_SIZE;

    while (1) { 
        struct stack_t *prep = bounded_buffer_dequeue(buf);
        if (prep == NULL) {
            break; 
        }

        struct stack_t *queens = create_stack(BOARD_SIZE);

        queens->capacity = prep->capacity;
        queens->size = prep->size;
        memcpy(queens->buffer, prep->buffer, prep->size * sizeof(int));
        int start;
        top(prep, &start);
        delete_stack(prep);

        while (queens->size <= N) {
            int latest_queen;
            top(queens, &latest_queen);

            if (latest_queen == BOARD_SIZE * BOARD_SIZE) {
                pop(queens, &latest_queen);
                if (!is_empty(queens)) {
                    pop(queens, &latest_queen);
                    if (start == latest_queen) break;
                    push(queens, latest_queen + 1);
                } else {
                    break;
                }
                continue;
            }

            if (is_feasible(queens)) {
                if (get_size(queens) == N) {
                    pthread_mutex_lock(&(buf->lock));
                    printf("[thread %ld] : ", (long)tid);
                    print_placement(queens);
                    cnt++;
                    pthread_mutex_unlock(&(buf->lock));
                    pop(queens, &latest_queen);
                    push(queens, latest_queen + 1);
                } else {
                    push(queens, latest_queen + 1);
                }
            } else {
                pop(queens, &latest_queen);
                push(queens, latest_queen + 1);
            }
        }
        delete_stack(queens);
    }

    return NULL;
}

void handle_signal(int sig) {
    gettimeofday(&end, NULL);
    printf("\nN-Queens(%dx%d) Total number of found arrangements: %d\n", BOARD_SIZE, BOARD_SIZE, cnt);
    printf("Ctrl + C signal Termination Time: %lfs\n", (double)((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0));
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    pthread_t prod;
    pthread_t *cons;
    int c;

    signal(SIGINT, handle_signal);

    while ((c = getopt(argc, argv, ":n:t:")) != -1) {
        switch (c) {
            case 'n':
                BOARD_SIZE = atoi(optarg);
                break;
            case 't':
                threadNum = atoi(optarg);
                break;
            case '?':
            case ':':
            default:
                printf("Usage: nqueens -n -n <# number of queens> -t <# number of threads>\n");
                exit(EXIT_FAILURE);
        }
    }

    if (BOARD_SIZE <= 0 || threadNum == 0) {
        printf("Usage: nqueens -n <BOARD_SIZE> -t <thread number>\nNumber must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start, NULL);

    buf = malloc(sizeof(bounded_buffer));
    bounded_buffer_init(buf, BOARD_SIZE * BOARD_SIZE);
    
    // 입력한 스레드의 개수만큼 메모리 할당
    cons = malloc(threadNum * sizeof(pthread_t));
    
    // producer 생성
    pthread_create(&prod, NULL, producer, NULL);
    
    // consumer 생성
    for (int i = 0; i < threadNum; i++) {
        pthread_create(&cons[i], NULL, consumer, NULL);
    }

    pthread_join(prod, NULL);
    for (int i = 0; i < threadNum; i++) {
        pthread_join(cons[i], NULL);
    }

    gettimeofday(&end, NULL); // 종료 시간 측정
    //총 가능한 개수와 측정 시간을 second로 출력
    printf("\nTotal possible solutions for N-Queens(%dx%d) = %d\n", BOARD_SIZE, BOARD_SIZE, cnt);
    printf("Execution Time: %lfs\n", (double)((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0));

    return EXIT_SUCCESS;
}
