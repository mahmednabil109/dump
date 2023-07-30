#include <stdio.h>
#include <stdlib.h>

typedef struct entry {
    __uint8_t free;
    __int32_t next;
    __int32_t data;
} _entry;

typedef struct {
    __int32_t head;
    __int32_t size;
    _entry *array;
} free_list;

free_list* free_list_init(__uint32_t size) {
    free_list* list = (free_list*) malloc(sizeof(free_list));
    list->head = 0;
    list->size = size;
    list->array = (_entry *)malloc(size * sizeof(_entry));
    for(int i=0; i<size; i++){
        _entry t = {1, i == size - 1 ? -1 : i + 1, 0};
        list->array[i] = t;
    }
    return list;
}

__int32_t free_list_add(free_list* list, __int32_t entry) {
    // TODO(mahmednabil): don't fail silently
    if(list->head == -1) return -1;
    __int32_t pos = list->head;
    _entry new = {0, -1, entry};

    list->head = list->array[pos].next;
    list->array[pos] = new;

    return pos;
}

void free_list_delete(free_list* list, __int32_t idx) {
    if (idx >= list->size) return;
    if (list->array[idx].free == 1)
        return;
    list->array[idx].free = 0;
    list->array[idx].next = list->head;
    list->head = idx;
}

void free_list_print(free_list* list){
    if(!list) return;

    printf("[ ");
    for(int i=0; i<list->size; i++){
        _entry e = list->array[i];
        if (e.free == 0)
            printf("%d ", e.data);
    }
    printf("]\n");
}

int main() {
    free_list* list = free_list_init(10);
    
    free_list_print(list);
    free_list_add(list, 1);
    free_list_add(list, 4);
    free_list_add(list, 2);
    free_list_add(list, 3);
    free_list_print(list);
    free_list_delete(list, 0);
    free_list_delete(list, 3);
    free_list_add(list, 32);
    free_list_add(list, 3323);
    free_list_add(list, 322);
    free_list_add(list, 231);
    free_list_add(list, 23);
    free_list_add(list, 10);
    free_list_print(list);
    
    return 0;
}