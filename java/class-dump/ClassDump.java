import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

public class ClassDump {

    private static boolean showConstantPool = false;
    private static boolean showFields = true;
    private static boolean showMethods = true;
    private static boolean verbose = false;

    // ANSI Colors
    private static final String RESET = "\u001B[0m";
    private static final String BOLD = "\u001B[1m";
    private static final String CYAN = "\u001B[36m";
    private static final String GREEN = "\u001B[32m";
    private static final String YELLOW = "\u001B[33m";
    private static final String RED = "\u001B[31m";
    private static final String BLUE = "\u001B[34m";
    private static final String MAGENTA = "\u001B[35m";

    public static void main(String[] args) {
        if (args.length == 0 || hasArg(args, "-h", "--help")) {
            printHelp();
            System.exit(args.length == 0 ? 1 : 0);
        }

        String targetPath = null;
        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            if (arg.equals("-cp") || arg.equals("--constant-pool")) {
                showConstantPool = true;
            } else if (arg.equals("-v") || arg.equals("--verbose")) {
                verbose = true;
                showConstantPool = true;
            } else if (arg.equals("--no-methods")) {
                showMethods = false;
            } else if (arg.equals("--no-fields")) {
                showFields = false;
            } else if (!arg.startsWith("-")) {
                targetPath = arg;
            }
        }

        if (targetPath == null) {
            System.err.println(RED + "Error: No .class or .jar file specified." + RESET);
            System.exit(1);
        }

        File file = new File(targetPath);
        if (!file.exists()) {
            System.err.println(RED + "Error: File '" + targetPath + "' not found." + RESET);
            System.exit(1);
        }

        try {
            if (targetPath.endsWith(".jar")) {
                inspectJar(file);
            } else {
                try (DataInputStream dis = new DataInputStream(new BufferedInputStream(new FileInputStream(file)))) {
                    inspectClassStream(dis, file.getName());
                }
            }
        } catch (Exception e) {
            System.err.println(RED + "Parsing Error: " + e.getMessage() + RESET);
            if (verbose) {
                e.printStackTrace();
            }
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
        System.out.println(BOLD + CYAN + "class-dump" + RESET + " - JVM .class & .jar Inspector in Base Java\n");
        System.out.println(BOLD + "Usage:" + RESET);
        System.out.println("  java ClassDump.java <file.class|file.jar> [options]\n");
        System.out.println(BOLD + "Options:" + RESET);
        System.out.println("  -cp, --constant-pool  Display constant pool table entries");
        System.out.println("  -v,  --verbose        Full inspection (enables constant pool & attributes)");
        System.out.println("  --no-fields           Suppress fields summary");
        System.out.println("  --no-methods          Suppress methods disassembly summary");
        System.out.println("  -h,  --help           Show this help message\n");
        System.out.println(BOLD + "Examples:" + RESET);
        System.out.println("  java ClassDump.java App.class -cp");
        System.out.println("  java ClassDump.java library.jar");
    }

    private static void inspectJar(File jarFile) throws IOException {
        System.out.println(BOLD + BLUE + "============================================================" + RESET);
        System.out.println(BOLD + "📦 JAR Archive Overview: " + RESET + jarFile.getName());
        System.out.println(BOLD + BLUE + "============================================================" + RESET + "\n");

        int classCount = 0;
        try (JarFile jar = new JarFile(jarFile)) {
            Enumeration<JarEntry> entries = jar.entries();
            while (entries.hasMoreElements()) {
                JarEntry entry = entries.nextElement();
                if (entry.getName().endsWith(".class")) {
                    classCount++;
                    System.out.println(BOLD + GREEN + "─── [" + classCount + "] " + entry.getName() + RESET);
                    try (DataInputStream dis = new DataInputStream(jar.getInputStream(entry))) {
                        inspectClassStream(dis, entry.getName());
                    }
                }
            }
        }
        System.out.println(BOLD + "Total .class entries inspected: " + classCount + RESET);
    }

