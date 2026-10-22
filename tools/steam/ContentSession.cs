// GUI integration for DepotDownloader. Distributed under GPL-2.0 with the
// complete pinned and adapted sources in corresponding-source/ in the SDK.
using System.Security.Cryptography;
using System.Text.Json;
using Halo2.SteamSetup;
using SteamKit2;

namespace DepotDownloader;

static partial class ContentDownloader
{
    private static readonly List<ContentFile> SetupFiles = [];
    private static List<DepotSelection> setupDepots = [];

    internal static void CancelForSetup() => steam3?.CancelForSetup();

    // This hook runs before upstream creates any per-file directories. Cached
    // manifests receive the same validation as freshly authorized manifests.
    private static void SetupManifest(uint depot, ulong manifestId, DepotManifest manifest)
    {
        if (!setupDepots.Any(d => d.Id == depot && d.Manifest == manifestId) || manifest.DepotID != depot ||
            manifest.ManifestGID != manifestId) throw new ContentPolicyException(ContentIssue.UnexpectedManifest,
                new { depot_id = depot, reported_depot_id = manifest.DepotID,
                    manifest_id = manifestId.ToString(), reported_manifest_id = manifest.ManifestGID.ToString() });
        var selected = ContentRules.SelectFiles(depot, manifestId, manifest);
        SetupFiles.AddRange(selected);
        foreach (var file in selected) Config.FilesToDownload.Add(file.path);
    }

    private static async Task<bool> Licensed(uint app, CancellationToken token)
    {
        var licenses = steam3.Licenses.Select(v => v.PackageID).Distinct().ToArray();
        await steam3.RequestPackageInfo(licenses).WaitAsync(token);
        return licenses.Any(id => steam3.PackageInfo.TryGetValue(id, out var package) && package is not null &&
            package.KeyValues["appids"].Children.Any(v => v.AsUnsignedInteger() == app));
    }

    private static void CheckTree(string path)
    {
        ContentRules.NoLinks(path);
        if (!Directory.Exists(path)) return;
        // Do not traverse a reparse point before rejecting it.
        foreach (var child in Directory.EnumerateFileSystemEntries(path))
        {
            var attributes = File.GetAttributes(child);
            if ((attributes & FileAttributes.ReparsePoint) != 0) throw new IOException("Linked download entry");
            if ((attributes & FileAttributes.Directory) != 0) CheckTree(child);
        }
    }

    private static void CheckDisk(string root, ulong required)
    {
        var available = new DriveInfo(root).AvailableFreeSpace;
        if (available < 0 || (ulong)available < required) throw new IOException("Insufficient setup space");
    }

