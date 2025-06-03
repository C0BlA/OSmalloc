#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

typedef enum { BEST, WORST, FIRST } mode;

// 헤더 구조체
typedef struct header {
    size_t size;
    int used;
    struct header* next;  
} header;


#define HEADER_SIZE sizeof(header)

header* find_block(size_t size);
void* srealloc(void* ptr, size_t size);
void* Make_Addtional_Page(size_t size);
void sfree(void* ptr);
void sset_mode(mode m);
void*smalloc(size_t size);
void smcoalesce();



mode _mode = BEST;

static header* heap_start = NULL;
static size_t PAGE_SIZE = 4096;

// 메모리 할당
void* smalloc(size_t size) {
    if (size == 0) return NULL;
    header* block = find_block(size); //크기 맞는거 찾기
    if (!block) block = Make_Addtional_Page(size); // 없으면 새 페이지 요청
    if (!block) return NULL;

    // 분할
    if (block->size >= size + HEADER_SIZE + 8) {
        header* new_block = (header*)((char*)block + HEADER_SIZE + size);
        new_block->size = block->size - size - HEADER_SIZE;
        new_block->used = 0;
        new_block->next = block->next;

        block->size = size;
        block->next = new_block;
    }

    block->used = 1;
    return (void*)(block + 1);
    }

    void* srealloc(void* ptr, size_t size) {

        if (ptr == NULL) return smalloc(size);

        header* old_header = (header*)ptr - 1;

        if (size == 0) {
            sfree(ptr);
            return NULL;
        }


        header* curr = heap_start;
        int found = 0;
        while (curr) {
            if (curr == old_header) {
                found = 1;
                break;
            }
            curr = curr->next;
        }
        if (!found) abort();

        if (old_header->size >= size) {
            return ptr;
        }



        void* new_ptr = smalloc(size);
        if (!new_ptr) return NULL;

        memcpy(new_ptr, ptr, old_header->size);
        sfree(ptr);
        return new_ptr;
    }