    private static void inspectClassStream(DataInputStream dis, String label) throws IOException {
        // 1. Magic Number
        int magic = dis.readInt();
        if (magic != 0xCAFEBABE) {
            throw new IOException("Invalid JVM class file header (Magic: 0x" + Integer.toHexString(magic).toUpperCase() + ")");
        }

        // 2. Class File Version
        int minorVersion = dis.readUnsignedShort();
        int majorVersion = dis.readUnsignedShort();
        String javaVersion = mapMajorVersion(majorVersion);

        // 3. Constant Pool
        int cpCount = dis.readUnsignedShort();
        CPEntry[] constantPool = new CPEntry[cpCount];

        for (int i = 1; i < cpCount; i++) {
            int tag = dis.readUnsignedByte();
            switch (tag) {
                case 1: // UTF8
                    int len = dis.readUnsignedShort();
                    byte[] bytes = new byte[len];
                    dis.readFully(bytes);
                    constantPool[i] = new CPEntry(tag, new String(bytes, StandardCharsets.UTF_8));
                    break;
                case 3: // Integer
                    constantPool[i] = new CPEntry(tag, String.valueOf(dis.readInt()));
                    break;
                case 4: // Float
                    constantPool[i] = new CPEntry(tag, String.valueOf(dis.readFloat()));
                    break;
                case 5: // Long
                    constantPool[i] = new CPEntry(tag, String.valueOf(dis.readLong()));
                    i++; // Longs take 2 slots
                    break;
                case 6: // Double
                    constantPool[i] = new CPEntry(tag, String.valueOf(dis.readDouble()));
                    i++; // Doubles take 2 slots
                    break;
                case 7: // Class
                    constantPool[i] = new CPEntry(tag, "Class -> #" + dis.readUnsignedShort());
                    break;
                case 8: // String
                    constantPool[i] = new CPEntry(tag, "String -> #" + dis.readUnsignedShort());
                    break;
                case 9: // Fieldref
                case 10: // Methodref
                case 11: // InterfaceMethodref
                    int classIdx = dis.readUnsignedShort();
                    int ntIdx = dis.readUnsignedShort();
                    String type = (tag == 9) ? "Field" : (tag == 10) ? "Method" : "InterfaceMethod";
                    constantPool[i] = new CPEntry(tag, type + " -> #" + classIdx + ".#" + ntIdx);
                    break;
                case 12: // NameAndType
                    constantPool[i] = new CPEntry(tag, "NameAndType -> #" + dis.readUnsignedShort() + ":#" + dis.readUnsignedShort());
                    break;
                case 15: // MethodHandle
                    dis.readUnsignedByte(); // reference_kind
                    dis.readUnsignedShort(); // reference_index
                    constantPool[i] = new CPEntry(tag, "MethodHandle");
                    break;
                case 16: // MethodType
                    constantPool[i] = new CPEntry(tag, "MethodType -> #" + dis.readUnsignedShort());
                    break;
                case 17: // Dynamic
                case 18: // InvokeDynamic
                    dis.readUnsignedShort();
                    dis.readUnsignedShort();
                    constantPool[i] = new CPEntry(tag, (tag == 17 ? "Dynamic" : "InvokeDynamic"));
                    break;
                case 19: // Module
                    constantPool[i] = new CPEntry(tag, "Module -> #" + dis.readUnsignedShort());
                    break;
                case 20: // Package
                    constantPool[i] = new CPEntry(tag, "Package -> #" + dis.readUnsignedShort());
                    break;
                default:
                    constantPool[i] = new CPEntry(tag, "Unknown Tag: " + tag);
                    break;
            }
        }

        // 4. Access Flags, This Class, Super Class
        int accessFlags = dis.readUnsignedShort();
        int thisClassIdx = dis.readUnsignedShort();
        int superClassIdx = dis.readUnsignedShort();

        String thisClassName = resolveClassName(constantPool, thisClassIdx);
        String superClassName = superClassIdx > 0 ? resolveClassName(constantPool, superClassIdx) : "None";

        // 5. Interfaces
        int interfacesCount = dis.readUnsignedShort();
        List<String> interfaces = new ArrayList<>();
        for (int i = 0; i < interfacesCount; i++) {
            interfaces.add(resolveClassName(constantPool, dis.readUnsignedShort()));
        }

        System.out.println(CYAN + "============================================================" + RESET);
        System.out.println(BOLD + "📌 Class File: " + RESET + label);
        System.out.println(CYAN + "============================================================" + RESET);
        System.out.println("   Target JVM Version : " + javaVersion + " (Major: " + majorVersion + ", Minor: " + minorVersion + ")");
        System.out.println("   Access Flags       : 0x" + Integer.toHexString(accessFlags).toUpperCase() + " [" + decodeAccessFlags(accessFlags, true) + "]");
        System.out.println("   Class Name         : " + BOLD + thisClassName + RESET);
        System.out.println("   Super Class        : " + superClassName);
        System.out.println("   Interfaces         : " + (interfaces.isEmpty() ? "None" : String.join(", ", interfaces)));
        System.out.println("   Constant Pool Size : " + (cpCount - 1) + " entries\n");

        // Display Constant Pool if requested
        if (showConstantPool) {
            System.out.println(BOLD + YELLOW + "📋 Constant Pool Entries:" + RESET);
            for (int i = 1; i < cpCount; i++) {
                if (constantPool[i] != null) {
                    System.out.printf("   [%03d] %-18s %s\n", i, getTagName(constantPool[i].tag), constantPool[i].value);
                }
            }
            System.out.println();
        }

        // 6. Fields
        int fieldsCount = dis.readUnsignedShort();
        if (showFields) {
            System.out.println(BOLD + MAGENTA + "⚙️ Fields (" + fieldsCount + "):" + RESET);
            for (int i = 0; i < fieldsCount; i++) {
                int fFlags = dis.readUnsignedShort();
                int nameIdx = dis.readUnsignedShort();
                int descIdx = dis.readUnsignedShort();
                skipAttributes(dis);

                String name = getUtf8(constantPool, nameIdx);
                String desc = getUtf8(constantPool, descIdx);
                System.out.printf("   %-20s %-30s (%s)\n", decodeAccessFlags(fFlags, false), name, desc);
            }
            System.out.println();
        } else {
            skipMemberInfo(dis, fieldsCount);
        }

        // 7. Methods
        int methodsCount = dis.readUnsignedShort();
        if (showMethods) {
            System.out.println(BOLD + GREEN + "🛠️ Methods (" + methodsCount + "):" + RESET);
            for (int i = 0; i < methodsCount; i++) {
                int mFlags = dis.readUnsignedShort();
                int nameIdx = dis.readUnsignedShort();
                int descIdx = dis.readUnsignedShort();
                skipAttributes(dis);

                String name = getUtf8(constantPool, nameIdx);
                String desc = getUtf8(constantPool, descIdx);
                System.out.printf("   %-20s %-30s %s\n", decodeAccessFlags(mFlags, false), name, desc);
            }
            System.out.println();
        } else {
            skipMemberInfo(dis, methodsCount);
        }

        // Skip Class Attributes
        skipAttributes(dis);
    }

