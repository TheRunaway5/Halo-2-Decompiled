using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using SteamKit2;

namespace Halo2.SteamSetup;

internal enum ContentIssue { UnsafePath, EmptyManifest, EncryptedPaths, AmbiguousFile,
    InvalidChunks, IncompleteChunks, MissingRequiredFile, OverlappingFiles, OverlappingDirectory, UnexpectedManifest }

internal sealed class ContentPolicyException(ContentIssue issue, object? details = null) : IOException
{
    internal ContentIssue Issue { get; } = issue;
    internal object? Details { get; } = details;
}

internal sealed record ContentFile(string path, ulong bytes, ulong download_bytes,
    string sha1, uint depot_id, string manifest_id);
internal sealed record DepotSelection(uint Id, ulong Manifest, string Role);

internal static class ContentRules
{
    internal const uint App = 976730, Dlc = 1064270, MultiplayerDlc = 1097223;
    private static readonly JsonElement Profile = JsonDocument.Parse(
        typeof(ContentRules).Assembly.GetManifestResourceStream("Halo2Steam.content-profile.json")
        ?? throw new InvalidDataException("Missing content profile")).RootElement.Clone();
    private static readonly HashSet<string> Shared = Profile.GetProperty("shared_files").EnumerateArray()
        .Select(v => v.GetString()!).ToHashSet(StringComparer.OrdinalIgnoreCase);
    private static readonly HashSet<string> Excluded = Profile.GetProperty("excluded_suffixes").EnumerateArray()
        .Select(v => v.GetString()!).ToHashSet(StringComparer.OrdinalIgnoreCase);

