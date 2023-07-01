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

fn main() {
    save_status();

    get_shell();
}