    private static void skipMemberInfo(DataInputStream dis, int count) throws IOException {
        for (int i = 0; i < count; i++) {
            dis.skipBytes(6); // flags, name, desc
            skipAttributes(dis);
        }
    }

    private static void skipAttributes(DataInputStream dis) throws IOException {
        int attrCount = dis.readUnsignedShort();
        for (int j = 0; j < attrCount; j++) {
            dis.skipBytes(2); // attr_name_index
            int attrLen = dis.readInt();
            dis.skipBytes(attrLen);
        }
    }

    private static String resolveClassName(CPEntry[] cp, int classIndex) {
        if (classIndex <= 0 || classIndex >= cp.length || cp[classIndex] == null) return "Unknown";
        String val = cp[classIndex].value;
        if (val.startsWith("Class -> #")) {
            try {
                int utf8Idx = Integer.parseInt(val.substring(10));
                return getUtf8(cp, utf8Idx).replace('/', '.');
            } catch (Exception e) {
                return val;
            }
        }
        return val;
    }

    private static String getUtf8(CPEntry[] cp, int index) {
        if (index > 0 && index < cp.length && cp[index] != null && cp[index].tag == 1) {
            return cp[index].value;
        }
        return "#" + index;
    }

    private static String mapMajorVersion(int major) {
        switch (major) {
            case 45: return "Java 1.1";
            case 46: return "Java 1.2";
            case 47: return "Java 1.3";
            case 48: return "Java 1.4";
            case 49: return "Java 5";
            case 50: return "Java 6";
            case 51: return "Java 7";
            case 52: return "Java 8";
            case 53: return "Java 9";
            case 54: return "Java 10";
            case 55: return "Java 11";
            case 56: return "Java 12";
            case 57: return "Java 13";
            case 58: return "Java 14";
            case 59: return "Java 15";
            case 60: return "Java 16";
            case 61: return "Java 17";
            case 62: return "Java 18";
            case 63: return "Java 19";
            case 64: return "Java 20";
            case 65: return "Java 21";
            case 66: return "Java 22";
            default: return "Java Major " + major;
        }
    }

