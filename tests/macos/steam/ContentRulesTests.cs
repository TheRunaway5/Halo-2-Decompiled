using System.Text.Json;
using Halo2.SteamSetup;
using SteamKit2;

internal static class ContentRulesTests
{
    private static int cases;
    private static void Require(bool value)
    {
        cases++;
        if (!value) throw new Exception("Steam content policy assertion failed: " + cases);
    }
    private static void Reject(Action action)
    {
        try { action(); }
        catch (Exception error) when (error is InvalidDataException or IOException) { cases++; return; }
        throw new Exception("Steam content policy accepted an invalid input");
    }
    private static KeyValue Node(string name, JsonElement value)
    {
        var node = new KeyValue(name);
        if (value.ValueKind == JsonValueKind.Object)
            foreach (var child in value.EnumerateObject()) node.Children.Add(Node(child.Name, child.Value));
        else node.Value = value.ToString();
        return node;
    }
    public static void Main(string[] args)
    {
        using var fixture = JsonDocument.Parse(File.ReadAllText(args[0]));
        var depots = Node("depots", fixture.RootElement.GetProperty("apps").GetProperty("976730").GetProperty("depots"));
        var selected = ContentRules.SelectDepots(depots);
        Require(selected.Select(d => d.Id).SequenceEqual(new uint[] { 976731, 976735, 976736, 976737 }));
        Require(selected[0].Role == "shared" && selected[1].Role == "halo2");
        Require(selected[2].Role == "halo2_multiplayer" && selected[3].Role == "halo2_multiplayer");
        var source = ContentRules.SourceId(selected);
        Require(source.Length == 24 && source == ContentRules.SourceId(selected.AsEnumerable().Reverse()));
        Require(source != ContentRules.SourceId(selected.Select(d => d with { Manifest = d.Manifest + 1 })));
        var gid = depots["976735"]["manifests"]["public"]["gid"];
        var old = gid.Value; gid.Value = "0";
        Reject(() => ContentRules.SelectDepots(depots)); gid.Value = old;
        var dlc = depots["976735"]["dlcappid"]; old = dlc.Value; dlc.Value = "1064271";
        Reject(() => ContentRules.SelectDepots(depots)); dlc.Value = old;
        var extra = new KeyValue("999999");
        extra.Children.Add(Node("manifests", JsonDocument.Parse("{\"public\":{\"gid\":\"1234\"}}").RootElement));
        depots.Children.Add(extra); Reject(() => ContentRules.SelectDepots(depots)); depots.Children.Remove(extra);
        foreach (var path in new[] { "", "/halo2/x", "../x", "halo2/../x", "halo2//x", "C:/halo2/x",
                                     "halo2/x.", "halo2/x ", "halo2/a\0b", "halo2/./x", "\\halo2\\x" })
            Reject(() => ContentRules.SafePath(path));
        Require(ContentRules.SafePath("halo2\\h2_maps_win64_dx11\\mainmenu.map") == "halo2/h2_maps_win64_dx11/mainmenu.map");
        foreach (var path in new[] { "halo2/h2_maps_win64_dx11/mainmenu.map", "HALO2/h2_fonts/font_table.txt",
                                     "MCC/Content/Paks/MCC-WindowsNoEditor.pak" }) Require(ContentRules.Includes(path));
        foreach (var path in new[] { "halo3/mainmenu.map", "halo2/prebuild/shaders_2_cache/s.shader",
                                     "halo2/a.EXE", "halo2/a.dll", "MCC/Content/Paks/other.pak", "halo2-other/x.map" })
            Require(!ContentRules.Includes(path));
        Reject(() => ContentRules.ValidateComplete([]));
        Reject(() => ContentRules.ValidateComplete([new("halo2/a", 1, 1, "00", 976735, "1"),
                                                   new("HALO2/A", 1, 1, "00", 976735, "1")]));
        var manifest = new DepotManifest { Files = [] };
        var asset = new DepotManifest.FileData { FileName = "halo2/test.map", FileHash = new byte[20], TotalSize = 4,
            Chunks = [new() { Offset = 0, UncompressedLength = 4, CompressedLength = 8, ChunkID = new byte[20] }] };
        manifest.Files.Add(asset);
        var inventory = ContentRules.SelectFiles(976735, 42, manifest);
        Require(inventory.Count == 1 && inventory[0].bytes == 4 && inventory[0].download_bytes == 8 &&
                inventory[0].manifest_id == "42" && inventory[0].sha1.Length == 40);
        asset.LinkTarget = "outside"; Reject(() => ContentRules.SelectFiles(976735, 42, manifest)); asset.LinkTarget = null;
        asset.Chunks[0].Offset = 1; Reject(() => ContentRules.SelectFiles(976735, 42, manifest)); asset.Chunks[0].Offset = 0;
        asset.TotalSize = 5; Reject(() => ContentRules.SelectFiles(976735, 42, manifest)); asset.TotalSize = 4;
        asset.FileHash = []; Reject(() => ContentRules.SelectFiles(976735, 42, manifest)); asset.FileHash = new byte[20];
        manifest.FilenamesEncrypted = true; Reject(() => ContentRules.SelectFiles(976735, 42, manifest));
        manifest.FilenamesEncrypted = false;
        manifest.Files.Add(new() { FileName = "HALO2/TEST.map", FileHash = new byte[20] });
        Reject(() => ContentRules.SelectFiles(976735, 42, manifest)); manifest.Files.RemoveAt(1);
        Reject(() => ContentRules.SelectFiles(976731, 42, manifest));
        using var profile = JsonDocument.Parse(File.ReadAllText(args[1]));
        var required = profile.RootElement.GetProperty("required_files").EnumerateArray().Select(v =>
            new ContentFile(v.GetString()!, 0, 0, new string('0', 40), 976735, "42")).ToList();
        ContentRules.ValidateComplete(required); cases++;
        Reject(() => ContentRules.ValidateComplete(required.Skip(1)));
        required.Add(new("halo2/A", 0, 0, new string('0', 40), 976735, "42"));
        required.Add(new("halo2/a/child", 0, 0, new string('0', 40), 976735, "42"));
        Reject(() => ContentRules.ValidateComplete(required));
        var duplicate = new ContentFile("halo2/file.map", 4, 8, new string('0', 40), 976735, "42");
        var composed = ContentRules.ComposeFiles([duplicate, duplicate with { depot_id = 976737, download_bytes = 9 }]);
        Require(composed.Count == 1 && composed[0] == duplicate);
        Reject(() => ContentRules.ComposeFiles([duplicate, duplicate]));
        Reject(() => ContentRules.ComposeFiles([duplicate, duplicate with { depot_id = 976737, bytes = 5 }]));
        Reject(() => ContentRules.ComposeFiles([duplicate, duplicate with { depot_id = 976737, sha1 = new string('1', 40) }]));
        Reject(() => ContentRules.ComposeFiles([duplicate, duplicate with { depot_id = 976737, path = "HALO2/FILE.map" }]));
        var scratch = Environment.GetEnvironmentVariable("HALO2_TEST_SCRATCH") ?? throw new Exception("No test owner");
        var target = Path.Combine(scratch, "actual"); Directory.CreateDirectory(target);
        var link = Path.Combine(scratch, "linked"); Directory.CreateSymbolicLink(link, target);
        Reject(() => ContentRules.NoLinks(Path.Combine(link, "not-created")));
        File.Delete(link);
        var dangling = Path.Combine(scratch, "dangling"); File.CreateSymbolicLink(dangling, Path.Combine(scratch, "missing"));
        Reject(() => ContentRules.NoLinks(dangling)); File.Delete(dangling);
        ContentRules.NoLinks(Path.Combine(target, "not-created")); cases++;
        Console.WriteLine(JsonSerializer.Serialize(new { schema = "H2_STEAM_RULES_TEST_V1", cases, source_id = source }));
    }
}
