#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Flag {
    const char* name;
    char* short_hand;
    void (*callback)(char*);
    struct Flag* next;
} Flag;

typedef struct FlagContainer {
    int amount;
    struct Flag* root;
} FlagContainer;

char* get_short_hand(const char* name) {
    const char* split = (char*) name;
    char* result = malloc(2);
    if(result == NULL)
        return NULL;

    size_t size = 1;
    result[0] = name[0];

    while((split = strchr(split + 1, '-'))) {
        char* temp = realloc(result, size + 2);
        if (temp == NULL) {
            free(result);
            return NULL;
        }

        result = temp;
        result[size] = split[1];
        size++;
    }

    result[size] = '\0';

    return result;
}

const char* trim_hyphen(const char* name) {
    while(*name == '-')
        name++;

    return name;
}

FlagContainer* create_container() {
    FlagContainer* container = malloc(sizeof(FlagContainer));

    container->root = NULL;
    container->amount = 0;

    return container;
}

void free_container(FlagContainer* container) {
    Flag* flag = container->root;

    while(flag) {
        Flag* tmp_next = flag->next;

        free(flag->short_hand);
        free((char*)flag->name);
        free(flag);

        flag = tmp_next;
    }

    free(container);
}

Flag* add_flag(FlagContainer* container, const char* name, void (*callback)(char*), const char* short_hand) {
    Flag* flag = malloc(sizeof(Flag));

    if(flag == NULL) {
        fprintf(stderr, "Failed to allocate flag: %s\n", name);
        return NULL;
    }

    flag->name = strdup(name);
    if (flag->name == NULL) {
        fprintf(stderr, "Failed to duplicate flag name: %s\n", name);
        free(flag);
        return NULL;
    }

    flag->callback = callback;
    flag->next = NULL;

    if(short_hand == NULL)
        flag->short_hand = get_short_hand(name);
    else {
        char* tmp_short_hand = strdup(short_hand);
        if(tmp_short_hand == NULL) {
            fprintf(stderr, "Failed to allocate short hand for flag: %s\n", name);
            free((char*)flag->name);
            free(flag);
            return NULL;
        }
        flag->short_hand = tmp_short_hand;
    }

    container->amount++;

    if(container->root == NULL) {
        container->root = flag;
        return flag;
    }

    Flag* last_flag = container->root;
    while(last_flag->next != NULL)
        last_flag = last_flag->next;

    last_flag->next = flag;

    return flag;
}

Flag* find_flag(Flag* root ,const char* name) {
    Flag* current = root;
    
    while(current) {
        if(strcmp(name, current->name) == 0
        || strcmp(name, current->short_hand) == 0)
            return current;

        current = current->next;
    }

    return NULL;
}

void handle_args(FlagContainer* container, int argc, char** argv, int required_params) {
    if(required_params < 1)
        required_params = 0;

    int i = required_params + 1;

    if(i >= argc &&
       *argv[1] == '-')
        i = 1;

    for(; i < argc; i++) {
        if(*argv[i] != '-')
            continue;

        const char* name = trim_hyphen(argv[i]);
        Flag* flag = find_flag(container->root, name);
        if(flag == NULL) {
            fprintf(stderr, "Flag \"%s\" doesn't exist.\n", name);
            continue;
        }

        char* value = NULL;
        if(i + 1 < argc && argv[i + 1][0] != '-') {
            i++;
            value = argv[i];
        }

        flag->callback(value);
    }

    free_container(container);
}
