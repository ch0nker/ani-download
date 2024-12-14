#ifndef ARGS_H
#define ARGS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Flag {
    const char* name;
    void (*callback)(char*);
    struct Flag* next;
} Flag;

typedef struct FlagContainer {
    int amount;
    struct Flag* root;
} FlagContainer;

FlagContainer* create_container();
Flag* add_flag(FlagContainer* container, const char* name, void (*callback)(char*), const char* short_hand);
void handle_args(FlagContainer* container, int argc, char** argv, int required_params);

#ifdef __cplusplus
}
#endif
#endif

