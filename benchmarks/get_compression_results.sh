outfile=results/compression_results.out
metafile=compressed_metadata.txt
idfile=identifiers.txt
graphfile=graph.cpg
rm $outfile
touch $outfile

for f in results/*.prov; do
    (echo $f) >> results/compression_results.out 2>&1
    cd ../compression && ./main.py ../benchmarks/$f  >> ../benchmarks/$outfile 2>&1 
    xz -9 -k $idfile
    csize=$(ls -lrt $metafile $idfile.xz $graphfile | awk '{ total += $5 }; END { print total }')
    rm *.xz
    cd ../benchmarks
    xz -9 $f 
    xzsize=$(ls -lrt $f.xz | awk '{ total += $5 }; END { print total }')
    xz -d $f.xz
    original=$(ls -lrt $f | awk '{ total += $5 }; END { print total }')
    (echo $csize $xzsize $original $(bc <<<"scale=2;$original/$csize") $(bc <<<"scale=2;$original/$xzsize")) >> $outfile 2>&1
done
