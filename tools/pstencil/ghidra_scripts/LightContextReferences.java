// Read-only primary reference discovery; no saved program mutations.
// @category Halo2
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.nio.file.*;
import java.util.*;
public class LightContextReferences extends GhidraScript {
 public void run() throws Exception {
  Path out=Paths.get(getScriptArgs()[0]);TreeSet<String> seeds=new TreeSet<>();seeds.add("001cc280");
  StringBuilder refs=new StringBuilder("target\tsource\tkind\tcontaining_entry\tname\n");
  for(String target:new String[]{"000a4c50","000f7f20","007438a4"}) {
   if(!target.startsWith("007"))seeds.add(target);
   for(Reference ref:getReferencesTo(toAddr(Long.parseLong(target,16)))) {
    Function f=getFunctionContaining(ref.getFromAddress());
    refs.append(target+"\t"+ref.getFromAddress()+"\t"+ref.getReferenceType()+"\t"+(f==null?"":f.getEntryPoint())+"\t"+(f==null?"":f.getName())+"\n");
    if(f!=null)seeds.add(f.getEntryPoint().toString());
   }
  }
  FunctionIterator functions=currentProgram.getFunctionManager().getFunctions(true);
  while(functions.hasNext()) {Function f=functions.next();if(f.getName().equals("rasterizer_build_light_constants") || f.getName().equals("rasterizer_light_end"))seeds.add(f.getEntryPoint().toString());}
  Files.writeString(out.resolve("references.tsv"),refs.toString());
  Files.writeString(out.resolve("seeds.tsv"),String.join("\n",seeds)+"\n");
 }
}
