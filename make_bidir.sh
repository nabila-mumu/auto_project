#!/bin/bash

NED_FILE="/home/nabila/omnetpp-6.3.0/samples/auto_project/src/RingNode.ned"
INI_FILE="/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/omnetpp.ini"

TMP_FILE="tmp.ned"

> "$TMP_FILE"

while IFS= read -r line; do
    echo "$line" >> "$TMP_FILE"

    if [[ $line =~ node\[([0-9]+)\]\.outGate\+\+\ \-\-\>\ LossyChannel\ \-\-\>\ node\[([0-9]+)\]\.inGate\+\+ ]]; then

        u=${BASH_REMATCH[1]}
        v=${BASH_REMATCH[2]}

        reverse="    node[$v].outGate++ --> LossyChannel --> node[$u].inGate++;"

        echo "$reverse" >> "$TMP_FILE"
    fi

done < "$NED_FILE"

mv "$TMP_FILE" "$NED_FILE"
rm "$TMP_FILE"

# ---------- Update ini file ----------
sed -i \
's/\*\.node\[\*\]\.bidirectional *= *false/\*\.node[*].bidirectional = true/' \
"$INI_FILE"

sed -i \
's/bool bidirectional *= *default(false)/bool bidirectional = default(true)/' \
"$NED_FILE"

echo "✅ Bidirectional links added"
echo "✅ bidirectional=true updated in omnetpp.ini"