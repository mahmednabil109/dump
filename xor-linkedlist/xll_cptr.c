#include <stdio.h>
#include <stdlib.h>

typedef struct Xll {
    int data;
    // dealing with ptr as 64 bit integer
    __uint64_t next;
} Xll;

Xll* xll_new(int data){
    Xll* list = (Xll*) malloc(sizeof(Xll));
    list->data = data;
    list->next = 0;
    return list;
}

void xll_insert(Xll** head, int data){
    Xll* new_node = xll_new(data);
    new_node->next = (__uint64_t) *head;
    if (*head != NULL){
        (*head)->next ^= (__uint64_t) (new_node);
    }
    *head = new_node;
}

void xll_delete(Xll **head, int data){
    if((*head)->data == data){
        __uint64_t prev_head = (__uint64_t) (*head);
        (*head) = (Xll*) (*head)->next;
        if((*head) != NULL)
            (*head)->next ^= prev_head;
        free((Xll* ) prev_head);
        return;
    }

    Xll* curr = *head, *tmp = NULL;
    __uint64_t prev = 0;
    while(curr != NULL){
        if(curr->data == data){
            Xll* prev_node = (Xll*) prev;
            __uint64_t prev_curr = (__uint64_t) curr;
            __uint64_t next = curr->next ^ prev;
            prev_node->next ^= prev_curr ^ next;
            if(next != 0){
                Xll* next_node = (Xll*) next;
                next_node->next ^= prev_curr ^ prev;
            }
            return;
        }
        tmp = curr;
        curr = (Xll*) (prev ^ curr->next);
        prev = (__uint64_t) tmp;
    }

}

void xll_print(Xll* head){
    Xll *curr = head;
    Xll* tmp;
    __uint64_t prev = 0;

    while(curr != NULL){
        printf("%d ", curr->data);
        tmp = curr;
        curr = (Xll*) (prev ^ curr->next);
        prev = (__uint64_t) tmp;
    }
    printf("\n");
}

int main (){
    Xll* head = NULL;
    xll_insert(&head, 15);
    xll_insert(&head, 10);
    xll_insert(&head, 200);
    xll_insert(&head, 1000);
    xll_print(head);

    xll_delete(&head, 200);
    xll_print(head);

    xll_delete(&head, 1000);
    xll_print(head);

    xll_delete(&head, 15);
    xll_print(head);

    xll_insert(&head, 30);
    xll_print(head);

    return 0;
}