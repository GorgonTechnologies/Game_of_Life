#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>

enum Commands{
    QUIT = 'q',
    HELP = 'h',
    DELETE = 'd',
    INSERT = 'i',
    PRINT = 'p'
};

struct Node {
    int value;
    struct Node *next;
};

struct Node *root;

void print_help(){
    printf("\n");
    printf("------------------\n");
    printf(" .HELP (commands) \n");
    printf("------------------\n");
    printf("-i = insert\n");
    printf("-p = print list\n");
    printf("-q = quit / exit\n");
    printf("-h = print help\n");
    printf("------------------\n");
    printf("\n");
}

void print_list(){
    printf("%d ", root -> value);

    struct Node *next = root -> next;

    while (next)
    {
        printf("%d ", next -> value);
        next = next -> next;
    }
    printf("\n");
}

void print_insert(){
    int value;

    printf("Type value: ");
    scanf("%d", &value);

    if(!root){
        root = malloc(sizeof(struct Node));
        root->value = value;
        root->next = 0;
    } else {
        struct Node *next = root;
        while (next->next) next = next->next;

        next->next = malloc(sizeof(struct Node));
        next->next->value = value;
        next->next->next = 0;
    }
}

void delete_second_element(){
    struct Node *delete = root->next;
    root->next = root->next->next;
    free(delete);
}

void print_command_request(bool *run ){
    char input;

    printf("Type command (h, i, d, p, q): ");
    scanf("%s", &input);
    
    switch(input){
        case INSERT: print_insert(); break;
        case DELETE: delete_second_element(); break;
        case PRINT: print_list(); break;
        case QUIT: *run = false; break;
        case HELP:
        default: print_help(); break;    
    }
}

int main(int argc, char const *argv[])
{
    bool run = true;

    printf("\n");
    while(run) print_command_request(&run);
    printf("\n");
    
    while (root){
        struct Node *delete = root;
        root = root->next;
        free(delete);
    }

    return 0;
}
