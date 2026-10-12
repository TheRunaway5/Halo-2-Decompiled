// Export analysis pseudocode, NOT buildable C++ or a native reimplementation.
// @category Halo2
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public class ExportDecompilation extends GhidraScript {
    public void run() throws Exception {
        Path output = Paths.get(getScriptArgs()[0]);
        Path functions = output.resolve("functions");
        Files.createDirectories(functions);
        DecompInterface decompiler = new DecompInterface();
        if (!decompiler.openProgram(currentProgram)) throw new IOException("cannot open decompiler");
        int count = 0, success = 0;
        try (BufferedWriter index = Files.newBufferedWriter(output.resolve("functions.tsv"), StandardCharsets.UTF_8);
             BufferedWriter all = Files.newBufferedWriter(output.resolve("all.c"), StandardCharsets.UTF_8)) {
            index.write("address\tbytes\tname\tstatus\terror\n");
            all.write("/* Ghidra pseudocode from the user's retail XBE. Not compilable native source. */\n");
            FunctionIterator iterator = currentProgram.getFunctionManager().getFunctions(true);
            while (iterator.hasNext()) {
                monitor.checkCancelled();
                Function function = iterator.next();
                String address = function.getEntryPoint().toString();
                String name = function.getName().replace('\t', ' ').replace('\n', ' ');
                DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                boolean ok = result.decompileCompleted() && result.getDecompiledFunction() != null;
                String error = result.getErrorMessage().replace('\t', ' ').replace('\n', ' ');
                index.write(address + "\t" + function.getBody().getNumAddresses() + "\t" + name + "\t" + (ok ? "decompiled" : "failed") + "\t" + error + "\n");
                if (ok) {
                    String source = "/* " + address + " " + name + " */\n" + result.getDecompiledFunction().getC() + "\n";
                    Files.writeString(functions.resolve(address + ".c"), source, StandardCharsets.UTF_8);
                    all.write(source);
                    success++;
                }
                count++;
                if (count % 500 == 0) { println("Exported " + success + "/" + count); index.flush(); all.flush(); }
            }
        } finally { decompiler.dispose(); }
        Files.writeString(output.resolve("summary.json"), "{\n  \"functions\": " + count + ",\n  \"decompiled\": " + success + ",\n  \"failed\": " + (count - success) + ",\n  \"output_kind\": \"analysis pseudocode, not buildable game source\"\n}\n");
        println("Decompilation export complete: " + success + "/" + count);
    }
}
