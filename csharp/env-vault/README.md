# env-vault

A secure C# CLI tool for managing encrypted `.env` files using **AES-256-GCM** authenticated encryption and **PBKDF2-HMAC-SHA256** key derivation (600,000 iterations).

## Features
- **AES-256-GCM Authenticated Encryption:** Protects secrets against unauthorised reads and tampered/corrupted data payloads.
- **In-Memory Subprocess Runner (`run`):** Inject secrets into a target command's environment without ever writing plaintext files to disk.
- **Zero Third-Party Dependencies:** Uses standard C# `System.Security.Cryptography` primitives.
- **Interactive Masked Inputs:** Prevents password leakage in shell histories.

## Build & Run

```bash
cd csharp/env-vault
dotnet build -c Release
```

## Examples

### 1. Encrypt an existing `.env` file

```bash
./bin/Release/net8.0/env-vault encrypt .env .env.vault
```

### 2. Set or update a key directly inside the encrypted vault

```bash
./bin/Release/net8.0/env-vault set .env.vault DATABASE_URL "postgres://user:pass@localhost:5432/db"
```

### 3. Run a subprocess with injected secrets (Without saving unencrypted files)

```bash
./bin/Release/net8.0/env-vault run .env.vault -- node app.js
```