    internal static string SafePath(string path)
    {
        path = path.Replace('\\', '/');
        if (path.Length is 0 or > 4096 || path[0] == '/' || path.Contains(':') || path.Any(c => c < 32) ||
            path.Split('/').Any(p => p is "" or "." or ".." || p.EndsWith(' ') || p.EndsWith('.')))
            throw new ContentPolicyException(ContentIssue.UnsafePath, new
            {
                directory = false, path_length = path.Length, empty = path.Length == 0,
                trailing_separator = path.EndsWith('/'),
                path_sha256 = Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(path))).ToLowerInvariant()
            });
        return path;
    }

    internal static bool Includes(string path) => Shared.Contains(path) ||
        (path.StartsWith("halo2/", StringComparison.OrdinalIgnoreCase) &&
         !Excluded.Contains(Path.GetExtension(path)) &&
         !(path + '/').StartsWith("halo2/prebuild/shaders_2_cache/", StringComparison.OrdinalIgnoreCase));

    internal static List<DepotSelection> SelectDepots(KeyValue depots)
    {
        var expected = Profile.GetProperty("depots").EnumerateObject()
            .ToDictionary(v => uint.Parse(v.Name), v => v.Value.GetString()!);
        var result = new List<DepotSelection>();
        foreach (var node in depots.Children)
        {
            if (!uint.TryParse(node.Name, out var id) || node["depotfromapp"].Value is not null) continue;
            var publicManifest = node["manifests"]["public"]["gid"].Value;
            if (publicManifest is null) continue;
            var dlc = node["dlcappid"].AsUnsignedInteger();
            if (dlc != 0 && dlc != Dlc && dlc != MultiplayerDlc) continue;
            var config = node["config"];
            if (config["language"].Value is { Length: > 0 } language && language != "english") continue;
            if (config["oslist"].Value is { Length: > 0 } os && !os.Split(',').Contains("windows")) continue;
            if (config["osarch"].Value is { Length: > 0 } arch && arch != "64") continue;
            if (!expected.TryGetValue(id, out var role) || Profile.GetProperty("depot_dlc").GetProperty(id.ToString()).GetUInt32() != dlc ||
                !ulong.TryParse(publicManifest, out var manifest) || manifest == 0)
                throw new InvalidDataException("MCC content layout changed");
            result.Add(new(id, manifest, role));
        }
        if (!result.Select(v => v.Id).ToHashSet().SetEquals(expected.Keys))
            throw new InvalidDataException("Incomplete MCC content layout");
        return result.OrderBy(v => v.Id).ToList();
    }

    internal static List<ContentFile> SelectFiles(uint depot, ulong id, DepotManifest manifest)
    {
        if (manifest.FilenamesEncrypted) throw new ContentPolicyException(ContentIssue.EncryptedPaths);
        var files = new List<ContentFile>();
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var file in manifest.Files ?? throw new ContentPolicyException(ContentIssue.EmptyManifest))
        {
            string path;
            try { path = SafePath(file.FileName); }
            catch (ContentPolicyException error)
            {
                throw new ContentPolicyException(error.Issue, new
                {
                    directory = file.Flags.HasFlag(EDepotFileFlag.Directory), path_length = file.FileName.Length,
                    empty = file.FileName.Length == 0, trailing_separator = file.FileName.EndsWith('/') || file.FileName.EndsWith('\\'),
                    path_sha256 = Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(file.FileName))).ToLowerInvariant()
                });
            }
            file.FileName = path;
            if (file.Flags.HasFlag(EDepotFileFlag.Directory)) continue;
            if (!Includes(path)) continue;
            if (!string.IsNullOrEmpty(file.LinkTarget) ||
                !names.Add(path.Normalize(NormalizationForm.FormC)) || file.FileHash.Length != 20 ||
                (depot == 976731) != Shared.Contains(path))
                throw new ContentPolicyException(ContentIssue.AmbiguousFile, new
                {
                    linked = !string.IsNullOrEmpty(file.LinkTarget), hash_bytes = file.FileHash.Length,
                    depot_id = depot, shared = Shared.Contains(path)
                });
            ulong end = 0, compressed = 0;
            foreach (var chunk in file.Chunks.OrderBy(c => c.Offset))
            {
                if (chunk.Offset != end || chunk.UncompressedLength == 0 || chunk.CompressedLength == 0 ||
                    chunk.ChunkID?.Length != 20) throw new ContentPolicyException(ContentIssue.InvalidChunks, new
                    {
                        expected_offset = end, actual_offset = chunk.Offset, uncompressed = chunk.UncompressedLength,
                        compressed = chunk.CompressedLength, chunk_id_bytes = chunk.ChunkID?.Length ?? 0
                    });
                end = checked(end + chunk.UncompressedLength);
                compressed = checked(compressed + chunk.CompressedLength);
            }
            if (end != file.TotalSize) throw new ContentPolicyException(ContentIssue.IncompleteChunks,
                new { chunk_bytes = end, file_bytes = file.TotalSize });
            files.Add(new(path, file.TotalSize, compressed,
                Convert.ToHexString(file.FileHash).ToLowerInvariant(), depot, id.ToString()));
        }
        return files;
    }

    internal static void ValidateComplete(IEnumerable<ContentFile> files)
    {
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var file in files)
            if (!names.Add(file.path.Normalize(NormalizationForm.FormC)))
                throw new ContentPolicyException(ContentIssue.OverlappingFiles);
        foreach (var name in names)
            for (var end = name.IndexOf('/'); end >= 0; end = name.IndexOf('/', end + 1))
                if (names.Contains(name[..end])) throw new ContentPolicyException(ContentIssue.OverlappingDirectory);
        foreach (var required in Profile.GetProperty("required_files").EnumerateArray())
            if (!names.Contains(required.GetString()!)) throw new ContentPolicyException(ContentIssue.MissingRequiredFile,
                new { required_path = required.GetString(), selected_files = names.Count });
    }

    internal static List<ContentFile> ComposeFiles(IEnumerable<ContentFile> files)
    {
        var owners = new Dictionary<string, ContentFile>(StringComparer.OrdinalIgnoreCase);
        foreach (var file in files)
        {
            var key = file.path.Normalize(NormalizationForm.FormC);
            if (!owners.TryGetValue(key, out var prior)) { owners.Add(key, file); continue; }
            // Shared campaign/multiplayer files are transferable from either
            // authorized depot only when their complete content agrees. Keep
            // the first declared owner, its exact path and its transfer cost.
            if (prior.path != file.path || prior.depot_id == file.depot_id ||
                prior.bytes != file.bytes || prior.sha1 != file.sha1)
                throw new ContentPolicyException(ContentIssue.OverlappingFiles, new
                {
                    path = prior.path,
                    owners = new[] { new { depot_id = prior.depot_id, bytes = prior.bytes, sha1 = prior.sha1 },
                                     new { depot_id = file.depot_id, bytes = file.bytes, sha1 = file.sha1 } }
                });
        }
        return owners.Values.ToList();
    }

    internal static string SourceId(IEnumerable<DepotSelection> depots) =>
        Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(string.Join(';',
            depots.OrderBy(d => d.Id).Select(d => $"{d.Id}:{d.Manifest}"))))).ToLowerInvariant()[..24];

    internal static void NoLinks(string path)
    {
        var full = Path.GetFullPath(path);
        for (string? current = full; current is not null; current = Path.GetDirectoryName(current))
        {
            // File.Exists follows links and misses dangling links.
            FileAttributes attributes;
            try { attributes = File.GetAttributes(current); }
            catch (FileNotFoundException) { continue; }
            catch (DirectoryNotFoundException) { continue; }
            if ((attributes & FileAttributes.ReparsePoint) != 0) throw new IOException("Linked download path");
        }
    }
}
