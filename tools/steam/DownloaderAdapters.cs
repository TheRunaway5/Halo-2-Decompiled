// The downloader integration is distributed with DepotDownloader's GPL-2.0
// license and corresponding sources in the private helper SDK.
using System.Collections.Concurrent;

namespace DepotDownloader;

// Authentication is ephemeral. In particular, upstream's calls to Save()
// must never put refresh tokens or Steam Guard data in isolated storage.
internal sealed class AccountSettingsStore
{
    public static readonly AccountSettingsStore Instance = new();
    public ConcurrentDictionary<string, int> ContentServerPenalty { get; } = new();
    public Dictionary<string, string> LoginTokens { get; } = new(StringComparer.OrdinalIgnoreCase);
    public Dictionary<string, string> GuardData { get; } = new(StringComparer.OrdinalIgnoreCase);
    public static void Save() { }
}

internal static class Ansi
{
    public enum ProgressState { Hidden, Default, Error, Indeterminate, Warning }
    public static void Progress(ProgressState state, byte progress = 0) { }
    public static void Progress(ulong downloaded, ulong total) =>
        Halo2.SteamSetup.ContentProtocol.Progress(downloaded, total);
}

internal static class PlatformUtilities
{
    public static void SetExecutable(string path, bool value)
    {
        if (OperatingSystem.IsWindows()) return;
        const UnixFileMode execute = UnixFileMode.UserExecute | UnixFileMode.GroupExecute | UnixFileMode.OtherExecute;
        var mode = File.GetUnixFileMode(path);
        File.SetUnixFileMode(path, value ? mode | execute : mode & ~execute);
    }
}

partial class Steam3Session
{
    // The GUI owner supplies cancellation; do not drain a callback loop from
    // another callback or wait indefinitely for a disconnected notification.
    internal void CancelForSetup()
    {
        bAborted = true;
        bConnecting = false;
        bIsConnectionRecovery = false;
        abortedToken.Cancel();
        steamClient.Disconnect();
    }
    internal bool SetupAborted => bAborted;
}
