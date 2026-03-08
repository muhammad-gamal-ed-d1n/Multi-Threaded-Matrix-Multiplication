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
void write_matrix(FILE *file, int **mat, int rows, int cols, char* name, char *postfix);
void free_matrix(int **mat, int rows);

typedef struct args {
    int** a;
    int** b;
    int** result;
    int row;
    int col;
    int cols;
    int rows;
} args;

void main(int argc, char *argv[]) {
    FILE *file;
    char mat_a[MAX_LINE];
    char mat_b[MAX_LINE];
    char output[MAX_LINE];


    if (!argv[1] || !argv[2] || !argv[3]) {
        snprintf(mat_a, sizeof(char) * MAX_LINE, "a.txt");    
        snprintf(mat_b, sizeof(char) * MAX_LINE, "b.txt");    
        snprintf(output, sizeof(char) * MAX_LINE, "c");    
    }
    else {
        snprintf(mat_a, sizeof(char) * MAX_LINE, "%s.txt", argv[1]);    
        snprintf(mat_b, sizeof(char) * MAX_LINE, "%s.txt", argv[2]);    
        snprintf(output, sizeof(char) * MAX_LINE, "%s", argv[3]);    
    }

    file = fopen(mat_a, "r");
    if (!file) {
        printf("Error loading file '%s'\n", mat_a);
        return;
    }

    int *dim = get_dim(file);
    
    if (!dim) {
        printf("Error parsing first matrix' dimensions");
        return;
    }

    int **a = get_matrix(file, dim[0], dim[1]);
    fclose(file);
    //should free dim if mat returns null
    if (a == NULL) {
        printf("Error loading first input matrix\n");
        free_matrix(a, dim[0]);
        free(dim);
    }
    

    file = fopen(mat_b, "r");
    if (!file) {
        printf("Error loading file '%s'\n", mat_b);
        free_matrix(a, dim[0]);
        free(dim);
        return;
    }

    int * dim2 = get_dim(file);

    if (!dim2) {
        printf("Error parsing second matrix' dimensions");
        return;
    }
    
    //-------------------------------
    int **b = get_matrix(file, dim2[0], dim2[1]);
    fclose(file);
    if (b == NULL) {
        perror("Error loading second input file");
        free_matrix(a, dim[0]);
        free_matrix(b, dim2[0]);
        free(dim);
        free(dim2);
    }

    int **result = malloc(sizeof(int*)*dim[0]);
    
    for (int i = 0; i < dim[0]; i++) {
        result[i] = malloc(sizeof(int) * dim2[1]);
    }

    //THREAD PER ROW
    pthread_t *threads = malloc(sizeof(pthread_t)*dim[0]);
    for (int i = 0; i < dim[0]; i++) {
        struct args *data = malloc(sizeof(struct args));
        data->a = a;
        data->b = b;
        data->cols = dim2[1];
        data->rows = dim2[0];
        data->row = i;
        data->result = result;
        pthread_create(&threads[i], 0, multiply_row, (void *)data);
    }
    
    for (int i = 0; i < dim[0]; i++) {
        pthread_join(threads[i], 0);
    }
    free(threads);
    write_matrix(file, result, dim[0], dim2[1], output, "_per_row");
    

    //THREAD PER ELEMENT
    threads = malloc(sizeof(pthread_t) * dim[0] * dim2[1]);
    int t = 0;
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim2[1]; j++) {
            struct args *data = malloc(sizeof(struct args));
            data->a = a;
            data->b = b;
            data->row = i;
            data->col = j;
            data->cols = dim[1];
            data->result = result;
            
            pthread_create(&threads[t], 0, multiply_element, (void *)data);
            t++;
        }
    }

    for (int i = 0; i < dim[0] * dim2[1]; i++) {
        pthread_join(threads[i], 0);
    }
    free(threads);
    write_matrix(file, result, dim[0], dim2[1], output, "_per_element");

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

    free_matrix(result, dim[0]);
    free_matrix(a, dim[0]);
    free_matrix(b, dim2[0]);
    free(dim);
    free(dim2);
}

void free_matrix(int **mat, int rows) {
    for (int r = 0; r < rows; r++) {
        free(mat[r]);
    }
    free(mat);
}

void write_matrix(FILE *file, int **mat, int rows, int cols, char* name, char *postfix) {
    char buffer[MAX_LINE];
    snprintf(buffer, sizeof(buffer), "%s%s.txt", name, postfix);

    file = fopen(buffer, "w");
    if (!file) {
        perror("Error writing to file");
        return;
    }
    fprintf(file, "rows=%d col=%d\n", rows, cols);
    for (int r = 0; r < rows; r++) {
        // printf("infinite\n");
        for (int c = 0; c < cols; c++) {
            fprintf(file, "%d", mat[r][c]);
            if (c + 1 < cols) fprintf(file, " ");
        }
        if (r + 1 < rows) fprintf(file, "\n");
    }
    fclose(file);
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
    return NULL;
}

void *multiply_row(void *args) {
    struct args *data = (struct args *)(args);
    int **a = data->a;
    int **b = data->b;
    int **result = data->result;
    int row = data->row;
    int cols = data->cols;
    int rows = data->rows;

    for (int c = 0; c < cols; c++) {
        int sum = 0;
        for (int i = 0; i < rows; i++) {
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
                return NULL;
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
                    //Done
                    free_matrix(mat, row);
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
            //Done
            free_matrix(mat, row);
            return NULL;
        }
    }

    return mat;
}