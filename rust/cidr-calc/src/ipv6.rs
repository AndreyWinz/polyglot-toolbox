use std::net::Ipv6Addr;

pub struct Ipv6Subnet {
    pub input_ip: Ipv6Addr,
    pub prefix: u32,
    pub network: u128,
    pub max_addr: u128,
}

impl Ipv6Subnet {
    pub fn parse(cidr: &str) -> Result<Self, String> {
        let parts: Vec<&str> = cidr.split('/').collect();
        if parts.len() != 2 {
            return Err("Invalid CIDR format. Expected IP/prefix (e.g. 2001:db8::1/64)".into());
        }

        let input_ip: Ipv6Addr = parts[0]
            .parse()
            .map_err(|_| format!("Invalid IPv6 address: '{}'", parts[0]))?;

        let prefix: u32 = parts[1]
            .parse()
            .map_err(|_| format!("Invalid prefix length: '{}'", parts[1]))?;

        if prefix > 128 {
            return Err("IPv6 prefix length must be between 0 and 128".into());
        }

        let ip_u128 = u128::from(input_ip);
        let mask = if prefix == 0 {
            0
        } else {
            !0u128 << (128 - prefix)
        };

        let network = ip_u128 & mask;
        let max_addr = network | !mask;

        Ok(Self {
            input_ip,
            prefix,
            network,
            max_addr,
        })
    }

    pub fn print_report(&self) {
        let net_addr = Ipv6Addr::from(self.network);
        let max_addr = Ipv6Addr::from(self.max_addr);
        let host_bits = 128 - self.prefix;

        println!("\x1b[1;35m🌐 IPv6 Subnet Summary\x1b[0m");
        println!("============================================================");
        println!("{:<20} : {}/{}", "CIDR Input", self.input_ip, self.prefix);
        println!("{:<20} : {}", "Network Prefix", net_addr);
        println!("{:<20} : {}", "First Address", net_addr);
        println!("{:<20} : {}", "Last Address", max_addr);
        println!("{:<20} : /{}", "Prefix Length", self.prefix);
        println!("{:<20} : {}", "Host Bits", host_bits);
        println!(
            "{:<20} : 2^{} ({})",
            "Total Addresses",
            host_bits,
            format_ipv6_host_count(host_bits)
        );
        println!("{:<20} : {}", "Scope", get_ipv6_scope(self.input_ip));
        println!("============================================================");
    }
}

fn format_ipv6_host_count(host_bits: u32) -> String {
    if host_bits >= 128 {
        "3.4 x 10^38 (Entire IPv6 Space)".to_string()
    } else if host_bits >= 64 {
        format!("1.84 x 10^19 (~{:.2} Quintillion)", (1u128 << (host_bits - 64)) as f64)
    } else {
        format!("{}", 1u128 << host_bits)
    }
}

fn get_ipv6_scope(ip: Ipv6Addr) -> &'static str {
    if ip.is_loopback() {
        "Loopback (::1)"
    } else if (u128::from(ip) >> 120) == 0xfe {
        "Link-Local Unicast (fe80::/10)"
    } else if (u128::from(ip) >> 121) == 0xfc >> 1 {
        "Unique Local Unicast (fc00::/7)"
    } else if ip.is_multicast() {
        "Multicast (ff00::/8)"
    } else {
        "Global Unicast"
    }
}
