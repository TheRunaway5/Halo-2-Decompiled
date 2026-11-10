#!/usr/bin/env python3
"""Lossless hashes, bounded chunks, corruption and unchanged retention limits."""
import gzip,hashlib,json,os,random,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_metadata as storage

class NativeMetadata(unittest.TestCase):
    def setUp(self):
        self.owner=tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'));self.addCleanup(self.owner.cleanup);self.root=Path(self.owner.name)
    def test_plain_and_gzip_preserve_original_bytes_hash_and_json(self):
        data=b'{ "source": "same bytes", "count": 17 }\n';path=self.root/'source-hashes.json';path.write_bytes(data)
        self.assertEqual(storage.read_bytes(path),data)
        result=storage.retain_bytes(data,path,self.root)
        self.assertFalse(path.exists());self.assertEqual(storage.read_bytes(path),data);self.assertEqual(storage.load(path),json.loads(data))
        self.assertEqual(storage.digest(path),hashlib.sha256(data).hexdigest());self.assertEqual(result['raw_sha256'],storage.digest(path));first=Path(result['path']).read_bytes()
        self.assertEqual(storage.retain_bytes(data,path,self.root)['path'],result['path']);self.assertEqual(Path(result['path']).read_bytes(),first)
    def test_large_source_manifest_chunks_remain_bounded_and_authenticated(self):
        data=json.dumps({'payload':random.Random(17).randbytes(1_400_000).hex()}).encode();path=self.root/'source-hashes.json';path.write_bytes(data)
        result=storage.retain_bytes(data,path,self.root);self.assertTrue(result['path'].endswith('.index.json'))
        self.assertEqual(result['stored_sha256'],hashlib.sha256(Path(result['path']).read_bytes()).hexdigest())
        self.assertEqual(result['stored_bytes'],sum(row['bytes']for row in result['storage_files']))
        self.assertTrue(all(hashlib.sha256(Path(row['path']).read_bytes()).hexdigest()==row['sha256']for row in result['storage_files']))
        self.assertTrue(all(p.stat().st_size<=storage.FILE_LIMIT for p in storage.files(self.root)));self.assertEqual(storage.read_bytes(path),data)
        index=json.loads(Path(result['path']).read_text());part=self.root/index['chunks'][0]['name'];changed=bytearray(part.read_bytes());changed[30]^=1;part.write_bytes(changed)
        with self.assertRaisesRegex(ValueError,'chunk hash'):storage.read_bytes(path)
    def test_runtime_decoded_one_mib_limit_is_unchanged(self):
        path=self.root/'run.json'
        with self.assertRaisesRegex(ValueError,'Raw metadata exceeds'):storage.retain_bytes(b'0'*(storage.FILE_LIMIT+1),path,self.root,max_bytes=storage.FILE_LIMIT)
        compressed=Path(str(path)+'.gz');compressed.write_bytes(gzip.compress(b'0'*(storage.FILE_LIMIT+1),mtime=0))
        with self.assertRaisesRegex(ValueError,'Decoded metadata exceeds'):storage.read_bytes(path,max_bytes=storage.FILE_LIMIT)
    def test_ambiguous_plain_compressed_inputs_are_rejected(self):
        path=self.root/'run.json';path.write_bytes(b'{}');Path(str(path)+'.gz').write_bytes(gzip.compress(b'{}',mtime=0))
        with self.assertRaisesRegex(ValueError,'Ambiguous'):storage.load(path)
    def test_chunk_index_cannot_escape_or_duplicate_owned_parts(self):
        data=json.dumps({'payload':random.Random(18).randbytes(1_400_000).hex()}).encode();path=self.root/'large.json';result=storage.retain_bytes(data,path,self.root);index_path=Path(result['path']);original=json.loads(index_path.read_text())
        for name in('../outside.gz',str(self.root/'absolute.gz'),original['chunks'][0]['name']):
            changed=json.loads(json.dumps(original));changed['chunks'][1]['name']=name;index_path.write_text(json.dumps(changed))
            with self.assertRaisesRegex(ValueError,'chunk name'):storage.load(path)
    def test_eight_mib_plus_reserve_rejection_is_atomic(self):
        for n in range(7):(self.root/f'existing-{n}.json').write_bytes(b'0'*storage.FILE_LIMIT)
        before={p.name:p.read_bytes()for p in self.root.iterdir()};payload=json.dumps({'bytes':random.Random(19).randbytes(1_000_000).hex()}).encode()
        with self.assertRaisesRegex(storage.MetadataBudgetError,'8MiB'):storage.retain_bytes(payload,self.root/'new.json',self.root)
        self.assertEqual(before,{p.name:p.read_bytes()for p in self.root.iterdir()})
    def test_relative_destination_and_owned_file_replacement(self):
        path=self.root/'owned.json';path.write_text('{"x":1}');source=self.root/'input';source.write_bytes(b'{"x":2}\n')
        result=storage.retain_file(source,path,self.root);self.assertEqual(storage.read_bytes(path),source.read_bytes());self.assertFalse(path.exists());self.assertTrue(Path(result['path']).is_file())
        with self.assertRaisesRegex(ValueError,'owned JSON'):storage.retain_bytes(b'{}',self.root.parent/'outside.json',self.root)
    def test_reserve_and_file_limits_are_not_relaxed(self):
        self.assertEqual((storage.FILE_LIMIT,storage.TOTAL_LIMIT,storage.RESERVE),(1_048_576,8_388_608,131_072))
        (self.root/'too-large.json').write_bytes(b'0'*(storage.FILE_LIMIT+1))
        with self.assertRaisesRegex(storage.MetadataBudgetError,'1MiB'):storage.check_budget(self.root)

if __name__=='__main__':unittest.main()
