make all

./cco_latency -r
./cco_latency -w
./tlb_miss

python ./parsers/cco_parser.py
python ./parsers/tlb_parser.py