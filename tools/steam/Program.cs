// Steam content metadata transport. Account/content authentication is a
// separate operation from the multiplayer service's Steam OpenID identity.
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;
using SteamKit2;

namespace Halo2.SteamSetup;

internal static class Program
{
    private const uint Mcc = 976730;
    private const uint Halo2 = 1064270;

    private static object Node(KeyValue node) => node.Children.Count > 0
        ? node.Children.ToDictionary(child => child.Name ?? throw new InvalidDataException("Unnamed Steam metadata node"), Node)
        : node.Value ?? "";

    private static async Task<object> AppInfo(CancellationToken cancellation)
    {
        var client = new SteamClient();
        var user = client.GetHandler<SteamUser>() ?? throw new InvalidOperationException();
        var apps = client.GetHandler<SteamApps>() ?? throw new InvalidOperationException();
        var callbacks = new CallbackManager(client);
        var loggedOn = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
        using var connected = callbacks.Subscribe<SteamClient.ConnectedCallback>(_ => user.LogOnAnonymous());
        using var logged = callbacks.Subscribe<SteamUser.LoggedOnCallback>(value =>
        {
            if (value.Result == EResult.OK) loggedOn.TrySetResult(true);
            else loggedOn.TrySetException(new IOException("Steam did not accept the metadata session"));
        });
        using var disconnected = callbacks.Subscribe<SteamClient.DisconnectedCallback>(_ =>
            loggedOn.TrySetException(new IOException("Steam metadata connection closed")));
        using var owner = CancellationTokenSource.CreateLinkedTokenSource(cancellation);
        var worker = Task.Run(async () =>
        {
            try
            {
                while (!owner.IsCancellationRequested) await callbacks.RunWaitCallbackAsync(owner.Token);
            }
            catch (OperationCanceledException) when (owner.IsCancellationRequested) { }
        }, CancellationToken.None);
        client.Connect();
        try
        {
            await loggedOn.Task.WaitAsync(cancellation);
            var result = new Dictionary<string, object>();
            foreach (var id in new[] { Mcc, Halo2 })
            {
                var tokens = await apps.PICSGetAccessTokens([id], []).ToTask().WaitAsync(cancellation);
                var request = new SteamApps.PICSRequest(id);
                if (tokens.AppTokens.TryGetValue(id, out var token)) request.AccessToken = token;
                var infos = await apps.PICSGetProductInfo([request], []).ToTask().WaitAsync(cancellation);
                foreach (var part in infos.Results ?? throw new IOException("Steam returned no metadata results"))
                {
                    if (!part.Apps.TryGetValue(id, out var info) || info is null) continue;
                    result.Add(id.ToString(), new
                    {
                        change_number = info.ChangeNumber,
                        common = Node(info.KeyValues["common"]),
                        depots = Node(info.KeyValues["depots"])
                    });
                }
                if (!result.ContainsKey(id.ToString())) throw new IOException("Steam did not return the required application metadata");
            }
            return new { schema = "H2_STEAM_APPINFO_V1", apps = result, anonymous = true };
        }
        finally
        {
            client.Disconnect();
            owner.Cancel();
            await worker;
        }
    }

    [DllImport("/usr/lib/libSystem.B.dylib", EntryPoint = "_dyld_image_count")]
    private static extern uint ImageCount();
    [DllImport("/usr/lib/libSystem.B.dylib", EntryPoint = "_dyld_get_image_name")]
    private static extern IntPtr ImageName(uint index);

    private static object RuntimeInfo()
    {
        var images = new List<string>();
        if (OperatingSystem.IsMacOS())
            for (uint i = 0, count = ImageCount(); i < count; i++)
                images.Add(Marshal.PtrToStringUTF8(ImageName(i)) ?? throw new InvalidDataException("Missing loaded image path"));
        return new
        {
            schema = "H2_STEAM_RUNTIME_V1", architecture = RuntimeInformation.ProcessArchitecture.ToString(),
            framework = RuntimeInformation.FrameworkDescription,
            root = AppContext.BaseDirectory, core_library = typeof(object).Assembly.Location,
            steamkit = typeof(SteamClient).Assembly.GetName().Version?.ToString(),
            sha256_abc = Convert.ToHexString(SHA256.HashData("abc"u8)), images
        };
    }

    public static async Task<int> Main(string[] arguments)
    {
        if (arguments is ["session"]) return await ContentProtocol.Run();
        try
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(65));
            var value = arguments switch
            {
                ["app-info"] => await AppInfo(timeout.Token),
                ["runtime-info"] => RuntimeInfo(),
                _ => throw new ArgumentException("Use app-info or runtime-info")
            };
            Console.WriteLine(JsonSerializer.Serialize(value));
            return 0;
        }
        catch (OperationCanceledException)
        {
            Console.Error.WriteLine("Steam metadata request timed out or was cancelled.");
            return 2;
        }
        catch (Exception error)
        {
            // No arbitrary remote exception text, account identifier or session
            // material is emitted by the subprocess protocol.
            Console.Error.WriteLine("Steam metadata request failed (" + error.GetType().Name + ").");
            return 1;
        }
    }
}
