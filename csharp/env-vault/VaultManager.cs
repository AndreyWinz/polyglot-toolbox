using System.Text.Json;

namespace EnvVault;

public static class VaultManager
{
    private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };

    public static void SaveVault(string path, VaultEnvelope envelope)
    {
        var json = JsonSerializer.Serialize(envelope, JsonOptions);
        File.WriteAllText(path, json);
    }

    public static VaultEnvelope LoadVault(string path)
    {
        if (!File.Exists(path))
            throw new FileNotFoundException($"Vault file '{path}' not found.");

        var json = File.ReadAllText(path);
        var envelope = JsonSerializer.Deserialize<VaultEnvelope>(json);
        
        return envelope ?? throw new InvalidDataException($"Failed to parse vault file '{path}'.");
    }

    public static Dictionary<string, string> ParseEnvContent(string envContent)
    {
        var dict = new Dictionary<string, string>(StringComparer.Ordinal);
        using var reader = new StringReader(envContent);

        string? line;
        while ((line = reader.ReadLine()) != null)
        {
            var trimmed = line.Trim();
            if (string.IsNullOrWhiteSpace(trimmed) || trimmed.StartsWith('#'))
                continue;

            var equalsIdx = trimmed.IndexOf('=');
            if (equalsIdx <= 0) continue;

            var key = trimmed[..equalsIdx].Trim();
            var val = trimmed[(equalsIdx + 1)..].Trim();

            if ((val.StartsWith('"') && val.EndsWith('"')) || (val.StartsWith('\'') && val.EndsWith('\'')))
            {
                val = val[1..^1];
            }

            dict[key] = val;
        }

        return dict;
    }

    public static string SerializeEnv(Dictionary<string, string> envDict)
    {
        var lines = envDict.Select(kv => $"{kv.Key}={FormatValue(kv.Value)}");
        return string.Join(Environment.NewLine, lines);
    }

    private static string FormatValue(string val)
    {
        if (val.Contains(' ') || val.Contains('#') || val.Contains('"'))
        {
            return $"\"{val.Replace("\"", "\\\"")}\"";
        }
        return val;
    }
}
