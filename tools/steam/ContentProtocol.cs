using System.Text;
using System.Text.Json;
using System.Threading.Channels;
using QRCoder;

namespace Halo2.SteamSetup;

// The only writer to stdout during an authenticated session. Upstream console
// messages can contain account names; they never reach the UI or a log file.
internal static class ContentProtocol
{
    private static readonly object OutputLock = new();
    private static StreamWriter? output;
    private static long lastProgress;
    private static bool active;
    private static ulong reusedBytes;
    private static int reusedFiles;
    internal static ulong ReusedBytes { get { lock (OutputLock) return reusedBytes; } }
    private static readonly Channel<JsonElement> Commands = Channel.CreateBounded<JsonElement>(
        new BoundedChannelOptions(4) { FullMode = BoundedChannelFullMode.Wait });

    internal static void Emit(object value)
    {
        lock (OutputLock)
        {
            if (!active) return;
            output!.WriteLine(JsonSerializer.Serialize(value));
            output.Flush();
        }
    }

    internal static void Qr(string challenge)
    {
        if (!Uri.TryCreate(challenge, UriKind.Absolute, out var uri) || uri.Scheme != "https" ||
            uri.Host != "s.team" || !uri.AbsolutePath.StartsWith("/q/", StringComparison.Ordinal))
            throw new InvalidDataException("Unexpected Steam QR challenge");
        using var generator = new QRCodeGenerator();
        using var data = generator.CreateQrCode(challenge, QRCodeGenerator.ECCLevel.M);
        using var qr = new PngByteQRCode(data);
        Emit(new { @event = "qr", png = Convert.ToBase64String(qr.GetGraphic(6)) });
    }

    internal static void Progress(ulong bytes, ulong total)
    {
        var now = Environment.TickCount64;
        if (now - Interlocked.Read(ref lastProgress) < 250 && bytes != total) return;
        Interlocked.Exchange(ref lastProgress, now);
        Emit(new { @event = "progress", bytes, total });
    }

    internal static void Reused(ulong bytes)
    {
        if (bytes == 0) return;
        lock (OutputLock)
        {
            reusedBytes = checked(reusedBytes + bytes);
            reusedFiles++;
            Emit(new { @event = "resume", bytes = reusedBytes, files = reusedFiles });
        }
    }

    internal static async Task<JsonElement> Next(string expected, CancellationToken token)
    {
        var command = await Commands.Reader.ReadAsync(token);
        if (command.GetProperty("command").GetString() != expected)
            throw new InvalidDataException("Unexpected setup command");
        return command;
    }

    private static async Task ReadCommands(StreamReader input, CancellationTokenSource owner)
    {
        try
        {
            // Small local commands only. Never buffer an unbounded stdin line.
            var line = new StringBuilder();
            var character = new char[1];
            while (await input.ReadAsync(character.AsMemory(), owner.Token) != 0)
            {
                if (character[0] != '\n')
                {
                    if (line.Length >= 8192) throw new InvalidDataException("Oversized setup command");
                    line.Append(character[0]);
                    continue;
                }
                using var parsed = JsonDocument.Parse(line.ToString());
                line.Clear();
                var value = parsed.RootElement.Clone();
                if (value.GetProperty("command").GetString() == "cancel") break;
                if (!Commands.Writer.TryWrite(value)) throw new InvalidDataException("Too many setup commands");
            }
        }
        catch (Exception error) when (error is IOException or JsonException or InvalidDataException or
                                      OperationCanceledException or InvalidOperationException or KeyNotFoundException) { }
        finally { owner.Cancel(); Commands.Writer.TryComplete(); }
    }

    internal static async Task<int> Run()
    {
        using var owner = new CancellationTokenSource();
        using var input = new StreamReader(Console.OpenStandardInput(), new UTF8Encoding(false, true));
        output = new StreamWriter(Console.OpenStandardOutput(), new UTF8Encoding(false));
        Console.SetOut(TextWriter.Null);
        Console.SetError(TextWriter.Null);
        Console.SetIn(TextReader.Null);
        active = true;
        var reader = Task.Run(() => ReadCommands(input, owner));
        try
        {
            Emit(new { @event = "ready", schema = "H2_STEAM_CONTENT_SESSION_V1" });
            var start = await Next("start", owner.Token).WaitAsync(TimeSpan.FromSeconds(30), owner.Token);
            var root = start.GetProperty("data_root").GetString() ?? throw new InvalidDataException("Missing data root");
            await DepotDownloader.ContentDownloader.RunForSetup(root, owner);
            return 0;
        }
        catch (Exception error)
        {
            var code = error switch
            {
                ContentPolicyException => "invalid_content_or_command",
                OperationCanceledException => "cancelled",
                TimeoutException => "timeout",
                UnauthorizedAccessException => "ownership_or_permission",
                InvalidDataException or JsonException or KeyNotFoundException => "invalid_content_or_command",
                IOException => "storage_or_connection",
                _ => "steam_session_failed"
            };
            if (error is ContentPolicyException policy)
                Emit(new { @event = "error", code, reason = policy.Issue.ToString(), details = policy.Details });
            else Emit(new { @event = "error", code });
            return code == "cancelled" ? 2 : 1;
        }
        finally
        {
            owner.Cancel();
            DepotDownloader.ContentDownloader.CancelForSetup();
            // The subprocess owner also imposes a kill/reap deadline. Never
            // block shutdown on an upstream callback drain or a blocked stdin.
            try { await reader.WaitAsync(TimeSpan.FromMilliseconds(500)); }
            catch (TimeoutException) { }
            lock (OutputLock) { active = false; output.Dispose(); }
        }
    }
}
