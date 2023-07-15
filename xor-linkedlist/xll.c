#include <stdio.h>
#include <stdlib.h>

typedef struct Xll {
    int data;
    struct Xll* next;
} Xll;

Xll* xll_new(int data){
    Xll* list = (Xll*) malloc(sizeof(Xll));
    list->data = data;
    list->next = NULL;
    return list;
}

Xll* xll_xor(Xll* a, Xll* b){
    return (Xll*) (((__uint64_t) a) ^ ((__uint64_t) b));
}

void xll_insert(Xll** head, int data){
    Xll* new_node = xll_new(data);
    new_node->next = *head;
    if (*head != NULL){
        (*head)->next = xll_xor(new_node, (*head)->next);
    }
    *head = new_node;
}

void xll_print(Xll* head){
    Xll *curr = head, *prev = NULL;
    Xll* tmp;

    while(curr != NULL){
        printf("%d ", curr->data);
        tmp = curr;
        curr = xll_xor(prev, curr->next);
        prev = tmp;
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
    return 0;
}