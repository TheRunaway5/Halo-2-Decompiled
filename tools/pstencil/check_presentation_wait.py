"""Check native image-acquisition timing, without treating stream FPS as native FPS."""
import argparse
import csv
import statistics

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('timings')
parser.add_argument('--maximum-median-ms', type=float, default=100)
parser.add_argument('--skip', type=int, default=1)
parser.add_argument('--minimum-frames', type=int, default=20)
args = parser.parse_args()
with open(args.timings) as stream:
    rows = [r for r in csv.DictReader(stream, delimiter='\t') if r.get('acquire_ms')]
rows = rows[args.skip:]
assert len(rows) >= args.minimum_frames, 'Insufficient native frames to assess acquisition timing'
median = statistics.median(float(r['acquire_ms']) for r in rows)
print(f'Native frames={len(rows)}, median image acquisition={median:.4f}ms')
assert median < args.maximum_median_ms, 'Native image acquisition exceeds the diagnostic limit'
