mod ipv4;
mod ipv6;

use ipv4::Ipv4Subnet;
use ipv6::Ipv6Subnet;
use std::env;
use std::process;

fn print_usage(prog_name: &str) {
    println!(
        "Usage: {} <CIDR_STRING>\n\n\
        Zero-dependency IPv4/IPv6 CIDR Subnet Calculator.\n\n\
        Examples:\n  \
          {} 192.168.1.50/24\n  \
          {} 10.0.0.0/16\n  \
          {} 2001:db8::1/64",
        prog_name, prog_name, prog_name, prog_name
    );
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 || args.contains(&"--help".to_string()) || args.contains(&"-h".to_string()) {
        print_usage(&args[0]);
        process::exit(if args.len() < 2 { 1 } else { 0 });
    }

    let cidr_arg = &args[1];

    if cidr_arg.contains(':') {
        // Evaluate as IPv6
        match Ipv6Subnet::parse(cidr_arg) {
            Ok(subnet) => subnet.print_report(),
            Err(err) => {
                eprintln!("\x1b[1;31mError:\x1b[0m {}", err);
                process::exit(1);
            }
        }
    } else {
        // Evaluate as IPv4
        match Ipv4Subnet::parse(cidr_arg) {
            Ok(subnet) => subnet.print_report(),
            Err(err) => {
                eprintln!("\x1b[1;31mError:\x1b[0m {}", err);
                process::exit(1);
            }
        }
    }
}
