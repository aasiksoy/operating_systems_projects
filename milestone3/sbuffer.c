/**
 * \author {AUTHOR}
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "sbuffer.h"

typedef struct sbuffer_node {
    struct sbuffer_node *next;
    sensor_data_t data;
} sbuffer_node_t;

struct sbuffer {
    sbuffer_node_t *head;
    sbuffer_node_t *tail;
    pthread_mutex_t mutex; 
    pthread_cond_t cond;
    int closed; 
};

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;

    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    (*buffer)->closed = 0;

    if (pthread_mutex_init(&((*buffer)->mutex), NULL) != 0) {
        free(*buffer);
        *buffer = NULL;
        return SBUFFER_FAILURE;
    }
    if (pthread_cond_init(&((*buffer)->cond), NULL) != 0) {
        pthread_mutex_destroy(&((*buffer)->mutex));
        free(*buffer);
        *buffer = NULL;
        return SBUFFER_FAILURE;
    }
    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    sbuffer_node_t *dummy;

    if (buffer == NULL || *buffer == NULL) {
        return SBUFFER_FAILURE;
    }

    pthread_mutex_lock(&(*buffer)->mutex);

    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }

    pthread_mutex_unlock(&(*buffer)->mutex);
    pthread_mutex_destroy(&(*buffer)->mutex);
    pthread_cond_destroy(&(*buffer)->cond);

    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;

    if (buffer == NULL || data == NULL) return SBUFFER_FAILURE;

    dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL) return SBUFFER_FAILURE;

    dummy->data = *data;
    dummy->next = NULL;

    pthread_mutex_lock(&buffer->mutex);

    if (buffer->tail == NULL) { 
        buffer->head = buffer->tail = dummy;
    } else {
        buffer->tail->next = dummy;
        buffer->tail = dummy;
    }

    if (data->id == 0) {
        buffer->closed = 1;
    }

    pthread_cond_broadcast(&buffer->cond);
    pthread_mutex_unlock(&buffer->mutex);

    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;

    if (buffer == NULL || data == NULL) return SBUFFER_FAILURE;

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->head == NULL && buffer->closed == 0) {
        pthread_cond_wait(&buffer->cond, &buffer->mutex);
    }
    if (buffer->head == NULL && buffer->closed == 1) {
        pthread_mutex_unlock(&buffer->mutex);
        return SBUFFER_NO_DATA;
    }

    dummy = buffer->head;
    buffer->head = dummy->next;
    if (buffer->head == NULL) {
        buffer->tail = NULL;
    }

    if (dummy->data.id == 0) {
        free(dummy);
        pthread_mutex_unlock(&buffer->mutex);
        return SBUFFER_NO_DATA;
    }

    *data = dummy->data;
    free(dummy);

    pthread_mutex_unlock(&buffer->mutex);
    return SBUFFER_SUCCESS;
}

