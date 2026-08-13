#!/usr/bin/env python3
"""
net-check - Async Port & Endpoint Monitor
Monitors TCP ports and HTTP/HTTPS endpoints concurrently with zero external dependencies.
"""

import sys
import os
import time
import async_timeout  # Python 3.11+ uses asyncio.timeout; fallback provided below
import asyncio
import argparse
import ssl
from typing import NamedTuple, Optional, List
from urllib.parse import urlparse

# ANSI Colors
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"
CLEAR_SCREEN = "\033[H\033[J"


class CheckResult(NamedTuple):
    target: str
    target_type: str  # "HTTP" or "TCP"
    is_up: bool
    status_detail: str
    latency_ms: float


async def check_tcp(host: str, port: int, timeout: float) -> tuple[bool, str, float]:
    """Check availability of a raw TCP host and port."""
    start = time.perf_counter()
    try:
        if sys.version_info >= (3, 11):
            async with asyncio.timeout(timeout):
                reader, writer = await asyncio.open_connection(host, port)
                writer.close()
                await writer.wait_closed()
        else:
            fut = asyncio.open_connection(host, port)
            reader, writer = await asyncio.wait_for(fut, timeout=timeout)
            writer.close()
            await writer.wait_closed()

        latency = (time.perf_counter() - start) * 1000
        return True, "CONNECTED", latency
    except asyncio.TimeoutError:
        latency = (time.perf_counter() - start) * 1000
        return False, "TIMEOUT", latency
    except Exception as e:
        latency = (time.perf_counter() - start) * 1000
        return False, f"REFUSED ({e.__class__.__name__})", latency


async def check_http(url: str, timeout: float) -> tuple[bool, str, float]:
    """Perform a lightweight HTTP/HTTPS HEAD request using raw asyncio and SSL."""
    parsed = urlparse(url)
    scheme = parsed.scheme.lower()
    host = parsed.hostname or ""
    port = parsed.port or (443 if scheme == "https" else 80)
    path = parsed.path if parsed.path else "/"

    start = time.perf_counter()
    try:
        ssl_ctx = ssl.create_default_context() if scheme == "https" else None

        if sys.version_info >= (3, 11):
            async with asyncio.timeout(timeout):
                reader, writer = await asyncio.open_connection(host, port, ssl=ssl_ctx)
        else:
            fut = asyncio.open_connection(host, port, ssl=ssl_ctx)
            reader, writer = await asyncio.wait_for(fut, timeout=timeout)

        # Send minimal HTTP HEAD request
        request = f"HEAD {path} HTTP/1.1\r\nHost: {host}\r\nUser-Agent: net-check/1.0\r\nConnection: close\r\n\r\n"
        writer.write(request.encode("utf-8"))
        await writer.drain()

        # Read status line
        response = await reader.readline()
        writer.close()
        await writer.wait_closed()

        latency = (time.perf_counter() - start) * 1000
        response_str = response.decode("utf-8", errors="ignore").strip()

        if response_str.startswith("HTTP/"):
            parts = response_str.split(" ", 2)
            status_code = parts[1] if len(parts) > 1 else "??? "
            is_ok = status_code.startswith(("2", "3"))
            return is_ok, f"HTTP {status_code}", latency

        return False, "INVALID HTTP RESP", latency

    except asyncio.TimeoutError:
        latency = (time.perf_counter() - start) * 1000
        return False, "TIMEOUT", latency
    except Exception as e:
        latency = (time.perf_counter() - start) * 1000
        return False, f"ERR ({e.__class__.__name__})", latency


async def evaluate_target(target: str, timeout: float) -> CheckResult:
    """Route target string to appropriate TCP or HTTP checker."""
    target = target.strip()

    if target.startswith(("http://", "https://")):
        is_up, detail, latency = await check_http(target, timeout)
        return CheckResult(target, "HTTP", is_up, detail, latency)
    
    # Otherwise treat as host:port
    if ":" in target:
        host, port_str = target.rsplit(":", 1)
        try:
            port = int(port_str)
            is_up, detail, latency = await check_tcp(host, port, timeout)
            return CheckResult(target, "TCP", is_up, detail, latency)
        except ValueError:
            pass

    return CheckResult(target, "UNKNOWN", False, "INVALID TARGET FORMAT", 0.0)


def load_targets(targets_input: List[str], file_path: Optional[str]) -> List[str]:
    """Combine targets passed via CLI and loaded from a target file."""
    targets = [t for t in targets_input if t.strip()]

    if file_path:
        if not os.path.exists(file_path):
            print(f"Error: Target file '{file_path}' not found.", file=sys.stderr)
            sys.exit(1)
        with open(file_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    targets.append(line)

    return list(dict.fromkeys(targets))  # Remove duplicates preserving order


def render_dashboard(results: List[CheckResult], interval: int, continuous: bool) -> None:
    """Print clean terminal UI for check results."""
    if continuous:
        print(CLEAR_SCREEN, end="")

    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    print(f"{BOLD}{CYAN}📡 net-check Status Dashboard{RESET} [{timestamp}]")
    print("=" * 68)
    print(f"{'TARGET':<32} {'TYPE':<6} {'STATUS':<12} {'LATENCY':<10}")
    print("-" * 68)

    for res in results:
        status_color = GREEN if res.is_up else RED
        status_str = f"{status_color}{'UP':<6} ({res.status_detail}){RESET}"
        latency_str = f"{res.latency_ms:.1f} ms" if res.is_up else "N/A"

        # Truncate target name if long
        target_display = res.target if len(res.target) <= 30 else res.target[:27] + "..."

        print(f"{target_display:<32} {res.target_type:<6} {status_str:<22} {latency_str:<10}")

    print("=" * 68)
    if continuous:
        print(f"Refreshing every {interval}s... Press {BOLD}Ctrl+C{RESET} to stop.")


async def monitor_loop(targets: List[str], interval: int, timeout: float, continuous: bool) -> None:
    """Main monitoring loop."""
    try:
        while True:
            tasks = [evaluate_target(t, timeout) for t in targets]
            results = await asyncio.gather(*tasks)

            render_dashboard(results, interval, continuous)

            if not continuous:
                break

            await asyncio.sleep(interval)
    except asyncio.CancelledError:
        pass


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Async Port & Endpoint Monitor (TCP & HTTP/HTTPS)"
    )
    parser.add_argument(
        "targets",
        nargs="*",
        help="Targets to monitor (e.g. 'https://google.com' or '127.0.0.1:22').",
    )
    parser.add_argument(
        "-f",
        "--file",
        type=str,
        help="File containing targets (one per line).",
    )
    parser.add_argument(
        "-i",
        "--interval",
        type=int,
        default=5,
        help="Monitoring interval in seconds (default: 5).",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        type=float,
        default=3.0,
        help="Connection timeout per target in seconds (default: 3.0).",
    )
    parser.add_argument(
        "-c",
        "--continuous",
        action="store_true",
        help="Run continuously and refresh terminal dashboard.",
    )

    args = parser.parse_args()

    targets = load_targets(args.targets, args.file)

    if not targets:
        print("Error: No valid targets provided. Pass targets via CLI or '-f/--file'.", file=sys.stderr)
        parser.print_help()
        sys.exit(1)

    try:
        asyncio.run(monitor_loop(targets, args.interval, args.timeout, args.continuous))
    except KeyboardInterrupt:
        print(f"\n{YELLOW}Exiting net-check.{RESET}")
        sys.exit(0)


if __name__ == "__main__":
    main()