void* Make_Addtional_Page(size_t size) {

    size_t total_size = ((size + HEADER_SIZE + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE;

    //읽기/쓰기 권한 + 순수 메모리 +프로세스 전용
    void* page = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (page == MAP_FAILED) return NULL;

    header* h = (header*)page;
    h->size = total_size - HEADER_SIZE;
    h->used = 0;
    h->next = NULL;

    // 힙에 연결
    if (!heap_start) {
        heap_start = h;
    } else {
        header* curr = heap_start;
        while (curr->next) curr = curr->next;
        curr->next = h;
    }
    return h;
}

header* find_block(size_t size) {
    header* curr = heap_start;
    header* best = NULL; // 가장 사이즈가 잘 맞는 블록

    while (curr) {
        if (!curr->used && curr->size >= size) { //사이즈 확인 
            if (_mode == FIRST) return curr;

            // 최대/최솟값 찾기
            if (_mode == BEST && (!best || curr->size < best->size)) best = curr;
            if (_mode == WORST && (!best || curr->size > best->size)) best = curr;
        }
        curr = curr->next;
    }
    return best;
}



// 메모리 해제
void sfree(void* ptr) {
    if (!ptr) return;
    header* h = (header*)ptr - 1;
    h->used = 0;
}

// 모드 변경
void sset_mode(mode m) {
    _mode = m;
}

// 블록 병합
void smcoalesce() {
    header* curr = heap_start;
    while (curr && curr->next) {
        if (!curr->used && !curr->next->used) {
            curr->size += HEADER_SIZE + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}
/*---------------------위에가 malloc 관련----------------------*/


/*-------------------아래부터 maze (코드 관련)-----------------*/
typedef struct {
    short int vert;  // 세로
    short int horiz; // 가로
} offsets;


// DFS 방식으로 경로탐색
// 탐색 방향을 위한 offset
offsets move[8] = {
    {-1, -1},
    {-1, 0},
    {-1, 1},
    {0, 1},
    {1, 1},
    {1, 0},
    {1, -1},
    {0, -1}
};


//stack에 저장할 위치 정보
typedef struct {
    short int row;
    short int col;
    short int dir;
} element;




element *stack;         // 스택
int top = 0;            // 스택 포인터
int stackSize = 1000;   // 초기 스택 크기



int sizecheck(FILE *fp, int *n, int *m);
int mkmaze(FILE *fp, int n, int m, int **maze);
void initStack();
void push(element item);
element pop();
int solveMaze(int **maze, int **visited, int n, int m);
void printPath(int **visited, int n, int m);




int main() {

    FILE *fp = fopen("maze.txt", "r");

    if (fp == NULL) {
        printf("파일을 열 수 없습니다.\n");
        return 1;
    }

    int n = 0, m = 0; // n = 세로, m = 가로

    //파일의 미로의 가로, 세로 크기 확인
    sizecheck(fp, &n, &m);

    //미로 틀 제작
    int **maze = (int **)smalloc(sizeof(int *) * (n + 2));


    for (int i = 0; i < n + 2; i++) {
        maze[i] = (int *)smalloc(sizeof(int) * (m + 2));

        //밖으로 빠져나가지 않게
        //전체 1로 초기화
        for (int j = 0; j < m + 2; j++) {
            maze[i][j] = 1;
        }

    }


    int **visited = (int **)smalloc(sizeof(int *) * (n + 2));

    //방문한 미로(나중에는 결과 미로) 1로 채우기

    for (int i = 0; i < n + 2; i++) {
        visited[i] = (int *)smalloc(sizeof(int) * (m + 2));
        for (int j = 0; j < m + 2; j++) {
            visited[i][j] = 1;
        }

    }

    //C에서 사용할 수 있게 미로 제작
    mkmaze(fp, n, m, maze);

    /*
    //미로 확인용
    for (int i = 0; i < n + 2; i++) {
        for (int j = 0; j < m + 2; j++) {
            printf("%d", maze[i][j]);
        }
        printf("\n");
    }
    */

    //파일의 미로 보여주기
    printf("<<<INPUT MAZE>>>\n");

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            printf("%d", maze[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    //solvemaze로 maze를 풀어서 visited에 저장
    if (!solveMaze(maze, visited, n, m)) {
        printf("<<<FAIL>>>\n");
    }

    for (int i = 0; i < n + 2; i++) {
        sfree(maze[i]);
        sfree(visited[i]);
    }
    sfree(maze);
    sfree(visited);
    sfree(stack);


    fclose(fp);

    return 0;
}






//미로 크기 저장
int sizecheck(FILE *fp, int *n, int *m) {
    int c, i = 0, j = 0, sizem = 0;

    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            i++;
            if (j > sizem) sizem = j;
            j = 0;
        } else if (c == '0' || c == '1') {
            j++;
        }
    }

    *n = i + 1;
    *m = sizem;

    return 0;
}



// 미로 데이터를 읽어서 maze에 저장
// 주요기능:
// 문자 데이터를 사용하기 편하게 숫자로 변환
int mkmaze(FILE *fp, int n, int m, int **maze) {
    rewind(fp);
    int c;
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            do {
                c = fgetc(fp);
            } while (c != '0' && c != '1' && c != EOF);
            if (c == '0' || c == '1') {
                maze[i][j] = c - '0';
            }
        }
    }
    return 0;
}



// 스택 초기화
void initStack() {
    stack = (element *)smalloc(stackSize * sizeof(element));
}



// 스택에 element(위치 정보) push
void push(element item) {
    if (top >= stackSize) {
        stackSize *= 2;
        stack = (element *)srealloc(stack, stackSize * sizeof(element));
    }
    stack[top++] = item;

    //printf("Push: (%d, %d, %d)\n", item.row, item.col, item.dir);

}




// 스택에 위치정보 pop
// 막힌 길 갔을 때 사용
element pop() {
    if (top <= 0) {
        printf("<<STACK EMPTY>>\n");
        exit(EXIT_FAILURE);
    }
     //printf("Pop: (%d, %d, %d)\n", e.row, e.col, e.dir);

    return stack[--top];

}



//길 찾기
int solveMaze(int **maze, int **visited, int n, int m) {

    initStack();

    //초기 시작 위치 넣기
    //항상 고정되어 있음.
    element position = {1, 1, 0};

    //시작 위치 0으로 표시
    visited[1][1] = 0;
    push(position);


    int row, col, nextRow, nextCol, dir;

    while (top > 0) {
        position = pop();
        row = position.row;
        col = position.col;
        dir = position.dir;


        //8방향에 대해서 경로 탐색
        while (dir < 8) {

            nextRow = row + move[dir].vert;
            nextCol = col + move[dir].horiz;


            // printf("현재 위치: (%d, %d), dir: %d\n", row, col, dir);


            // 종료시점에 도달 했을경우
            // 지나온 길을 print 하면서 종료
            if (nextRow == n && nextCol == m) {
                visited[nextRow][nextCol] = 0;
                printf("<<<SOLUTION>>>\n");
                printPath(visited, n, m);
                return 1;
            }


            //maze에서는 "갈 수 있는 길"이여야 하고
            //visited에서는 "간 적 없는 길"이여야 함.
            //즉, 탐색하지 않은 길을 탐색

            if (maze[nextRow][nextCol] == 0 && visited[nextRow][nextCol] == 1) {
                visited[nextRow][nextCol] = 0;
                position.row = row;
                position.col = col;
                position.dir = dir + 1;
                push(position);

                row = nextRow;
                col = nextCol;
                dir = 0;
            }
            //그렇지 않은 경우라면,
            //다른 방향을 모색
            else {
                dir++;
            }
        }
    }

    return 0;

}


void printPath(int **visited, int n, int m) {

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            printf("%d", visited[i][j]);
        }
        printf("\n");
    }
}

