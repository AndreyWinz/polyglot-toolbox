# cidr-calc

A zero-dependency IPv4 and IPv6 CIDR subnet calculator written in Rust.

## Features
- **Zero External Dependencies:** Built entirely with Rust's standard library.
- **Dual IP Support:** Automatically routes and parses both IPv4 (`192.168.1.0/24`) and IPv6 (`2001:db8::/64`) notations.
- **Detailed Network Metrics:** Calculates network/broadcast addresses, wildcard masks, usable host ranges, binary masks, and address scopes.

## Building

```bash
cd rust/cidr-calc
cargo build --release
```

The output binary will be located at `target/release/cidr-calc`.

## Usage

### 1. IPv4 Subnet Calculation

```bash
./target/release/cidr-calc 192.168.1.50/24
```

### 2. IPv6 Subnet Calculation

```bash
./target/release/cidr-calc 2001:db8::1/64
```
