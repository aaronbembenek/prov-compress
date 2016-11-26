for f in *.py; do
    sudo camflow-prov -t $f true 
    sudo camflow-prov -t $f propagate
    sudo rm -rf /tmp/audit.log
    python $f
    sudo cp /tmp/audit.log {$f}_prov.log
    sudo camflow-prov -t $f false
