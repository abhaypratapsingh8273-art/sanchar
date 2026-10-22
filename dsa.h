#ifndef SANCHAR_DSA_H
#define SANCHAR_DSA_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define TEXT_SIZE 1024
#define QUEUE_SIZE 32
#define DEVICE_LIMIT 64
typedef struct {
    char text[TEXT_SIZE];
    int64_t timestamp;
    int outgoing;
} ChatRecord;
/* Fixed-size circular queue: O(1) push and pop. */
typedef struct {
    ChatRecord items[QUEUE_SIZE];
    size_t front, count;
} MessageQueue;
void queue_init(MessageQueue *q);
int queue_push(MessageQueue *q, const ChatRecord *message);
int queue_pop(MessageQueue *q, ChatRecord *message);
/* Append-only singly linked list: O(1) append using a tail pointer. */
typedef struct HistoryNode {
    ChatRecord record;
    struct HistoryNode *next;
} HistoryNode;
typedef struct { HistoryNode *head, *tail; size_t count; } History;
void history_init(History *h);
int history_append(History *h, const ChatRecord *record);
void history_clear(History *h);
typedef struct { uint64_t address; char name[256]; int paired; } DeviceRecord;
void devices_sort(DeviceRecord *devices, size_t count);
int devices_find(const DeviceRecord *devices, size_t count, uint64_t address);
#ifdef __cplusplus
}
#endif
#endif
