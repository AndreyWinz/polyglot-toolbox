using DllInspect;

if (args.Length == 0 || args.Contains("-h") || args.Contains("--help"))
{
    PrintUsage();
    return 0;
}

string assemblyPath = args[0];
bool includeNonPublic = args.Contains("-a") || args.Contains("--all");
bool summaryOnly = args.Contains("-s") || args.Contains("--summary");

if (!File.Exists(assemblyPath))
{
    Console.ForegroundColor = ConsoleColor.Red;
    Console.WriteLine($"Error: Assembly file '{assemblyPath}' does not exist.");
    Console.ResetColor();
    return 1;
}

try
{
    Console.WriteLine($"🔍 Inspecting .NET Assembly: \u001b[1m{assemblyPath}\u001b[0m\n");
    var report = AssemblyInspector.Inspect(assemblyPath, includeNonPublic);

    // Assembly Metadata Header
    Console.ForegroundColor = ConsoleColor.Cyan;
    Console.WriteLine("============================================================");
    Console.WriteLine($"📦 Assembly        : {report.Name}");
    Console.WriteLine($"🏷️  Version         : {report.Version}");
    Console.WriteLine($"🎯 Target Framework: {report.TargetFramework}");
    Console.WriteLine("============================================================");
    Console.ResetColor();

    // Dependencies
    Console.WriteLine("\n🔗 Direct Referenced Dependencies:");
    if (report.Dependencies.Count == 0)
    {
        Console.WriteLine("   (No external dependencies)");
    }
    else
    {
        foreach (var dep in report.Dependencies)
        {
            Console.WriteLine($"   ├─ {dep}");
        }
    }

    if (summaryOnly)
    {
        Console.WriteLine($"\n📊 Summary: {report.Namespaces.Count} Namespace(s), {report.Namespaces.Sum(n => n.Types.Count)} Type(s) found.");
        return 0;
    }

    // Namespaces & Types
    Console.WriteLine("\n📂 Namespaces & Public API Surface:");

    foreach (var ns in report.Namespaces)
    {
        Console.ForegroundColor = ConsoleColor.Yellow;
        Console.WriteLine($"\n📁 Namespace: {ns.Name}");
        Console.ResetColor();

        foreach (var type in ns.Types)
        {
            var inheritance = !string.IsNullOrEmpty(type.BaseType) ? $" : {type.BaseType}" : "";
            var interfaces = type.Interfaces.Count > 0 ? $" [{string.Join(", ", type.Interfaces)}]" : "";

            Console.WriteLine($"   ├── \u001b[1m{type.Kind}\u001b[0m \u001b[36m{type.Name}\u001b[0m{inheritance}{interfaces}");

            // Properties
            foreach (var prop in type.Properties)
            {
                Console.WriteLine($"   │    ├─ [prop] {prop}");
            }

            // Constructors
            foreach (var ctor in type.Constructors)
            {
                Console.WriteLine($"   │    ├─ [ctor] {ctor}");
            }

            // Methods
            foreach (var method in type.Methods)
            {
                Console.WriteLine($"   │    ├─ [fn]   {method}");
            }

            // Fields
            foreach (var field in type.Fields)
            {
                Console.WriteLine($"   │    ├─ [field] {field}");
            }
        }
    }

    return 0;
}
catch (Exception ex)
{
    Console.ForegroundColor = ConsoleColor.Red;
    Console.WriteLine($"\nAn error occurred while inspecting assembly: {ex.Message}");
    Console.ResetColor();
    return 1;
}

static void PrintUsage()
{
    Console.WriteLine("""
    \u001b[1;36mdll-inspect\u001b[0m - .NET Assembly Inspector

    Usage:
      dll-inspect <path_to_dll_or_exe> [options]

    Options:
      -a, --all        Include internal and private members (default: public API surface only)
      -s, --summary    Print assembly metadata and dependency list without full API tree
      -h, --help       Show this help message

    Examples:
      dll-inspect ./bin/Release/net8.0/MyLibrary.dll
      dll-inspect ./MyApp.dll --all
      dll-inspect ./ThirdParty.dll --summary
    """);
}
