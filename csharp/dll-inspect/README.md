# dll-inspect

A fast CLI tool written in C# (.NET 8) to inspect compiled `.dll` and `.exe` assemblies. It extracts assembly metadata, framework target versions, direct dependencies, and public API surfaces (namespaces, classes, structs, enums, methods, properties).

## Features
- **Zero External Dependencies:** Built using standard `System.Reflection` and `System.IO` primitives.
- **Dependency Map:** Lists all referenced NuGet/framework assemblies with their targeted version bounds.
- **API Surface Explorer:** Tree view of namespaces, class hierarchies, implemented interfaces, properties, and methods.
- **Private/Internal Support:** Pass `-a` / `--all` to inspect internal/private types and members.

## Building

```bash
cd csharp/dll-inspect
dotnet build -c Release
```

### To build a standalone executable:

```bash
dotnet publish -c Release -r osx-arm64 --self-contained false -p:PublishSingleFile=true
```

## Usage

### 1. Inspect Public API Surface of a DLL

```bash
./bin/Release/net8.0/dll-inspect /path/to/Assembly.dll
```

### 2. View Quick Assembly Metadata & Dependencies Only

```bash
./bin/Release/net8.0/dll-inspect /path/to/Assembly.dll --summary
```

### 3. Inspect All Members (Including Non-Public Types)

```bash
./bin/Release/net8.0/dll-inspect /path/to/Assembly.dll --all
```
