// Seed optionally supplied function boundaries from a matching retail inventory.
// @category Halo2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
import java.util.*;

public class SeedFunctions extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) return;
        int seeded = 0;
        for (String row : Files.readAllLines(Paths.get(args[0]))) {
            monitor.checkCancelled();
            String[] fields = row.split("\t", -1);
            Address address = toAddr(Long.parseUnsignedLong(fields[0], 16));
            if (!currentProgram.getMemory().contains(address)) continue;
            String name = fields[1].isEmpty() ? "FUN_" + fields[0] : fields[1];
            try {
                disassemble(address);
                Function function = getFunctionAt(address);
                if (function == null) function = createFunction(address, name);
                if (function != null && !fields[1].isEmpty()) function.setName(name, SourceType.USER_DEFINED);
                seeded++;
            } catch (Exception error) {
                println("seed " + address + ": " + error.getMessage());
            }
        }
        println("Seeded " + seeded + " function addresses");
    }
}
