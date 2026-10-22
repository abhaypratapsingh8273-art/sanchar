#include "dsa.h"
#include <stdlib.h>
#include <string.h>
void queue_init(MessageQueue *q) { q->front = q->count = 0; }
int queue_push(MessageQueue *q, const ChatRecord *message) {
    if (q->count == QUEUE_SIZE) return 0;
    q->items[(q->front + q->count) % QUEUE_SIZE] = *message;
    ++q->count;
    return 1;
}
int queue_pop(MessageQueue *q, ChatRecord *message) {
    if (!q->count) return 0;
    *message = q->items[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    --q->count;
    return 1;
}
void history_init(History *h) { h->head = h->tail = NULL; h->count = 0; }
int history_append(History *h, const ChatRecord *record) {
    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (!node) return 0;
    node->record = *record;
    node->next = NULL;
    if (h->tail) h->tail->next = node; else h->head = node;
    h->tail = node;
    ++h->count;
    return 1;
}
void history_clear(History *h) {
    HistoryNode *node = h->head;
    while (node) { HistoryNode *next = node->next; free(node); node = next; }
    history_init(h);
}
void devices_sort(DeviceRecord *devices, size_t count) {
    /* Insertion sort is easy to explain and sufficient for 64 devices. */
    for (size_t i = 1; i < count; ++i) {
        DeviceRecord key = devices[i];
        size_t j = i;
        while (j && strcmp(devices[j-1].name, key.name) > 0) {
            devices[j] = devices[j-1]; --j;
        }
        devices[j] = key;
    }
}
int devices_find(const DeviceRecord *devices, size_t count, uint64_t address) {
    for (size_t i = 0; i < count; ++i)
        if (devices[i].address == address) return (int)i;
    return -1;
}
