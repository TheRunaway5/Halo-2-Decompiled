// Export only the inspected 2003 renderer functions; analysis, not buildable C++.
// @category Halo2
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.Function;
import java.nio.file.*;

public class ExportRenderer extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path output = Paths.get(args[1]);
        Files.createDirectories(output);
        DecompInterface decompiler = new DecompInterface();
        if (!decompiler.openProgram(currentProgram)) throw new Exception("Cannot open decompiler");
        try {
            for (String row : Files.readAllLines(Paths.get(args[0]))) {
                monitor.checkCancelled();
                String address = row.split("\t")[0];
                Function function = getFunctionAt(toAddr(Long.parseUnsignedLong(address, 16)));
                if (function == null) continue;
                DecompileResults result = decompiler.decompileFunction(function, 30, monitor);
                if (!result.decompileCompleted() || result.getDecompiledFunction() == null)
                    throw new Exception(address + ": " + result.getErrorMessage());
                Files.writeString(output.resolve(address + ".c"),
                    "/* Local 2003 XBE pseudocode. " + function.getName() + " */\n" +
                    result.getDecompiledFunction().getC());
            }
        } finally { decompiler.dispose(); }
    }
}
