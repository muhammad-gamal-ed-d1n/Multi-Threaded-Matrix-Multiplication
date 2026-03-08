#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_LINE 1024

int *get_dim(FILE *file);
int **get_matrix(FILE *file, int row, int col);
void *multiply_row(void *args);
void *multiply_element(void *args);

typedef struct args {
    int** a;
    int** b;
    int** result;
    int row;
    int col;
    int cols;
} args;

void main() {
    FILE *file = fopen("input/a.txt", "r");
    char line[MAX_LINE];

    if (!file) {
        printf("input file doesn't exist\n");
        return;
    }

    int *dim = get_dim(file);

    // printf("row: %d\ncolumn: %d\n", dim[0], dim[1]);

    int **a = get_matrix(file, dim[0], dim[1]);
    //should free dim if mat returns null

    fclose(file);
    file = fopen("input/b.txt", "r");

    int * dim2 = get_dim(file);
    int **b = get_matrix(file, dim2[0], dim2[1]);
    int **result = malloc(sizeof(int*)*dim[0]);
    
    //allocate matrix
    for (int i = 0; i < dim[0]; i++) {
        result[i] = malloc(sizeof(int) * dim2[1]);
    }

    //THREAD PER ROW
    pthread_t *threads = malloc(sizeof(pthread_t)*dim[0]);
    for (int i = 0; i < dim[0]; i++) {
        struct args *data = malloc(sizeof(struct args));
        data->a = a;
        data->b = b;
        data->col = dim[1];
        data->row = i;
        data->result = result;
        pthread_create(&threads[i], 0, multiply_row, (void *)data);
    }

    //THREAD PER ELEMENT
    threads = malloc(sizeof(pthread_t) * dim[0] * dim2[1]);
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim2[1]; j++) {
            struct args *data = malloc(sizeof(struct args));
            data->a = a;
            data->b = b;
            data->row = i;
            data->col = j;
            data->cols = dim[1];
            data->result = result;
            
            pthread_create(&threads[i], 0, multiply_element, (void *)data);
        }
    }

    for (int i = 0; i < dim[0] * dim2[1]; i++) {
        pthread_join(threads[i], 0);
    }

    printf("mat a:\n");
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            printf("%d ", a[i][j]);
        }
        printf("\n");
    }

    printf("mat b:\n");
    for (int i = 0; i < dim2[0]; i++) {
        for (int j = 0; j < dim2[1]; j++) {
            printf("%d ", b[i][j]);
        }
        printf("\n");
    }

    printf("result:\n");
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim2[1]; j++) {
            printf("%d ", result[i][j]);
        }
        printf("\n");
    }

    free(dim);
    free(a);
    free(b);
}

void *multiply_element(void *args) {
    struct args *data = args;
    int **a = data->a;
    int **b = data->b;
    int **result = data->result;
    int row = data->row;
    int col = data->col;
    int cols = data->cols;

    int sum = 0;
    for (int c = 0; c < cols; c++) {
        sum += a[row][c] * b[c][col];
    }
    result[row][col] = sum;

    free(args);
}

void *multiply_row(void *args) {
    struct args *data = (struct args *)(args);
    int **a = data->a;
    int **b = data->b;
    int **result = data->result;
    int row = data->row;
    int col = data->col;

    for (int c = 0; c < col; c++) {
        int sum = 0;
        for (int i = 0; i < col; i++) {
            sum += a[row][i] * b[i][c];
        }
        result[row][c] = sum;
    }
    // 1 2  1 2 3 
    // 3 4  4 5 6 

    free(args);

    return NULL;
}


int *get_dim(FILE *file) {
    char line[MAX_LINE];
    char *start = line;
    char *end = line;
    // Should be freed later
    int *dim = malloc(sizeof(int) * 2);

    if (fgets(start, sizeof(char) * MAX_LINE, file) !=NULL) {
        for (int i = 0; i < 2; i++) {
            int j = 0;
            while(start[j] && !(start[j] >= '0' && start[j] <= '9')) j++;
            start += j;

            int d = strtol(start, &end, 0);
            if (start == end || d == 0) {
                printf("Error: invalid matrix format\n");
                free(dim);
            }

            start = end;
            dim[i] = d;
        }
    }

    return dim;
}

int **get_matrix(FILE *file, int row, int col) {
    int **mat = malloc(sizeof(int*)*row);
    
    //allocate matrix
    for (int i = 0; i < row; i++) {
        mat[i] = malloc(sizeof(int) * col);
    }

    for (int i = 0; i < row; i++) {
        char line[MAX_LINE];
        char *start = line;
        char *end;

        if (fgets(start, sizeof(char) * MAX_LINE, file) != NULL) {
            for (int j = 0; j < col; j++) {
                int elem = strtol(start, &end, 0);

                if (start == end) {
                    //handle error in column
                    printf("Error: missing column/s in matrix input");
                    //SHOULD IMPLEMENT METHOD WHICH FREES MATRIX BUT THIS IS FINE FOR NOW
                    free(mat);
                    return NULL;
                }

                int k = 0; 
                while(start[k] && !(start[j] >= '0' && start[j] <= '9')) k++;
                start += k;

                start = end;
                mat[i][j] = elem;
            }
        }
        else {
            //handle error in row
            printf("Error: missing row/s in matrix");
            //SHOULD IMPLEMENT METHOD WHICH FREES MATRIX BUT THIS IS FINE FOR NOW
            free(mat);
            return NULL;
        }
    }

    return mat;
}