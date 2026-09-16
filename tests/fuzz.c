#include "rbtree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>

/* tracks which keys are currently present in the tree, so a delete
 * target can be picked from keys guaranteed to exist */
typedef struct {
    char **keys;
    size_t size;
    size_t cap;
} key_list_t;

static key_list_t key_list_create(void)
{
    key_list_t list;
    list.size = 0;
    list.cap = 10;
    list.keys = malloc(list.cap * sizeof(char *));
    if (list.keys == NULL)
    {
        fprintf(stderr, "Error: out of memory allocating key list\n");
        exit(1);
    }
    return list;
}

static void key_list_add(key_list_t *list, char *key)
{
    for (size_t i = 0; i < list->size; i++)
    {
        if (strcmp(key, list->keys[i]) == 0)
        {
            goto duplicate_key;
        }
    }

    list->size++;
    if (list->size > list->cap)
    {
        list->cap *= 2;
        char **tmp = realloc(list->keys, list->cap * sizeof(char *));
        if (tmp == NULL)
        {
            fprintf(stderr, "Error: out of memory growing key list\n");
            exit(1);
        }
        list->keys = tmp;
    }
    list->keys[list->size-1] = key;
    return;

duplicate_key:
    free(key);
    return;
}

/* removes the key at index by moving the last key into its place;
 * order is never preserved, since this is just a bag of present keys */
static void key_list_remove(key_list_t *list, size_t index)
{
    size_t last = list->size - 1;
    free(list->keys[index]);
    if (index != last)
    {
        list->keys[index] = list->keys[last];
    }
    list->keys[last] = NULL;
    list->size--;
}

static void key_list_destroy(key_list_t *list)
{
    for (size_t i = 0; i < list->size; i++)
    {
        free(list->keys[i]);
    }
    free(list->keys);
}

/* placeholder: real fuzzing logic belongs to a later milestone (M2) */
int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Error: Supply at least one argument for # of tests");
        exit(1);
    }
    
    char *endptr; //For str to long function
    errno = 0; //Reset to 0 because C does not accidentally reset the value of errno 
    long count = strtol(argv[1], &endptr, 10); //How many times the fuzzer runs
    if(endptr == argv[1] || *endptr != '\0')
    {
        fprintf(stderr, "Error: Please supply a string made up only of #'s for the first argument");
        exit(1);
    }
    if(errno == ERANGE || count < 0)
    {
        fprintf(stderr, "Error: Supplied # is too small/large to fit in a long (or is negative)");
        exit(1);
    }

    endptr = NULL;
    errno = 0;
    long seed = (argc==3) ? strtol(argv[2], &endptr, 10) : time(NULL);
    srand(seed);

    key_list_t list = key_list_create();
    rbtree_t *t = rb_create(NULL);
    int operation = 0;
    for(int i=0; i<count; i++)
    {
        int choice = 1 + (rand() % 2);

        if(choice == 1)
        {
            operation++;

            size_t len = 1 + rand() % 100;
            char *key = malloc(len+1);
            if(key==NULL)
            {
                fprintf(stderr, "Error: Ran out of memory generating key");
                exit(1);
            }        
            for(size_t i=0; i<len; i++) {key[i] = (char)(1 + rand() % 255);}
            key[len] = '\0';

            fprintf(stdout, "Operation %d: Insert key ", operation);
            for (size_t j = 0; j < len; j++) fprintf(stdout, "%02x", (unsigned char)key[j]);
            fprintf(stdout, "\n");

            

            if(rb_insert(t, key, NULL) == 0 && rb_validate(t) == 0) 
            {
                key_list_add(&list, key);
                continue;
            }
            else
            {
                fprintf(stdout, "Insert failed on operation: %d\n", operation);
                fprintf(stdout, "Current seed: %ld", seed);
                exit(1);
            }
        }
        else
        {
            operation++;

            if(list.size == 0) {continue;}

            size_t index = rand() % list.size;
            char *key = list.keys[index];
            size_t len = strlen(key);

            fprintf(stdout, "Operation %d: Delete key ", operation);
            for (size_t j = 0; j < len; j++) fprintf(stdout, "%02x", (unsigned char)key[j]);
            fprintf(stdout, "\n");

            if(rb_delete(t, key) == 0 && rb_validate(t) == 0)
            {
                key_list_remove(&list, index);
                continue;
            }
            else
            {
                fprintf(stdout, "Delete failed on operation: %d\n", operation);
                fprintf(stdout, "Current seed: %ld", seed);
                exit(1);
            }
        }
    }
    fprintf(stdout, "Fuzzer runs green!\n");
    key_list_destroy(&list);
    rb_destroy(t);
}
