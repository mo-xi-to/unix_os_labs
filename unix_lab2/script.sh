#!/bin/sh

SHARED_DIR="/data"
LOCK_FILE="$SHARED_DIR/.lock"
CONTAINER_ID=$(hostname) 
COUNTER=1

echo "Container $CONTAINER_ID started"

while true; do
    {
        flock -x 200
        
        i=1
        while true; do
            NAME=$(printf "%03d" "$i")
            FILE_PATH="$SHARED_DIR/$NAME"

            if [ ! -e "$FILE_PATH" ]; then
                echo "$CONTAINER_ID $COUNTER" > "$FILE_PATH"
                break
            fi 
            i=$((i + 1))
        done
    } 200>"$LOCK_FILE"

    echo "$CONTAINER_ID created $NAME (seq $COUNTER)"

    sleep 1

    if [ -f "$SHARED_DIR/$NAME" ]; then
        rm "$SHARED_DIR/$NAME"
        echo "$CONTAINER_ID deleted $NAME"
    fi

    COUNTER=$((COUNTER + 1))
done