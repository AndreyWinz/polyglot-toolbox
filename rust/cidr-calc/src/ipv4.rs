use std::net::Ipv4Addr;

pub struct Ipv4Subnet {
    pub input_ip: Ipv4Addr,
    pub prefix: u32,
    pub mask: u32,
    pub network: u32,
    pub broadcast: u32,
    pub total_hosts: u64,
    pub usable_hosts: u64,
}

impl Ipv4Subnet {
    pub fn parse(cidr: &str) -> Result<Self, String> {
        let parts: Vec<&str> = cidr.split('/').collect();
        if parts.len() != 2 {
            return Err("Invalid CIDR format. Expected IP/prefix (e.g. 192.168.1.1/24)".into());
        }

        let input_ip: Ipv4Addr = parts[0]
            .parse()
            .map_err(|_| format!("Invalid IPv4 address: '{}'", parts[0]))?;

        let prefix: u32 = parts[1]
            .parse()
            .map_err(|_| format!("Invalid prefix length: '{}'", parts[1]))?;

        if prefix > 32 {
            return Err("IPv4 prefix length must be between 0 and 32".into());
        }

        let ip_u32 = u32::from(input_ip);
        let mask = if prefix == 0 {
            0
        } else {
            !0u32 << (32 - prefix)
        };

        let network = ip_u32 & mask;
        let broadcast = network | !mask;

        let total_hosts = if prefix == 32 { 1 } else { 1u64 << (32 - prefix) };
        let usable_hosts = match prefix {
            32 => 1,
            31 => 2, // RFC 3021 point-to-point links
            _ => if total_hosts > 2 { total_hosts - 2 } else { 0 },
        };

        Ok(Self {
            input_ip,
            prefix,
            mask,
            network,
            broadcast,
            total_hosts,
            usable_hosts,
        })
    }

    pub fn print_report(&self) {
        let net_addr = Ipv4Addr::from(self.network);
        let bcast_addr = Ipv4Addr::from(self.broadcast);
        let mask_addr = Ipv4Addr::from(self.mask);
        let wildcard_addr = Ipv4Addr::from(!self.mask);

        let (first_ip, last_ip) = match self.prefix {
            32 => (net_addr, net_addr),
            31 => (net_addr, bcast_addr),
            _ => (
                Ipv4Addr::from(self.network + 1),
                Ipv4Addr::from(self.broadcast - 1),
            ),
        };

        println!("\x1b[1;36m🌐 IPv4 Subnet Summary\x1b[0m");
        println!("============================================================");
        println!("{:<20} : {}/{}", "CIDR Input", self.input_ip, self.prefix);
        println!("{:<20} : {}", "Network Address", net_addr);
        println!("{:<20} : {}", "Broadcast Address", bcast_addr);
        println!("{:<20} : {} - {}", "Usable IP Range", first_ip, last_ip);
        println!("{:<20} : {}", "Subnet Mask", mask_addr);
        println!("{:<20} : {}", "Wildcard Mask", wildcard_addr);
        println!("{:<20} : {}", "Total Addresses", self.total_hosts);
        println!("{:<20} : {}", "Usable Hosts", self.usable_hosts);
        println!("{:<20} : {}", "Subnet Binary", format_binary_u32(self.mask));
        println!("{:<20} : {}", "Scope / Class", get_ipv4_scope(self.input_ip));
        println!("============================================================");
    }
}

fn format_binary_u32(val: u32) -> String {
    let bytes = val.to_be_bytes();
    format!(
        "{:08b}.{:08b}.{:08b}.{:08b}",
        bytes[0], bytes[1], bytes[2], bytes[3]
    )
}

fn get_ipv4_scope(ip: Ipv4Addr) -> &'static str {
    if ip.is_loopback() {
        "Loopback (Private)"
    } else if ip.is_private() {
        "Private Network (RFC 1918)"
    } else if ip.is_link_local() {
        "Link-Local (APIPA)"
    } else if ip.is_multicast() {
        "Multicast (Class D)"
    } else {
        "Public Unicast"
    }
}
