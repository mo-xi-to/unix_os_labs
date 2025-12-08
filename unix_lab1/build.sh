#!/bin/sh

cleanup() {
    exit_code=$?
    if [ -n "$TEMP_DIR" ] && [ -d "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
    fi
    exit $exit_code
}

trap cleanup INT TERM EXIT

if [ $# -ne 1 ]; then
    echo "Error: No source file specified" >&2
    echo "Usage: $0 <source_file>" >&2
    exit 1
fi

SOURCE_FILE="$1"

if [ ! -f "$SOURCE_FILE" ] || [ ! -r "$SOURCE_FILE" ]; then
    echo "Error: File '$SOURCE_FILE' does not exist or is not readable" >&2
    exit 2
fi
OUTPUT_NAME=$(grep 'Output:' "$SOURCE_FILE" | head -1 | sed 's/.*Output:[[:space:]]*//' | sed 's/[[:space:]]*$//')

if [ -z "$OUTPUT_NAME" ]; then
    echo "Error: Output filename comment not found in '$SOURCE_FILE'." >&2
    echo "Example comment: 'Output: myprogram' or '% Output: document.pdf'" >&2
    exit 3
fi

TEMP_DIR=$(mktemp -d)
if [ $? -ne 0 ]; then
    echo "Error: Failed to create temporary directory." >&2
    exit 4
fi

echo "Building in temporary directory: $TEMP_DIR"
echo "Target file: $OUTPUT_NAME"
case "$SOURCE_FILE" in
    *.c)
        cp "$SOURCE_FILE" "$TEMP_DIR/"
        if ! gcc "$TEMP_DIR/$(basename "$SOURCE_FILE")" -o "$TEMP_DIR/$OUTPUT_NAME"; then
            echo "Error: C compilation failed" >&2
            exit 5
        fi
        ;;
    *.cpp|*.cxx|*.cc)
        cp "$SOURCE_FILE" "$TEMP_DIR/"
        if ! g++ "$TEMP_DIR/$(basename "$SOURCE_FILE")" -o "$TEMP_DIR/$OUTPUT_NAME"; then
            echo "Error: C++ compilation failed" >&2
            exit 6
        fi
        ;;
    *.tex)
        cp "$SOURCE_FILE" "$TEMP_DIR/"
        cd "$TEMP_DIR" || exit 7
        if ! pdflatex -interaction=nonstopmode "$(basename "$SOURCE_FILE")" > /dev/null; then
            echo "Error: First pass of LaTeX compilation failed." >&2
            exit 8
        fi
        if ! pdflatex -interaction=nonstopmode "$(basename "$SOURCE_FILE")" > /dev/null; then
            echo "Error: Second pass of LaTeX compilation failed." >&2
            exit 9
        fi
        cd - > /dev/null
        ;;
    *)
        echo "Error: Unsupported file type. Supported: .c, .cpp, .cxx, .cc, .tex" >&2
        exit 10
        ;;
esac
if ! cp "$TEMP_DIR/$OUTPUT_NAME" "./"; then
    echo "Error: Failed to copy '$OUTPUT_NAME' to current directory." >&2
    exit 11
fi

echo "Build successful! File '$OUTPUT_NAME' created."