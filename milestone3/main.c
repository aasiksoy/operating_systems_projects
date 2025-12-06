#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "sbuffer.h"
#include "config.h"

#define N_READERS 2

typedef struct {
    sbuffer_t *buffer;
    FILE *fp;
} writer_args_t;

typedef struct {
    sbuffer_t *buffer;
    FILE *fp;
} reader_args_t;

void* writer_thread(void *arg) {
    writer_args_t *args = arg;
    sensor_data_t d;

    while (fread(&d, sizeof(sensor_data_t), 1, args->fp) == 1) {
        sbuffer_insert(args->buffer, &d);
        usleep(10000); // 10 ms
    }

    for (int i = 0; i < N_READERS; i++) {
        sensor_data_t eos = {0, 0.0, 0};
        sbuffer_insert(args->buffer, &eos);
    }

    return NULL;
}

void* reader_thread(void *arg) {
    reader_args_t *args = arg;
    sensor_data_t d;

    while (1) {
        int r = sbuffer_remove(args->buffer, &d);

        if (r == SBUFFER_NO_DATA) break;

        fprintf(args->fp, "%u,%.2f,%ld\n", d.id, d.value, d.ts);
        fflush(args->fp);
        usleep(25000);
    }
    return NULL;
}

int main() {
    sbuffer_t *buffer;
    sbuffer_init(&buffer);

    FILE *input = fopen("sensor_data", "rb");
    if (!input) {
        perror("Cannot open sensor_data binary file");
        exit(EXIT_FAILURE);
    }

    FILE *output = fopen("sensor_data_out.csv", "w");
    if (!output) {
        perror("Cannot open output CSV");
        exit(EXIT_FAILURE);
    }

    pthread_t writer;
    pthread_t readers[N_READERS];

    writer_args_t wargs = { buffer, input };
    reader_args_t rargs = { buffer, output };

    pthread_create(&writer, NULL, writer_thread, &wargs);

    for (int i = 0; i < N_READERS; i++)
        pthread_create(&readers[i], NULL, reader_thread, &rargs);

    pthread_join(writer, NULL);
    for (int i = 0; i < N_READERS; i++)
        pthread_join(readers[i], NULL);

    fclose(input);
    fclose(output);
    sbuffer_free(&buffer);
    return 0;
}

