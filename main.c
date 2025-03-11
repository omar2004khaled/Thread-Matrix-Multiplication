#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX 20

int A[MAX][MAX], B[MAX][MAX], M[MAX][MAX]; // Shared Matrices
int r1, c1, r2, c2;

/* Structure for thread arguments */
typedef struct {
    int row, col;
} ThreadData;

void Readinput(const char *filename, int *rows, int *cols, int matrix[MAX][MAX]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening %s\n", filename);
        exit(1);
    }

    fscanf(file, "row=%d col=%d", rows, cols);
    
    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            fscanf(file, "%d", &matrix[i][j]);
        }
    }

    fclose(file);
}

void WriteMatrix(const char *filename, int rows, int cols, int matrix[MAX][MAX]) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening %s\n", filename);
        exit(1);
    }

    fprintf(file, "row=%d col=%d\n", rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(file, "%d ", matrix[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

void* multiplyElement(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    int row = data->row;
    int col = data->col;

    M[row][col] = 0;
    for (int k = 0; k < c1; k++) {
        M[row][col] += A[row][k] * B[k][col];
    }

    free(arg);
    pthread_exit(NULL);
}

void* multiplyRow(void* arg) {
    int row = (int) arg;
    
    for (int j = 0; j < c2; j++) {
        M[row][j] = 0;
        for (int k = 0; k < c1; k++) {
            M[row][j] += A[row][k] * B[k][j];
        }
    }

    free(arg);
    pthread_exit(NULL);
}

void* multiplyMatrix(void* arg) {
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c2; j++) {
            M[i][j] = 0;
            for (int k = 0; k < c1; k++) {
                M[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

void measureTime(void (method)(void), const char methodName) {
    struct timeval start, stop;
    gettimeofday(&start, NULL);
    method();
    gettimeofday(&stop, NULL);
    printf("%s Execution Time:\n", methodName);
    printf("Seconds taken: %lu\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken: %lu\n\n", stop.tv_usec - start.tv_usec);
}

void perElementMethod() {
    pthread_t elementThreads[MAX * MAX];
    int threadCount = 0;

    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c2; j++) {
            ThreadData* data = (ThreadData*) malloc(sizeof(ThreadData));
            data->row = i;
            data->col = j;
            pthread_create(&elementThreads[threadCount], NULL, multiplyElement, (void*) data);
            threadCount++;
        }
    }

    for (int i = 0; i < threadCount; i++) {
        pthread_join(elementThreads[i], NULL);
    }
    
    printf("Per Element Method: %d threads used\n", threadCount);
    WriteMatrix("c_per_element.txt", r1, c2, M);
}

void perRowMethod() {
    pthread_t rowThreads[MAX];
    int threadCount = r1;

    for (int i = 0; i < r1; i++) {
        int* row = (int*) malloc(sizeof(int));
        *row = i;
        pthread_create(&rowThreads[i], NULL, multiplyRow, (void*) row);
    }

    for (int i = 0; i < r1; i++) {
        pthread_join(rowThreads[i], NULL);
    }

    printf("Per Row Method: %d threads used\n", threadCount);
    WriteMatrix("c_per_row.txt", r1, c2, M);
}

void perMatrixMethod() {
    pthread_t matrixThread;
    pthread_create(&matrixThread, NULL, multiplyMatrix, NULL);
    pthread_join(matrixThread, NULL);
    printf("Per Matrix Method: 1 thread used\n");
    WriteMatrix("c_per_matrix.txt", r1, c2, M);
}

int main() {
    Readinput("a.txt", &r1, &c1, A);
    Readinput("b.txt", &r2, &c2, B);

    if (c1 != r2) {
        printf("Matrix multiplication not possible: cols of A (%d) != rows of B (%d)\n", c1, r2);
        return 1;
    }

measureTime(perElementMethod, "Thread Per Element");
    measureTime(perRowMethod, "Thread Per Row");
    measureTime(perMatrixMethod, "Thread Per Matrix");

    return 0;
}
