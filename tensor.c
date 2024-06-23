#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define MAX_ROWS 1000
#define MAX_COLS 1000
#define MAX_TENSOR_ROWS (MAX_ROWS * MAX_ROWS)
#define MAX_TENSOR_COLS (MAX_COLS * MAX_COLS)

typedef struct {
    int (*matrix1)[MAX_COLS];
    int (*matrix2)[MAX_COLS];
    int **result;
    int rows1, cols1, rows2, cols2;
    int start, end;
} ThreadData;

int getMaxDigits(int matrix[MAX_ROWS][MAX_COLS], int rows, int cols) {
    int max = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int digits = (matrix[i][j] == 0) ? 1 : (int)log10(abs(matrix[i][j])) + 1 + (matrix[i][j] < 0);
            if (digits > max) {
                max = digits;
            }
        }
    }
    return max;
}

int getMaxDigitsTensor(int **matrix, int rows, int cols) {
    int max = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int digits = (matrix[i][j] == 0) ? 1 : (int)log10(abs(matrix[i][j])) + 1 + (matrix[i][j] < 0);
            if (digits > max) {
                max = digits;
            }
        }
    }
    return max;
}

void readMatrixFromFile(char *filename, int matrix[MAX_ROWS][MAX_COLS], int *rows, int *cols) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    int tempCols = 0, tempRows = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (tempRows == 0) {
            
            char *token = strtok(line, " \t\n");
            while (token != NULL) {
                tempCols++;
                token = strtok(NULL, " \t\n");
            }
        }
        tempRows++;
    }

    
    if (tempRows > MAX_ROWS || tempCols > MAX_COLS) {
        fclose(file);
        fprintf(stderr, "Matrix dimensions exceed the maximum allowed size.\n");
        exit(EXIT_FAILURE);
    }

    *rows = tempRows;
    *cols = tempCols;

    
    rewind(file);

    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            if (fscanf(file, "%d", &matrix[i][j]) != 1) {
                fclose(file);
                perror("Failed to read matrix data");
                exit(EXIT_FAILURE);
            }
        }
    }

    fclose(file);
}

void printMatrix(int matrix[MAX_ROWS][MAX_COLS], int rows, int cols) {
    int width = getMaxDigits(matrix, rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%*d ", width, matrix[i][j]);
        }
        printf("\n");
    }
}

void *tensorProductThread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int index = data->start; index < data->end; index++) {
        int i = index / data->cols1;
        int j = index % data->cols1;
        for (int k = 0; k < data->rows2; k++) {
            for (int l = 0; l < data->cols2; l++) {
                data->result[i * data->rows2 + k][j * data->cols2 + l] = data->matrix1[i][j] * data->matrix2[k][l];
            }
        }
    }
    return NULL;
}

void tensorProduct(int matrix1[MAX_ROWS][MAX_COLS], int matrix2[MAX_ROWS][MAX_COLS], int rows1, int cols1, int rows2, int cols2, int **result) {
    int numElements = rows1 * cols1;
    int numThreads = 3; 
    pthread_t threads[numThreads];
    ThreadData data[numThreads];
    int elementsPerThread = numElements / numThreads;
    
    for (int i = 0; i < numThreads; i++) {
        data[i].matrix1 = matrix1;
        data[i].matrix2 = matrix2;
        data[i].result = result;
        data[i].rows1 = rows1;
        data[i].cols1 = cols1;
        data[i].rows2 = rows2;
        data[i].cols2 = cols2;
        data[i].start = i * elementsPerThread;
        data[i].end = (i == numThreads - 1) ? numElements : (i + 1) * elementsPerThread;
        pthread_create(&threads[i], NULL, tensorProductThread, &data[i]);
    }
    
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
}

void printTensorMatrixToFile(int **matrix, int rows, int cols, char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    
    int *maxWidth = (int *)malloc(cols * sizeof(int));
    for (int j = 0; j < cols; j++) {
        maxWidth[j] = 0;
        for (int i = 0; i < rows; i++) {
            int len = snprintf(NULL, 0, "%d", matrix[i][j]);
            if (len > maxWidth[j]) {
                maxWidth[j] = len;
            }
        }
    }
    
    for (int i = 0; i < rows; i++) {
        if (i > 0) {
            fprintf(file, "\n");
        }
        for (int j = 0; j < cols; j++) {
            if (j == cols - 1) {
            
                fprintf(file, "%*d", maxWidth[j], matrix[i][j]);
            } else {
               
                fprintf(file, "%*d ", maxWidth[j], matrix[i][j]);
            }
        }
    }
    
    fclose(file);
    free(maxWidth);
}



int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <matrix1_file> <matrix2_file>\n", argv[0]);
        return 1;
    }

    int matrix1[MAX_ROWS][MAX_COLS], matrix2[MAX_ROWS][MAX_COLS];
    int rows1, cols1, rows2, cols2;

    int **matrix3 = (int **)malloc(MAX_TENSOR_ROWS * sizeof(int *));
    for (int i = 0; i < MAX_TENSOR_ROWS; i++) {
        matrix3[i] = (int *)malloc(MAX_TENSOR_COLS * sizeof(int));
    }

    readMatrixFromFile(argv[1], matrix1, &rows1, &cols1);
    readMatrixFromFile(argv[2], matrix2, &rows2, &cols2);
    tensorProduct(matrix1, matrix2, rows1, cols1, rows2, cols2, matrix3);

    printTensorMatrixToFile(matrix3, rows1 * rows2, cols1 * cols2, "tensor.out");


    for (int i = 0; i < MAX_TENSOR_ROWS; i++) {
        free(matrix3[i]);
    }
    free(matrix3);


    return 0;
}
