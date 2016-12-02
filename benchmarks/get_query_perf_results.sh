file=queries.perf

for f in results/*.prov; do
    cd ../compression && ./main.py ../benchmarks/$f
    cd ../querier 
    perffile=$f.$file
    make && ./query --auditfile=$f > ../benchmarks/$perffile
done
