mkdir temp
..\lib\Benchmark\Release\x64\Benchmark.exe --benchmark_format=csv > ./temp/bench_raw.csv
..\lib\BenchmarkProcessor\Debug\x64\BenchmarkProcessor.exe ./temp/bench_raw.csv ./temp/bench_raw_processed.csv
python benchmark_grapher.py -q -f ./temp/bench_raw_processed.csv --output ./temp/bench --transform nanos_to_micros
