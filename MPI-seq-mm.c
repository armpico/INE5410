//#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>

#ifndef MRAND
#define MRAND 3
#endif //MRAND

#ifndef DEBUG
#define DEBUG 0
#endif

typedef struct {
  int height;
  int length;
  int* data;
} matrix_t;

void set_element (matrix_t* mat, int x, int y, int val) {
  mat->data[x*(mat->length)+y] = val;
}

int get_element (matrix_t* mat, int x, int y) {
  return mat->data[x*(mat->length)+y];
}

void generate_matrix (int rseed, matrix_t* mat) {
  srand(rseed);
  for (int i = 0; i < mat->height; ++i) {
    for (int j = 0; j < mat->length; ++j) {
      set_element(mat, i, j, (int)(rand()%MRAND));
    }
  }
}

void mat_mult (matrix_t* a, matrix_t* b, matrix_t* c) {
  int max_i = a->length;
  int max_j = b->height;

  if (max_i != c->height || max_j != c->length) {
    // Wrong size of C
    return;
  }

  for (int i = 0; i < max_i; ++i) {
    for (int j = 0; j < max_j; ++j) {
      int calc = 0;
      for (int k = 0; k < b->length; ++k) {
        calc += get_element(a, i, k) * get_element(b, k, j);
      }
      set_element(c, i, j, calc);
    }
  }
}

unsigned int modular_sum(matrix_t* a, int modulus) {
  unsigned int retval = 0;
  for (int i = 0; i < a->height; ++i) {
    for (int j = 0; j < a->length; ++j) {
      retval += get_element(a, i, j);
      retval %= modulus;
    }
  }
  return retval;
}

void print_mat(matrix_t* mat) {
  for (int i = 0; i < mat->height; ++i) {
    printf("[ ");
    for (int j = 0; j < mat->length; ++j) {
      printf("%d ", get_element(mat, i, j));
    }
    printf("]\n");
  }
}

int main (int argc, char** argv) {
  int size,rank, signal_c[1];
  int sqr_mat_size;
  int n_matrices;
  int rseed = 3;

  MPI_Status st;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // not enough params
  if (argc < 3) {
    if(rank == 0) printf("Not enough params\n");
    return 0;
  } else {
      // Squared matrices
      if (argc < 5) {
        sqr_mat_size = atoi(argv[1]);
        n_matrices = atoi(argv[2]);
        if (argc == 4) rseed = atoi(argv[3]);
      } else {
        if(rank == 0) printf("Too many params\n");
        return 1;
      }
  }
  // Incorrect params, non compatible

  if(rank == 0) { //mestre
    matrix_t * a = (matrix_t*) malloc(sizeof(matrix_t));
    matrix_t * b = (matrix_t*) malloc(sizeof(matrix_t));
    matrix_t * c = (matrix_t*) malloc(sizeof(matrix_t));
    a->height = sqr_mat_size;
    b->height = sqr_mat_size;
    c->height = sqr_mat_size;
    a->length = sqr_mat_size;
    b->length = sqr_mat_size;
    c->length = sqr_mat_size;
    a->data = (int*) malloc(sizeof(int)*sqr_mat_size*sqr_mat_size);
    b->data = (int*) malloc(sizeof(int)*sqr_mat_size*sqr_mat_size);
    c->data = (int*) malloc(sizeof(int)*sqr_mat_size*sqr_mat_size);

    printf("Generating matrices\n");

    generate_matrix(rseed, a);
    generate_matrix(rseed++, b);

    printf("Multiplying...\n");

    for (int i=1; i< n_matrices; i++) {
      for (int i=1; i< size; i++) {
        generate_matrix(rseed++, b);
        MPI_Send(&signal_c, 1, MPI_INT, i%size, 0, MPI_COMM_WORLD);
        MPI_Send(&a->data, sqr_mat_size*sqr_mat_size, MPI_INT, i%size, 0, MPI_COMM_WORLD);
        MPI_Send(&b->data, sqr_mat_size*sqr_mat_size, MPI_INT, i%size, 0, MPI_COMM_WORLD);
      }

      memcpy(a->data, c->data, sqr_mat_size*sqr_mat_size*sizeof(int));
    }
  }
  else { // escravo
    int continuar = 1;
    while (continuar) {
      MPI_Recv(&signal_c, 1,
        MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
      if(signal_c[0] == 0) return;
      MPI_Recv(&a->data, sqr_mat_size*sqr_mat_siz,
        MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
      MPI_Recv(&b->data, sqr_mat_size*sqr_mat_siz,
        MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
      mat_mult(a, b, c);
      MPI_Send(&c->data, sqr_mat_size*sqr_mat_size,
        MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
  }


  mat_mult(a, b, c);
  memcpy(a->data, c->data, sqr_mat_size*sqr_mat_size*sizeof(int));
  for (int i = 2; i < n_matrices; ++i) {
    printf(".");
    fflush(stdout);
    generate_matrix(rseed++, b);
    mat_mult(a, b, c);
    memcpy(a->data, c->data, sqr_mat_size*sqr_mat_size*sizeof(int));
  }
  printf("\n");

  if(rank == 0) {
    printf("Summing result for checkups...\n");

    unsigned int modsum = modular_sum(c, MRAND*1000);

    printf("The modsum of the result matrix was %d \n", modsum);
  }

  free(a);
  free(b);
  free(c);

  return 10;
}
