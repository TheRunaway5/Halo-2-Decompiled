// Analysis-only export. Existing projects use -readOnly -noanalysis.
// Missing direct/map-proven entries are disassembled in the unsaved working
// program; no original file bytes are changed.
// @category Halo2
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.nio.file.*;
import java.security.*;
public class ExportLightContext extends GhidraScript {
 public void run() throws Exception {
  String[] a=getScriptArgs();Path out=Paths.get(a[1]);Files.createDirectories(out.resolve("functions"));Files.createDirectories(out.resolve("disassembly"));
  DecompInterface dec=new DecompInterface();if(!dec.openProgram(currentProgram))throw new Exception("decompiler unavailable");StringBuilder index=new StringBuilder("entry\tname\tstart\tend\tsha256\n");
  try{for(String row:Files.readAllLines(Paths.get(a[0]))){monitor.checkCancelled();String id=row.split("\\t")[0];Address entry=toAddr(Long.parseUnsignedLong(id,16));Function f=getFunctionAt(entry);if(f==null){Function owner=getFunctionContaining(entry);if(owner!=null){println("Entry "+id+" contained by "+owner.getEntryPoint());continue;} disassemble(entry);f=createFunction(entry,"audit_"+id);if(f==null){println("Missing function "+id);continue;}}
   Address min=f.getBody().getMinAddress(),max=f.getBody().getMaxAddress();byte[] raw=new byte[(int)max.subtract(min)+1];currentProgram.getMemory().getBytes(min,raw);byte[] digest=MessageDigest.getInstance("SHA-256").digest(raw);StringBuilder hex=new StringBuilder();for(byte v:digest)hex.append(String.format("%02x",v&255));
   index.append(id+"\t"+f.getName()+"\t"+min+"\t"+max+"\t"+hex+"\n");StringBuilder asm=new StringBuilder("; "+f.getName()+"; primary program "+currentProgram.getName()+"; entry "+id+"\n");InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);while(it.hasNext()){Instruction i=it.next();asm.append(i.getAddress()+" "+i+"\n");}
   Files.write(out.resolve("disassembly").resolve(id+".bin"),raw);Files.writeString(out.resolve("disassembly").resolve(id+".asm"),asm.toString());DecompileResults r=dec.decompileFunction(f,60,monitor);if(!r.decompileCompleted()||r.getDecompiledFunction()==null)throw new Exception(id+" "+r.getErrorMessage());Files.writeString(out.resolve("functions").resolve(id+".c"),"/* Primary "+currentProgram.getName()+"; "+id+" "+f.getName()+" */\n"+r.getDecompiledFunction().getC());
  }}finally{dec.dispose();}Files.writeString(out.resolve("functions.tsv"),index.toString());
 }
}
