using System.Diagnostics;
using EnvVault;

if (args.Length == 0 || args[0] is "-h" or "--help")
{
    PrintUsage();
    return 0;
}

string command = args[0].ToLowerInvariant();

try
{
    switch (command)
    {
        case "encrypt":
            HandleEncrypt(args);
            break;
        case "decrypt":
            HandleDecrypt(args);
            break;
        case "get":
            HandleGet(args);
            break;
        case "set":
            HandleSet(args);
            break;
        case "list":
            HandleList(args);
            break;
        case "run":
            HandleRun(args);
            break;
        default:
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Unknown command '{command}'.");
            Console.ResetColor();
            PrintUsage();
            return 1;
    }
    return 0;
}
catch (Exception ex)
{
    Console.ForegroundColor = ConsoleColor.Red;
    Console.WriteLine($"\nError: {ex.Message}");
    Console.ResetColor();
    return 1;
}

void HandleEncrypt(string[] args)
{
    if (args.Length < 3)
    {
        Console.WriteLine("Usage: env-vault encrypt <input_.env> <output_.vault> [-p password]");
        return;
    }

    string inputFile = args[1];
    string outputFile = args[2];
    string password = GetPasswordFromArgs(args) ?? PromptPassword("Enter encryption password: ");

    if (!File.Exists(inputFile))
        throw new FileNotFoundException($"Input file '{inputFile}' not found.");

    string plaintext = File.ReadAllText(inputFile);
    var envelope = CryptoService.Encrypt(plaintext, password);
    VaultManager.SaveVault(outputFile, envelope);

    Console.ForegroundColor = ConsoleColor.Green;
    Console.WriteLine($"\u2705 Successfully encrypted '{inputFile}' -> '{outputFile}' (AES-256-GCM + PBKDF2).");
    Console.ResetColor();
}

void HandleDecrypt(string[] args)
{
    if (args.Length < 3)
    {
        Console.WriteLine("Usage: env-vault decrypt <input_.vault> <output_.env> [-p password]");
        return;
    }

    string vaultFile = args[1];
    string outputFile = args[2];
    string password = GetPasswordFromArgs(args) ?? PromptPassword("Enter decryption password: ");

    var envelope = VaultManager.LoadVault(vaultFile);
    string plaintext = CryptoService.Decrypt(envelope, password);
    File.WriteAllText(outputFile, plaintext);

    Console.ForegroundColor = ConsoleColor.Green;
    Console.WriteLine($"\u2705 Successfully decrypted '{vaultFile}' -> '{outputFile}'.");
    Console.ResetColor();
}

void HandleGet(string[] args)
{
    if (args.Length < 3)
    {
        Console.WriteLine("Usage: env-vault get <vault_file> <KEY> [-p password]");
        return;
    }

    string vaultFile = args[1];
    string key = args[2];
    string password = GetPasswordFromArgs(args) ?? PromptPassword("Enter vault password: ");

    var envelope = VaultManager.LoadVault(vaultFile);
    string plaintext = CryptoService.Decrypt(envelope, password);
    var envDict = VaultManager.ParseEnvContent(plaintext);

    if (envDict.TryGetValue(key, out var val))
    {
        Console.WriteLine(val);
    }
    else
    {
        Console.ForegroundColor = ConsoleColor.Yellow;
        Console.WriteLine($"Key '{key}' not found in vault.");
        Console.ResetColor();
    }
}

void HandleSet(string[] args)
{
    if (args.Length < 4)
    {
        Console.WriteLine("Usage: env-vault set <vault_file> <KEY> <VALUE> [-p password]");
        return;
    }

    string vaultFile = args[1];
    string key = args[2];
    string value = args[3];
    string password = GetPasswordFromArgs(args) ?? PromptPassword("Enter vault password: ");

    Dictionary<string, string> envDict = new();
    if (File.Exists(vaultFile))
    {
        var envelope = VaultManager.LoadVault(vaultFile);
        string plaintext = CryptoService.Decrypt(envelope, password);
        envDict = VaultManager.ParseEnvContent(plaintext);
    }

    envDict[key] = value;
    string updatedEnv = VaultManager.SerializeEnv(envDict);
    var updatedEnvelope = CryptoService.Encrypt(updatedEnv, password);
    VaultManager.SaveVault(vaultFile, updatedEnvelope);

    Console.ForegroundColor = ConsoleColor.Green;
    Console.WriteLine($"\u2705 Updated key '{key}' in vault '{vaultFile}'.");
    Console.ResetColor();
}

