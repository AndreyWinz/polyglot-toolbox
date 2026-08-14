# class-dump

A zero-dependency terminal JVM bytecode inspector and constant pool decoder written in pure **base Java**. It directly parses the JVM binary class file format (`0xCAFEBABE`) and inspects `.class` or `.jar` files without needing external libraries like ASM or BCEL.

## Features
- **Zero Dependencies:** Compiles and runs with any standard Java compiler/runtime (`javac` / `java`).
- **Binary Header & Target Version Decoding:** Identifies magic bytes, major/minor JVM versions (Java 8 through 22+), access flags, and class inheritance hierarchies.
- **Constant Pool Reader:** Decodes UTF-8 strings, Class references, Methodrefs, Fieldrefs, and InvokeDynamic constant pool entries.
- **JAR Archive Parsing:** Scans and outputs bytecode structures for all embedded `.class` files inside `.jar` packages.
- **Member Summaries:** Lists class fields, descriptors, and method signatures with colourised terminal formatting.

## Setup & Compilation

Compile `ClassDump.java`:
```bash
javac java/class-dump/ClassDump.java
```

Or run directly via the single-file source code launcher in modern Java (JDK 11+):

```bash
java java/class-dump/ClassDump.java <file>
```

## Usage

```bash
java ClassDump <file.class|file.jar> [options]
```

### Options

- `-cp, --constant-pool`: Inspect and dump constant pool table entries.
- `-v,  --verbose`: Verbose mode (includes constant pool & attributes).
- `--no-fields`: Suppress field declarations.
- `--no-methods`: Suppress method signatures.
- `-h,  --help`: Display CLI usage menu.

## Examples

### Basic Inspection of a Compiled Class

```bash
java ClassDump App.class
```

### Dump Constant Pool Entries

```bash
java ClassDump App.class -cp
```

### Inspect an Entire JAR Archive

```bash
java ClassDump build/libs/service.jar
```
