# heap-lens

A lightweight, zero-dependency JVM **HPROF binary heap dump** inspector and **GC log analyser** written in base Java (JDK 11+). It inspects heap dumps generated via `jcmd`, `jmap`, or OutOfMemoryError crashes to pinpoint memory leaks and object allocation hotspots.

## Features
- **Zero Dependencies:** Pure base Java implementation with single-file launcher support.
- **Binary HPROF Streaming:** Parses binary `0x0C` and `0x1C` HPROF heap dump records, string pools, class loads, instance dumps, and primitive/object arrays.
- **Class Memory Histogram:** Generates ranked class allocation histograms displaying shallow memory footprint, instance counts, and percentage of heap occupied.
- **Memory Leak Diagnostics:** Automatically triggers warnings when dominant classes exceed threshold proportions of total heap capacity.
- **GC Log Summary:** Evaluates Unified JVM GC logs for total pause durations, maximum latency spikes, Full GC frequency, and peak heap utilisation.

## Setup & Execution

Compile `HeapLens.java`:
```bash
javac java/heap-lens/HeapLens.java
```

Or execute directly using single-file execution in Java 11+:
```bash
java java/heap-lens/HeapLens.java <dump.hprof|gc.log>
```

## Usage

```bash
java HeapLens <file> [options]
```

### Options

- `-top, -n <int>`: Limit the number of top memory-consuming classes displayed (default: `15`).
- `--hprof`: Force parsing the file as a binary HPROF dump.
- `--gc`: Force parsing the file as Unified JVM GC log text.
- `-v, --verbose`: Print extended parsing details.
- `-h, --help`: Show CLI help menu.

## Examples

### Inspect HPROF Heap Dump
```bash
java HeapLens heap_dump.hprof -top 20
```

### Analyse Garbage Collector Log
```bash
java HeapLens gc.log
```
