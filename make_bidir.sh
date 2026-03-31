#!/bin/bash

FILE="/home/nabila/omnetpp-6.3.0/samples/auto_project/src/RingNode.ned"
TMP_FILE="tmp.ned"

> "$TMP_FILE"

while IFS= read -r line; do
    # Write original line
    echo "$line" >> "$TMP_FILE"

    # Match connection lines
    if [[ $line =~ node\[([0-9]+)\]\.outGate\+\+\ \-\-\>\ LossyChannel\ \-\-\>\ node\[([0-9]+)\]\.inGate\+\+ ]]; then
        u=${BASH_REMATCH[1]}
        v=${BASH_REMATCH[2]}

        # Reverse connection
        reverse="    node[$v].outGate++ --> LossyChannel --> node[$u].inGate++;"

        echo "$reverse" >> "$TMP_FILE"
    fi

done < "$FILE"

# Replace original file
mv "$TMP_FILE" "$FILE"
rm "$TMP_FILE"

echo "✅ Updated $FILE with bidirectional links"