void HandleList(string[] args)
{
    if (args.Length < 2)
    {
        Console.WriteLine("Usage: env-vault list <vault_file> [-p password]");
        return;
    }

    string vaultFile = args[1];
    string password = GetPasswordFromArgs(args) ?? PromptPassword("Enter vault password: ");

    var envelope = VaultManager.LoadVault(vaultFile);
    string plaintext = CryptoService.Decrypt(envelope, password);
    var envDict = VaultManager.ParseEnvContent(plaintext);

    Console.ForegroundColor = ConsoleColor.Cyan;
    Console.WriteLine($"\n\U0001f511 Secret Keys in Vault '{vaultFile}':");
    Console.ResetColor();

    foreach (var kv in envDict)
    {
        Console.WriteLine($"   \u251c\u2500 {kv.Key} = **********");
    }
}

void HandleRun(string[] args)
{
    int dashIndex = Array.IndexOf(args, "--");
    if (dashIndex == -1 || dashIndex < 2 || dashIndex >= args.Length - 1)
    {
        Console.WriteLine("Usage: env-vault run <vault_file> [-p password] -- <command> [args...]");
        return;
    }

    string vaultFile = args[1];
    string password = GetPasswordFromArgs(args) ?? PromptPassword("Enter vault password: ");

    var envelope = VaultManager.LoadVault(vaultFile);
    string plaintext = CryptoService.Decrypt(envelope, password);
    var envDict = VaultManager.ParseEnvContent(plaintext);

    string execName = args[dashIndex + 1];
    var execArgs = args[(dashIndex + 2)..];

    var psi = new ProcessStartInfo(execName)
    {
        UseShellExecute = false
    };

    foreach (var (k, v) in envDict)
    {
        psi.EnvironmentVariables[k] = v;
    }

    foreach (var a in execArgs)
    {
        psi.ArgumentList.Add(a);
    }

    using var child = Process.Start(psi);
    child?.WaitForExit();
}

string? GetPasswordFromArgs(string[] args)
{
    int idx = Array.IndexOf(args, "-p");
    if (idx == -1) idx = Array.IndexOf(args, "--password");
    if (idx != -1 && idx + 1 < args.Length)
    {
        return args[idx + 1];
    }

    return Environment.GetEnvironmentVariable("ENV_VAULT_PASSWORD");
}

string PromptPassword(string prompt)
{
    Console.Write(prompt);
    var pass = new System.Text.StringBuilder();
    while (true)
    {
        var key = Console.ReadKey(intercept: true);
        if (key.Key == ConsoleKey.Enter)
        {
            Console.WriteLine();
            break;
        }
        if (key.Key == ConsoleKey.Backspace && pass.Length > 0)
        {
            pass.Remove(pass.Length - 1, 1);
        }
        else if (!char.IsControl(key.KeyChar))
        {
            pass.Append(key.KeyChar);
        }
    }
    return pass.ToString();
}

void PrintUsage()
{
    Console.WriteLine("""
    \u001b[1;36menv-vault\u001b[0m - AES-256-GCM Encrypted .env Secrets Manager

    Usage:
      env-vault <command> [options]

    Commands:
      encrypt <input_.env> <vault_file>     Encrypt a plaintext .env file into a vault
      decrypt <vault_file> <output_.env>    Decrypt a vault back into a plaintext .env file
      get     <vault_file> <KEY>            Retrieve a single secret value
      set     <vault_file> <KEY> <VALUE>    Add or update a secret inside the vault
      list    <vault_file>                  List all key names stored in the vault
      run     <vault_file> -- <cmd> [args]  Execute a process with secrets injected into ENV

    Options:
      -p, --password <pass>                 Provide password inline (or set ENV_VAULT_PASSWORD)
      -h, --help                            Show help options

    Examples:
      env-vault encrypt .env .env.vault
      env-vault set .env.vault API_KEY "sk-live-9921"
      env-vault run .env.vault -- python main.py
    """);
}
