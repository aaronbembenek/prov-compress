outfile=results/compression_results.data
metafile=compressed_metadata.txt
idfile=identifiers.txt
commonstrtxtfile=common_strs.txt
commonstrbinfile=common_strs.bin
graphfile=graph.cpg
rm $outfile
touch $outfile

for f in results/*.prov; do
    (echo File $f) >> $outfile
    cd ../compression && ./main.py ../benchmarks/$f  >> ../benchmarks/$outfile 
    xz -9 -k $idfile
    xz -9 -k $commonstrtxtfile
    xz -9 -k $commonstrbinfile
    mdsize=$(ls -lrt $metafile | awk '{ total += $5 }; END { print total }')
    idsize=$(ls -lrt $idfile.xz | awk '{ total += $5 }; END { print total }')
    commonstrsize=$(ls -lrt $commonstrbinfile.xz $commonstrtxtfile | awk '{ total += $5 }; END { print total }')
    graphsize=$(ls -lrt $graphfile | awk '{ total += $5 }; END { print total }')
    csize=$(($mdsize + $idsize + $commonstrsize + $graphsize))
    rm *.xz
    cd ../benchmarks
    xz -9 $f 
    xzsize=$(ls -lrt $f.xz | awk '{ total += $5 }; END { print total }')
    xz -d $f.xz
    original=$(ls -lrt $f | awk '{ total += $5 }; END { print total }')
    (echo $mdsize $idsize $graphsize $csize $xzsize $original $(bc <<<"scale=2;$original/$csize") $(bc <<<"scale=2;$original/$xzsize") $commonstrsize) >> $outfile 
done
