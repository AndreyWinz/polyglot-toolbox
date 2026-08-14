import java.io.*;
import java.nio.charset.StandardCharsets;
import java.text.DecimalFormat;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class HeapLens {

    // ANSI Colors
    private static final String RESET = "\u001B[0m";
    private static final String BOLD = "\u001B[1m";
    private static final String CYAN = "\u001B[36m";
    private static final String GREEN = "\u001B[32m";
    private static final String YELLOW = "\u001B[33m";
    private static final String RED = "\u001B[31m";
    private static final String BLUE = "\u001B[34m";
    private static final String MAGENTA = "\u001B[35m";

    private static int topLimit = 15;
    private static boolean verbose = false;

    public static void main(String[] args) {
        if (args.length == 0 || hasArg(args, "-h", "--help")) {
            printHelp();
            System.exit(args.length == 0 ? 1 : 0);
        }

        String filePath = null;
        String mode = "auto"; // auto, hprof, gc

        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            if (arg.equals("-top") || arg.equals("-n")) {
                if (i + 1 < args.length) topLimit = Integer.parseInt(args[++i]);
            } else if (arg.equals("--hprof")) {
                mode = "hprof";
            } else if (arg.equals("--gc")) {
                mode = "gc";
            } else if (arg.equals("-v") || arg.equals("--verbose")) {
                verbose = true;
            } else if (!arg.startsWith("-")) {
                filePath = arg;
            }
        }

        if (filePath == null) {
            System.err.println(RED + "Error: Target file not specified." + RESET);
            System.exit(1);
        }

        File file = new File(filePath);
        if (!file.exists()) {
            System.err.println(RED + "Error: File '" + filePath + "' not found." + RESET);
            System.exit(1);
        }

        try {
            if (mode.equals("hprof") || (mode.equals("auto") && isHprofFile(file))) {
                analyzeHprof(file);
            } else {
                analyzeGcLog(file);
            }
        } catch (Exception e) {
            System.err.println(RED + "Analysis Error: " + e.getMessage() + RESET);
            if (verbose) e.printStackTrace();
            System.exit(1);
        }
    }

    private static boolean hasArg(String[] args, String... flags) {
        for (String arg : args) {
            for (String flag : flags) {
                if (arg.equals(flag)) return true;
            }
        }
        return false;
    }

    private static void printHelp() {
        System.out.println(BOLD + CYAN + "heap-lens" + RESET + " - JVM Heap Dump & GC Log Analyzer in Base Java\n");
        System.out.println(BOLD + "Usage:" + RESET);
        System.out.println("  java HeapLens.java <dump.hprof|gc.log> [options]\n");
        System.out.println(BOLD + "Options:" + RESET);
        System.out.println("  -top, -n <int>   Number of top memory-consuming classes to show (default: 15)");
        System.out.println("  --hprof          Force file processing as HPROF binary heap dump");
        System.out.println("  --gc             Force file processing as Unified JVM GC log");
        System.out.println("  -v, --verbose    Enable verbose diagnostic traces");
        System.out.println("  -h, --help       Show this help message\n");
        System.out.println(BOLD + "Examples:" + RESET);
        System.out.println("  java HeapLens.java heap.hprof -top 20");
        System.out.println("  java HeapLens.java gc.log");
    }

    private static boolean isHprofFile(File file) {
        try (InputStream is = new FileInputStream(file)) {
            byte[] buf = new byte[12];
            int read = is.read(buf);
            if (read < 12) return false;
            String header = new String(buf, StandardCharsets.UTF_8);
            return header.startsWith("JAVA PROFILE");
        } catch (IOException e) {
            return false;
        }
    }

    // =========================================================================
    // HPROF BINARY PARSER
    // =========================================================================

    private static void analyzeHprof(File file) throws IOException {
        System.out.println(BOLD + BLUE + "============================================================" + RESET);
        System.out.println(BOLD + "🔍 Analyzing HPROF Heap Dump: " + RESET + file.getName() + " (" + formatBytes(file.length()) + ")");
        System.out.println(BOLD + BLUE + "============================================================" + RESET + "\n");

        long startTime = System.currentTimeMillis();

        try (DataInputStream dis = new DataInputStream(new BufferedInputStream(new FileInputStream(file)))) {
            // Read Null-terminated Header String
            StringBuilder headerSb = new StringBuilder();
            byte b;
            while ((b = dis.readByte()) != 0) {
                headerSb.append((char) b);
            }
            String formatVersion = headerSb.toString();
            int idSize = dis.readInt();
            long timestamp = dis.readLong();

            System.out.println("   Format Version : " + BOLD + formatVersion + RESET);
            System.out.println("   Pointer Size   : " + (idSize * 8) + "-bit (" + idSize + " bytes)");
            System.out.println("   Dump Timestamp : " + new Date(timestamp));
            System.out.println("   Streaming binary record tags...\n");

            Map<Long, String> stringMap = new HashMap<>();
            Map<Long, Long> classToNameStringMap = new HashMap<>();
            Map<Long, Integer> classInstanceSizes = new HashMap<>();
            Map<Long, Long> classSuperMap = new HashMap<>();

            // Histogram trackers (Class ID -> Count / Bytes)
            Map<Long, Long> instanceCounts = new HashMap<>();
            Map<Long, Long> instanceBytes = new HashMap<>();

            long totalHeapBytes = 0;
            long totalObjects = 0;

            while (dis.available() > 0) {
                int tag = dis.readUnsignedByte();
                dis.readInt(); // time
                long length = dis.readInt() & 0xFFFFFFFFL;

                switch (tag) {
                    case 0x01: { // STRING IN UTF8
                        long id = readId(dis, idSize);
                        byte[] strBytes = new byte[(int) (length - idSize)];
                        dis.readFully(strBytes);
                        stringMap.put(id, new String(strBytes, StandardCharsets.UTF_8));
                        break;
                    }
                    case 0x02: { // LOAD CLASS
                        dis.readInt(); // class serial
                        long classObjId = readId(dis, idSize);
                        dis.readInt(); // stack trace serial
                        long classNameStringId = readId(dis, idSize);
                        classToNameStringMap.put(classObjId, classNameStringId);
                        break;
                    }
                    case 0x0C: // HEAP DUMP
                    case 0x1C: { // HEAP DUMP SEGMENT
                        long endPos = length;
                        long readBytes = 0;
                        while (readBytes < endPos) {
                            int subTag = dis.readUnsignedByte();
                            readBytes += 1;

                            switch (subTag) {
                                case 0xFF: // ROOT UNKNOWN
                                case 0x01: // ROOT JNI GLOBAL
                                case 0x02: // ROOT JNI LOCAL
                                case 0x03: // ROOT JAVA FRAME
                                case 0x04: // ROOT NATIVE STACK
                                case 0x05: // ROOT STICKY CLASS
                                case 0x06: // ROOT THREAD BLOCK
                                case 0x07: // ROOT MONITOR USED
                                case 0x08: // ROOT THREAD OBJ
                                    readBytes += skipRoot(dis, subTag, idSize);
                                    break;

                                case 0x20: { // CLASS DUMP
                                    long classObjId = readId(dis, idSize);
                                    dis.readInt(); // stack
                                    long superObjId = readId(dis, idSize);
                                    readId(dis, idSize); // classloader
                                    readId(dis, idSize); // signers
                                    readId(dis, idSize); // prot domain
                                    readId(dis, idSize); // reserved
                                    readId(dis, idSize); // reserved
                                    int instSize = dis.readInt();
                                    
                                    classInstanceSizes.put(classObjId, instSize);
                                    classSuperMap.put(classObjId, superObjId);

                                    readBytes += (idSize * 7) + 8;

                                    // Constant pool entries
                                    int cpCount = dis.readUnsignedShort();
                                    readBytes += 2;
                                    for (int i = 0; i < cpCount; i++) {
                                        dis.readUnsignedShort();
                                        readBytes += 2 + skipValue(dis, idSize);
                                    }

                                    // Static fields
                                    int statCount = dis.readUnsignedShort();
                                    readBytes += 2;
                                    for (int i = 0; i < statCount; i++) {
                                        readId(dis, idSize);
                                        readBytes += idSize + skipValue(dis, idSize);
                                    }

                                    // Instance fields
                                    int instFieldCount = dis.readUnsignedShort();
                                    readBytes += 2 + (instFieldCount * (idSize + 1));
                                    dis.skipBytes(instFieldCount * (idSize + 1));
                                    break;
                                }
                                case 0x21: { // INSTANCE DUMP
                                    readId(dis, idSize); // obj id
                                    dis.readInt(); // stack
                                    long classObjId = readId(dis, idSize);
                                    int bytesFollow = dis.readInt();
                                    dis.skipBytes(bytesFollow);

                                    readBytes += (idSize * 2) + 8 + bytesFollow;

                                    int size = classInstanceSizes.getOrDefault(classObjId, bytesFollow);
                                    instanceCounts.put(classObjId, instanceCounts.getOrDefault(classObjId, 0L) + 1);
                                    instanceBytes.put(classObjId, instanceBytes.getOrDefault(classObjId, 0L) + size);
                                    totalHeapBytes += size;
                                    totalObjects++;
                                    break;
                                }
                                case 0x22: { // OBJECT ARRAY DUMP
                                    readId(dis, idSize);
                                    dis.readInt();
                                    int numElements = dis.readInt();
                                    long elemClassId = readId(dis, idSize);
                                    dis.skipBytes(numElements * idSize);

                                    readBytes += (idSize * 2) + 8 + ((long) numElements * idSize);
                                    long arraySize = (long) idSize * numElements + 16;
                                    
                                    instanceCounts.put(elemClassId, instanceCounts.getOrDefault(elemClassId, 0L) + 1);
                                    instanceBytes.put(elemClassId, instanceBytes.getOrDefault(elemClassId, 0L) + arraySize);
                                    totalHeapBytes += arraySize;
                                    totalObjects++;
                                    break;
                                }
                                case 0x23: { // PRIMITIVE ARRAY DUMP
                                    readId(dis, idSize);
                                    dis.readInt();
                                    int numElements = dis.readInt();
                                    int elemType = dis.readUnsignedByte();
                                    int elemSize = getPrimitiveTypeSize(elemType);
                                    dis.skipBytes(numElements * elemSize);

                                    readBytes += (idSize) + 9 + ((long) numElements * elemSize);
                                    long arraySize = (long) numElements * elemSize + 16;
                                    
                                    long pseudoClassId = -elemType; // synthetic ID for primitive arrays
                                    instanceCounts.put(pseudoClassId, instanceCounts.getOrDefault(pseudoClassId, 0L) + 1);
                                    instanceBytes.put(pseudoClassId, instanceBytes.getOrDefault(pseudoClassId, 0L) + arraySize);
                                    totalHeapBytes += arraySize;
                                    totalObjects++;
                                    break;
                                }
                                default:
                                    // Skip unknown heap tag by relying on remaining record boundary
                                    dis.skipBytes((int) (endPos - readBytes));
                                    readBytes = endPos;
                                    break;
                            }
                        }
                        break;
                    }
                    default:
                        dis.skipBytes((int) length);
                        break;
                }
            }

            long elapsed = System.currentTimeMillis() - startTime;
            System.out.println(GREEN + "✓ Heap Dump Parsing Complete (" + elapsed + " ms)" + RESET);
            System.out.println(sprintf("   Total Tracked Objects : %,d", totalObjects));
            System.out.println(sprintf("   Total Shallow Size    : %s\n", formatBytes(totalHeapBytes)));

            // Build Class Histogram
            List<ClassStat> stats = new ArrayList<>();
            for (Map.Entry<Long, Long> entry : instanceBytes.entrySet()) {
                long classId = entry.getKey();
                long bytes = entry.getValue();
                long count = instanceCounts.getOrDefault(classId, 0L);

                String className;
                if (classId < 0) {
                    className = getPrimitiveTypeName((int) -classId) + "[]";
                } else {
                    Long nameStrId = classToNameStringMap.get(classId);
                    className = (nameStrId != null && stringMap.containsKey(nameStrId))
                            ? stringMap.get(nameStrId).replace('/', '.')
                            : "Class@0x" + Long.toHexString(classId);
                }
                stats.add(new ClassStat(className, count, bytes));
            }

            stats.sort((a, b) -> Long.compare(b.bytes, a.bytes));

            System.out.println(BOLD + YELLOW + "📊 Top " + Math.min(topLimit, stats.size()) + " Memory Consuming Classes:" + RESET);
            System.out.printf("   %-5s | %-45s | %-12s | %-12s | %-8s\n", "Rank", "Class Name", "Instances", "Shallow Bytes", "% Heap");
            System.out.println("   " + "─".repeat(92));

            int rank = 1;
            for (ClassStat stat : stats) {
                if (rank > topLimit) break;
                double pct = totalHeapBytes > 0 ? ((double) stat.bytes / totalHeapBytes) * 100.0 : 0;
                String truncatedName = stat.className.length() > 45 ? stat.className.substring(0, 42) + "..." : stat.className;
                
                String color = pct > 20.0 ? RED : (pct > 5.0 ? YELLOW : RESET);
                System.out.printf("   %-5d | %s%-45s%s | %,12d | %-12s | %s%6.2f%%%s\n",
                        rank++, color, truncatedName, RESET, stat.count, formatBytes(stat.bytes), color, pct, RESET);
            }
            System.out.println();

            // Memory Leak Heuristics
            if (!stats.isEmpty() && totalHeapBytes > 0) {
                ClassStat top = stats.get(0);
                double topPct = ((double) top.bytes / totalHeapBytes) * 100.0;
                if (topPct > 25.0) {
                    System.out.println(BOLD + RED + "🚨 Potential Memory Leak Detected:" + RESET);
                    System.out.printf("   Class '%s' occupies %.2f%% of total heap memory (%s across %,d instances).\n\n",
                            top.className, topPct, formatBytes(top.bytes), top.count);
                }
            }

        }
    }

    private static long readId(DataInputStream dis, int idSize) throws IOException {
        if (idSize == 4) return dis.readInt() & 0xFFFFFFFFL;
        return dis.readLong();
    }

    private static int skipRoot(DataInputStream dis, int subTag, int idSize) throws IOException {
        switch (subTag) {
            case 0x01: dis.skipBytes(idSize * 2); return idSize * 2;
            case 0x02:
            case 0x03: dis.skipBytes(idSize + 8); return idSize + 8;
            case 0x04:
            case 0x05: dis.skipBytes(idSize + 4); return idSize + 4;
            case 0x06:
            case 0x07:
            case 0x08: dis.skipBytes(idSize); return idSize;
            default: dis.skipBytes(idSize + 4); return idSize + 4;
        }
    }

    private static int skipValue(DataInputStream dis, int idSize) throws IOException {
        int type = dis.readUnsignedByte();
        int sz = getPrimitiveTypeSize(type);
        if (type == 2) sz = idSize; // Object reference
        dis.skipBytes(sz);
        return 1 + sz;
    }

    private static int getPrimitiveTypeSize(int type) {
        switch (type) {
            case 4: return 1; // boolean
            case 5: return 2; // char
            case 6: return 4; // float
            case 7: return 8; // double
            case 8: return 1; // byte
            case 9: return 2; // short
            case 10: return 4; // int
            case 11: return 8; // long
            default: return 4; // object / default
        }
    }

    private static String getPrimitiveTypeName(int type) {
        switch (type) {
            case 4: return "boolean";
            case 5: return "char";
            case 6: return "float";
            case 7: return "double";
            case 8: return "byte";
            case 9: return "short";
            case 10: return "int";
            case 11: return "long";
            default: return "Object";
        }
    }

    // =========================================================================
    // GC LOG PARSER
    // =========================================================================

    private static void analyzeGcLog(File file) throws IOException {
        System.out.println(BOLD + BLUE + "============================================================" + RESET);
        System.out.println(BOLD + "📈 Analyzing JVM GC Log: " + RESET + file.getName());
        System.out.println(BOLD + BLUE + "============================================================" + RESET + "\n");

        Pattern gcPausePattern = Pattern.compile("Pause (Young|Full|Remark|Cleanup|Initial Mark).*?(\\d+\\.\\d+)ms|GC\\((\\d+)\\).*?(\\d+\\.\\d+)ms|(\\d+)K->(\\d+)K\\((\\d+)K\\).*?(\\d+\\.\\d+)ms");
        Pattern memoryPattern = Pattern.compile("(\\d+)([KMGT])->(\\d+)([KMGT])\\((\\d+)([KMGT])\\)");

        int totalGcEvents = 0;
        int fullGcCount = 0;
        double totalPauseTimeMs = 0;
        double maxPauseTimeMs = 0;
        long peakMemoryBytes = 0;

        List<Double> pauseList = new ArrayList<>();

        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.contains("Full") || line.contains("Full GC")) {
                    fullGcCount++;
                }

                Matcher matcher = gcPausePattern.matcher(line);
                if (matcher.find()) {
                    totalGcEvents++;
                    String msGroup = matcher.group(2) != null ? matcher.group(2) : (matcher.group(4) != null ? matcher.group(4) : matcher.group(8));
                    if (msGroup != null) {
                        double ms = Double.parseDouble(msGroup);
                        pauseList.add(ms);
                        totalPauseTimeMs += ms;
                        if (ms > maxPauseTimeMs) maxPauseTimeMs = ms;
                    }
                }

                Matcher memMatcher = memoryPattern.matcher(line);
                if (memMatcher.find()) {
                    long afterMem = parseMemory(memMatcher.group(3), memMatcher.group(4));
                    if (afterMem > peakMemoryBytes) peakMemoryBytes = afterMem;
                }
            }
        }

        System.out.println("   Total GC Events Parsed : " + BOLD + totalGcEvents + RESET);
        System.out.println("   Full GC Executions     : " + (fullGcCount > 0 ? RED + fullGcCount + RESET : GREEN + "0" + RESET));
        System.out.println(sprintf("   Total Pause Time       : %.2f ms (%.3f sec)", totalPauseTimeMs, totalPauseTimeMs / 1000.0));
        System.out.println(sprintf("   Max Pause Duration     : %s%.2f ms%s", (maxPauseTimeMs > 500 ? RED : GREEN), maxPauseTimeMs, RESET));
        if (totalGcEvents > 0) {
            System.out.println(sprintf("   Average GC Pause       : %.2f ms", totalPauseTimeMs / totalGcEvents));
        }
        if (peakMemoryBytes > 0) {
            System.out.println("   Peak Heap Usage        : " + formatBytes(peakMemoryBytes));
        }
        System.out.println();

        // Diagnostics
        if (fullGcCount > 5) {
            System.out.println(BOLD + RED + "⚠️ Diagnostic Warning:" + RESET + " High frequency of Full GC pauses detected (" + fullGcCount + " events). Consider increasing heap allocation (-Xmx) or reviewing object churn.");
        } else if (maxPauseTimeMs > 1000.0) {
            System.out.println(BOLD + YELLOW + "⚠️ Diagnostic Warning:" + RESET + " Long GC pause spikes observed (> 1.0 sec). Review latency requirements or GC algorithm choice.");
        } else {
            System.out.println(BOLD + GREEN + "✓ GC Performance Healthy:" + RESET + " No major latency spikes or excessive Full GC overhead found.");
        }
        System.out.println();
    }

    private static long parseMemory(String valStr, String unit) {
        long val = Long.parseLong(valStr);
        switch (unit.toUpperCase()) {
            case "K": return val * 1024L;
            case "M": return val * 1024L * 1024L;
            case "G": return val * 1024L * 1024L * 1024L;
            case "T": return val * 1024L * 1024L * 1024L * 1024L;
            default: return val;
        }
    }

    private static String sprintf(String fmt, Object... args) {
        return String.format(Locale.US, fmt, args);
    }

    private static String formatBytes(long bytes) {
        if (bytes < 1024) return bytes + " B";
        int exp = (int) (Math.log(bytes) / Math.log(1024));
        char pre = "KMGTPE".charAt(exp - 1);
        return String.format(Locale.US, "%.2f %cB", bytes / Math.pow(1024, exp), pre);
    }

    private static class ClassStat {
        String className;
        long count;
        long bytes;

        ClassStat(String className, long count, long bytes) {
            this.className = className;
            this.count = count;
            this.bytes = bytes;
        }
    }
}