    internal static async Task RunForSetup(string dataRoot, CancellationTokenSource owner)
    {
        if (!Path.IsPathFullyQualified(dataRoot) || dataRoot.Length > 4096)
            throw new InvalidDataException("Expected absolute application data root");
        dataRoot = Path.GetFullPath(dataRoot);
        ContentRules.NoLinks(dataRoot);
        Directory.CreateDirectory(dataRoot);
        Config = new DownloadConfig
        {
            UseQrCode = true, RememberPassword = false, VerifyAll = true, UsingFileList = true,
            FilesToDownload = new(StringComparer.OrdinalIgnoreCase), FilesToDownloadRegex = [],
            MaxDownloads = 4, LoginID = BitConverter.ToUInt32(RandomNumberGenerator.GetBytes(4))
        };
        using var cancel = owner.Token.Register(CancelForSetup);
        // BeginAuthSessionViaQRAsync is asynchronous inside the callback pump.
        // Give the user time to approve in Steam Mobile, with a finite deadline.
        ContentProtocol.Emit(new { @event = "authenticating" });
        if (!await Task.Run(() => InitializeSteam3(null!, null!)).WaitAsync(TimeSpan.FromMinutes(5), owner.Token))
            throw new UnauthorizedAccessException("Steam login was not completed");
        using var planning = CancellationTokenSource.CreateLinkedTokenSource(owner.Token);
        planning.CancelAfter(TimeSpan.FromMinutes(3));
        var token = planning.Token;
        while (steam3.Licenses is null)
        {
            if (steam3.SetupAborted) throw new IOException("Steam disconnected");
            await Task.Delay(50, token);
        }
        if (steam3.steamUser.SteamID?.AccountType != EAccountType.Individual ||
            !await Licensed(ContentRules.App, token) || !await Licensed(ContentRules.Dlc, token) ||
            !await Licensed(ContentRules.MultiplayerDlc, token))
            throw new UnauthorizedAccessException("MCC and Halo 2 ownership required");
        await steam3.RequestAppInfo(ContentRules.App, true).WaitAsync(token);
        setupDepots = ContentRules.SelectDepots(GetSteam3AppSection(ContentRules.App, EAppInfoSection.Depots)
            ?? throw new InvalidDataException("Missing MCC depots"));
        ContentProtocol.Emit(new { @event = "ownership_verified", app_id = ContentRules.App, dlc_id = ContentRules.Dlc,
            multiplayer_dlc_id = ContentRules.MultiplayerDlc });

        var sourceId = ContentRules.SourceId(setupDepots);
        var owned = Path.Combine(dataRoot, "downloads", "steam-mcc", sourceId);
        CheckTree(owned);
        Directory.CreateDirectory(owned);
        using var exclusive = new FileStream(Path.Combine(owned, ".session.lock"),
            FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        var marker = Path.Combine(owned, "owner.json");
        if (File.Exists(marker))
        {
            using var prior = JsonDocument.Parse(File.ReadAllText(marker));
            if (prior.RootElement.GetProperty("schema").GetString() != "H2_STEAM_SOURCE_V1" ||
                prior.RootElement.GetProperty("source_id").GetString() != sourceId)
                throw new IOException("Unrecognized download directory");
        }
        else
        {
            if (Directory.EnumerateFileSystemEntries(owned).Any(p => Path.GetFileName(p) != ".session.lock"))
                throw new IOException("Unowned download directory");
            using var created = new FileStream(marker, FileMode.CreateNew, FileAccess.Write, FileShare.None);
            JsonSerializer.Serialize(created, new { schema = "H2_STEAM_SOURCE_V1", source_id = sourceId,
                depots = setupDepots.Select(d => new { depot_id = d.Id, manifest_id = d.Manifest.ToString() }) });
        }
        Config.InstallDirectory = Path.Combine(owned, "content");
        Directory.CreateDirectory(Path.Combine(Config.InstallDirectory, CONFIG_DIR));
        DepotConfigStore.LoadFromFile(Path.Combine(Config.InstallDirectory, CONFIG_DIR, "depot.config"));
        cdnPool = new CDNClientPool(steam3, ContentRules.App);
        await cdnPool.UpdateServerList().WaitAsync(token);
        var counter = new GlobalDownloadCounter();
        var prepared = new List<DepotFilesData>();
        foreach (var depot in setupDepots)
        {
            var info = await GetDepotInfo(depot.Id, ContentRules.App, depot.Manifest, "public").WaitAsync(token)
                ?? throw new UnauthorizedAccessException("Depot access denied");
            var files = await ProcessDepotManifestAndFiles(planning, info, counter).WaitAsync(token)
                ?? throw new InvalidDataException("Manifest planning failed");
            prepared.Add(files);
        }
        var composed = ContentRules.ComposeFiles(SetupFiles);
        ContentRules.ValidateComplete(composed);
        SetupFiles.Clear();
        SetupFiles.AddRange(composed);
        var selectedOwners = composed.ToDictionary(f => f.path, f => f.depot_id, StringComparer.OrdinalIgnoreCase);
        foreach (var depot in prepared)
        {
            depot.filteredFiles.RemoveAll(f => !selectedOwners.TryGetValue(f.FileName, out var selectedDepot) ||
                selectedDepot != depot.depotDownloadInfo.DepotId);
            depot.allFileNames = depot.filteredFiles.Select(f => f.FileName).ToHashSet();
            depot.depotCounter.completeDownloadSize = depot.filteredFiles.Aggregate(0UL, (sum, f) => checked(sum + f.TotalSize));
        }
        counter.completeDownloadSize = composed.Aggregate(0UL, (sum, f) => checked(sum + f.bytes));
        var total = SetupFiles.Aggregate(0UL, (sum, file) => checked(sum + file.bytes));
        var compressed = SetupFiles.Aggregate(0UL, (sum, file) => checked(sum + file.download_bytes));
        // Source + prepared copy + possible staging copy, with room for UI
        // extraction and filesystem bookkeeping. The importer also preflights
        // its actual preparation budget before it starts.
        var reserve = checked(total * 3 + 2UL * 1024 * 1024 * 1024);
        var available = new DriveInfo(owned).AvailableFreeSpace;
        if (available < 0) throw new IOException("Cannot inspect setup space");
        CheckTree(owned);
        var inventory = JsonSerializer.SerializeToUtf8Bytes(SetupFiles.OrderBy(v => v.path, StringComparer.Ordinal));
        var inventoryHash = Convert.ToHexString(SHA256.HashData(inventory)).ToLowerInvariant();
        File.WriteAllBytes(Path.Combine(owned, "selected-files.json"), inventory);
        var confirmation = Convert.ToHexString(RandomNumberGenerator.GetBytes(16));
        ContentProtocol.Emit(new { @event = "plan", source_id = sourceId, token = confirmation,
            files = SetupFiles.Count, bytes = total, download_bytes = compressed,
            required_free_bytes = reserve, available_free_bytes = available,
            inventory_sha256 = inventoryHash, entitlement_verified = true });
        // No payload requests occur until the UI confirms this exact plan.
        planning.CancelAfter(Timeout.InfiniteTimeSpan);
        var command = await ContentProtocol.Next("download", owner.Token)
            .WaitAsync(TimeSpan.FromMinutes(30), owner.Token);
        if (command.GetProperty("token").GetString() != confirmation)
            throw new InvalidDataException("Download plan confirmation mismatch");
        CheckTree(owned);
        CheckDisk(owned, reserve);
        var names = prepared.SelectMany(v => v.allFileNames).ToHashSet();
        foreach (var depot in prepared)
            await DownloadSteam3AsyncDepotFiles(owner, counter, depot, names);
        ContentProtocol.Emit(new { @event = "verifying" });
        foreach (var file in SetupFiles)
        {
            owner.Token.ThrowIfCancellationRequested();
            var path = Path.Combine(Config.InstallDirectory, file.path);
            ContentRules.NoLinks(path);
            await using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
            if ((ulong)stream.Length != file.bytes || Convert.ToHexString(await SHA1.HashDataAsync(stream, owner.Token))
                .ToLowerInvariant() != file.sha1) throw new InvalidDataException("Steam file checksum mismatch");
        }
        var complete = Path.Combine(owned, "complete.json");
        File.WriteAllText(complete + ".tmp", JsonSerializer.Serialize(new { schema = "H2_STEAM_COMPLETE_V1",
            source_id = sourceId, inventory_sha256 = inventoryHash, files = SetupFiles.Count, bytes = total,
            validated_compressed_bytes = counter.totalBytesCompressed, reused_bytes = ContentProtocol.ReusedBytes }));
        File.Move(complete + ".tmp", complete, true);
        ContentProtocol.Emit(new { @event = "complete", source = Path.Combine(Config.InstallDirectory, "halo2"),
            source_id = sourceId, inventory_sha256 = inventoryHash,
            validated_compressed_bytes = counter.totalBytesCompressed, reused_bytes = ContentProtocol.ReusedBytes });
    }
}
