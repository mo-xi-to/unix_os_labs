#!/bin/sh

SHARED_VOLUME="/data"
LOCK_PATH="$SHARED_VOLUME/.container_lock"
CONTAINER_UUID=$(hostname)
FILE_NUMBER=1

echo "Container $CONTAINER_UUID started"

while true; do
    {
        flock -x 200
        
        attempt=1
        while true; do
            FILENAME=$(printf "%03d" "$attempt")
            FULL_PATH="$SHARED_VOLUME/$FILENAME"

            if [ ! -e "$FULL_PATH" ]; then
                echo "$CONTAINER_UUID $FILE_NUMBER" > "$FULL_PATH"
                break
            fi 
            attempt=$((attempt + 1))
        done
    } 200>"$LOCK_PATH"

    echo "$CONTAINER_UUID created $FILENAME (seq $FILE_NUMBER)"

    sleep 1

    if [ -f "$SHARED_VOLUME/$FILENAME" ]; then
        rm "$SHARED_VOLUME/$FILENAME"
        echo "$CONTAINER_UUID deleted $FILENAME"
    fi

    FILE_NUMBER=$((FILE_NUMBER + 1))
done