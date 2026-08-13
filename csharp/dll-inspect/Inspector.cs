using System.Reflection;
using System.Runtime.Versioning;
using System.Text;

namespace DllInspect;

public record AssemblyReport(
    string Name,
    string Version,
    string TargetFramework,
    List<string> Dependencies,
    List<NamespaceInfo> Namespaces
);

public record NamespaceInfo(
    string Name,
    List<TypeInfoReport> Types
);

public record TypeInfoReport(
    string Name,
    string Kind,
    string BaseType,
    List<string> Interfaces,
    List<string> Constructors,
    List<string> Methods,
    List<string> Properties,
    List<string> Fields
);

public static class AssemblyInspector
{
    public static AssemblyReport Inspect(string assemblyPath, bool includeNonPublic = false)
    {
        if (!File.Exists(assemblyPath))
        {
            throw new FileNotFoundException($"Assembly file not found at '{assemblyPath}'.");
        }

        // Load assembly into an isolated, reflection-only load context
        var absolutePath = Path.GetFullPath(assemblyPath);
        var assembly = Assembly.LoadFrom(absolutePath);

        var asmName = assembly.GetName();
        var targetFramework = assembly.GetCustomAttribute<TargetFrameworkAttribute>()?.FrameworkDisplayName 
                              ?? assembly.GetCustomAttribute<TargetFrameworkAttribute>()?.FrameworkName 
                              ?? "Unknown / Unspecified";

        // Extract external dependencies
        var dependencies = assembly.GetReferencedAssemblies()
            .Select(r => $"{r.Name} (v{r.Version})")
            .OrderBy(d => d)
            .ToList();

        // Extract types safely (handling ReflectionTypeLoadException for missing references)
        Type[] types;
        try
        {
            types = assembly.GetTypes();
        }
        catch (ReflectionTypeLoadException ex)
        {
            types = ex.Types.Where(t => t != null).ToArray()!;
        }

        var filterFlags = includeNonPublic
            ? BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly
            : BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly;

        var typeReports = new List<TypeInfoReport>();

        foreach (var type in types.OrderBy(t => t.FullName))
        {
            if (!includeNonPublic && !type.IsPublic && !type.IsNestedPublic)
                continue;

            var kind = GetTypeKind(type);
            var baseType = type.BaseType != null && type.BaseType != typeof(object) && type.BaseType != typeof(ValueType)
                ? type.BaseType.Name
                : string.Empty;

            var interfaces = type.GetInterfaces()
                .Select(i => i.Name)
                .OrderBy(i => i)
                .ToList();

            var constructors = type.GetConstructors(filterFlags)
                .Select(FormatConstructor)
                .ToList();

            var methods = type.GetMethods(filterFlags)
                .Where(m => !m.IsSpecialName) // Exclude getter/setter methods
                .Select(FormatMethod)
                .OrderBy(m => m)
                .ToList();

            var properties = type.GetProperties(filterFlags)
                .Select(FormatProperty)
                .OrderBy(p => p)
                .ToList();

            var fields = type.GetFields(filterFlags)
                .Where(f => !f.IsSpecialName)
                .Select(FormatField)
                .OrderBy(f => f)
                .ToList();

            typeReports.Add(new TypeInfoReport(
                type.Name,
                kind,
                baseType,
                interfaces,
                constructors,
                methods,
                properties,
                fields
            ));
        }

        var namespaces = typeReports
            .GroupBy(t => string.IsNullOrEmpty(types.First(orig => orig.Name == t.Name).Namespace) ? "<Global>" : types.First(orig => orig.Name == t.Name).Namespace!)
            .Select(g => new NamespaceInfo(g.Key, g.ToList()))
            .OrderBy(n => n.Name)
            .ToList();

        return new AssemblyReport(
            asmName.Name ?? Path.GetFileNameWithoutExtension(assemblyPath),
            asmName.Version?.ToString() ?? "0.0.0.0",
            targetFramework,
            dependencies,
            namespaces
        );
    }

    private static string GetTypeKind(Type t)
    {
        if (t.IsInterface) return "interface";
        if (t.IsEnum) return "enum";
        if (t.IsValueType) return "struct";
        if (t.GetMethod("<Clone>$") != null) return "record";
        if (t.IsAbstract && t.IsSealed) return "static class";
        if (t.IsAbstract) return "abstract class";
        return "class";
    }

    private static string FormatConstructor(ConstructorInfo c)
    {
        var paramsStr = string.Join(", ", c.GetParameters().Select(p => $"{FormatTypeName(p.ParameterType)} {p.Name}"));
        return $"{c.DeclaringType?.Name}({paramsStr})";
    }

    private static string FormatMethod(MethodInfo m)
    {
        var paramsStr = string.Join(", ", m.GetParameters().Select(p => $"{FormatTypeName(p.ParameterType)} {p.Name}"));
        var staticPrefix = m.IsStatic ? "static " : "";
        return $"{staticPrefix}{FormatTypeName(m.ReturnType)} {m.Name}({paramsStr})";
    }

    private static string FormatProperty(PropertyInfo p)
    {
        var accessors = new List<string>();
        if (p.GetMethod != null) accessors.Add("get;");
        if (p.SetMethod != null) accessors.Add("set;");
        return $"{FormatTypeName(p.PropertyType)} {p.Name} {{ {string.Join(" ", accessors)} }}";
    }

    private static string FormatField(FieldInfo f)
    {
        var constPrefix = f.IsLiteral ? "const " : (f.IsStatic ? "static " : "");
        return $"{constPrefix}{FormatTypeName(f.FieldType)} {f.Name}";
    }

    private static string FormatTypeName(Type type)
    {
        if (type == typeof(void)) return "void";
        if (type == typeof(int)) return "int";
        if (type == typeof(long)) return "long";
        if (type == typeof(bool)) return "bool";
        if (type == typeof(string)) return "string";
        if (type == typeof(double)) return "double";
        if (type == typeof(float)) return "float";

        if (type.IsGenericType)
        {
            var genericName = type.Name.Split('`')[0];
            var genericArgs = string.Join(", ", type.GetGenericArguments().Select(FormatTypeName));
            return $"{genericName}<{genericArgs}>";
        }

        return type.Name;
    }
}
