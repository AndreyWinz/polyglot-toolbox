using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace EnvVault;

public record VaultEnvelope(
    int Version,
    string Kdf,
    int Iterations,
    string Salt,
    string Nonce,
    string Tag,
    string Ciphertext
);

public static class CryptoService
{
    private const int SaltSizeBytes = 16;
    private const int NonceSizeBytes = 12; // 96 bits for AES-GCM
    private const int TagSizeBytes = 16;   // 128 bits auth tag
    private const int KeySizeBytes = 32;   // 256 bits for AES-256
    private const int DefaultIterations = 600_000;

    public static VaultEnvelope Encrypt(string plaintext, string password)
    {
        byte[] salt = RandomNumberGenerator.GetBytes(SaltSizeBytes);
        byte[] nonce = RandomNumberGenerator.GetBytes(NonceSizeBytes);
        byte[] key = DeriveKey(password, salt, DefaultIterations);

        byte[] plaintextBytes = Encoding.UTF8.GetBytes(plaintext);
        byte[] ciphertext = new byte[plaintextBytes.Length];
        byte[] tag = new byte[TagSizeBytes];

        using (var aesGcm = new AesGcm(key, TagSizeBytes))
        {
            aesGcm.Encrypt(nonce, plaintextBytes, ciphertext, tag);
        }

        CryptographicOperations.ZeroMemory(key);

        return new VaultEnvelope(
            Version: 1,
            Kdf: "PBKDF2-HMAC-SHA256",
            Iterations: DefaultIterations,
            Salt: Convert.ToBase64String(salt),
            Nonce: Convert.ToBase64String(nonce),
            Tag: Convert.ToBase64String(tag),
            Ciphertext: Convert.ToBase64String(ciphertext)
        );
    }

    public static string Decrypt(VaultEnvelope envelope, string password)
    {
        byte[] salt = Convert.FromBase64String(envelope.Salt);
        byte[] nonce = Convert.FromBase64String(envelope.Nonce);
        byte[] tag = Convert.FromBase64String(envelope.Tag);
        byte[] ciphertext = Convert.FromBase64String(envelope.Ciphertext);

        byte[] key = DeriveKey(password, salt, envelope.Iterations);
        byte[] decryptedBytes = new byte[ciphertext.Length];

        try
        {
            using (var aesGcm = new AesGcm(key, TagSizeBytes))
            {
                aesGcm.Decrypt(nonce, ciphertext, tag, decryptedBytes);
            }
            return Encoding.UTF8.GetString(decryptedBytes);
        }
        catch (CryptographicException)
        {
            throw new InvalidOperationException("Decryption failed. Invalid password or corrupted payload (tag mismatch).");
        }
        finally
        {
            CryptographicOperations.ZeroMemory(key);
            CryptographicOperations.ZeroMemory(decryptedBytes);
        }
    }

    private static byte[] DeriveKey(string password, byte[] salt, int iterations)
    {
        return Rfc2898DeriveBytes.Pbkdf2(
            password,
            salt,
            iterations,
            HashAlgorithmName.SHA256,
            KeySizeBytes
        );
    }
}