    private static String decodeAccessFlags(int flags, boolean isClass) {
        List<String> acc = new ArrayList<>();
        if ((flags & 0x0001) != 0) acc.add("public");
        if ((flags & 0x0002) != 0) acc.add("private");
        if ((flags & 0x0004) != 0) acc.add("protected");
        if ((flags & 0x0008) != 0) acc.add("static");
        if ((flags & 0x0010) != 0) acc.add("final");
        if ((flags & 0x0020) != 0) acc.add(isClass ? "super" : "synchronized");
        if ((flags & 0x0040) != 0) acc.add("volatile");
        if ((flags & 0x0080) != 0) acc.add("transient");
        if ((flags & 0x0100) != 0) acc.add("native");
        if ((flags & 0x0200) != 0) acc.add("interface");
        if ((flags & 0x0400) != 0) acc.add("abstract");
        if ((flags & 0x1000) != 0) acc.add("synthetic");
        if ((flags & 0x2000) != 0) acc.add("annotation");
        if ((flags & 0x4000) != 0) acc.add("enum");
        return String.join(" ", acc);
    }

    private static String getTagName(int tag) {
        switch (tag) {
            case 1: return "CONSTANT_Utf8";
            case 3: return "CONSTANT_Integer";
            case 4: return "CONSTANT_Float";
            case 5: return "CONSTANT_Long";
            case 6: return "CONSTANT_Double";
            case 7: return "CONSTANT_Class";
            case 8: return "CONSTANT_String";
            case 9: return "CONSTANT_Fieldref";
            case 10: return "CONSTANT_Methodref";
            case 11: return "CONSTANT_InterfaceMethodref";
            case 12: return "CONSTANT_NameAndType";
            case 15: return "CONSTANT_MethodHandle";
            case 16: return "CONSTANT_MethodType";
            case 17: return "CONSTANT_Dynamic";
            case 18: return "CONSTANT_InvokeDynamic";
            case 19: return "CONSTANT_Module";
            case 20: return "CONSTANT_Package";
            default: return "Tag_" + tag;
        }
    }

    private static class CPEntry {
        int tag;
        String value;

        CPEntry(int tag, String value) {
            this.tag = tag;
            this.value = value;
        }
    }
}
