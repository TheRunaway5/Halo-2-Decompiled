//! Bundled process-main SDK host, also used by owned component qualification.
#![cfg_attr(windows, windows_subsystem = "windows")]

fn main() {
    let mut args = std::env::args_os();
    let _executable = args.next();
    let worker = args.next().is_some_and(|arg| arg == "--eos-sdk-worker") && args.next().is_none();
    let code = if worker {
        halo2_online_protocol_bridge::eos_sdk_worker_main()
    } else {
        2
    };
    std::process::exit(code);
}
