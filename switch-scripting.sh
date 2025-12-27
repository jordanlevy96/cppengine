#!/bin/bash

# Script to switch between LUA and PYTHON scripting modes
# Usage: ./switch-scripting.sh [lua|python]

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VSCODE_SETTINGS="$SCRIPT_DIR/.vscode/settings.json"
CMAKE_CACHE="$SCRIPT_DIR/build/CMakeCache.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

usage() {
    echo "Usage: $0 [lua|python]"
    echo ""
    echo "Updates VS Code settings to hide irrelevant files for the selected scripting mode."
    echo ""
    echo "Options:"
    echo "  lua     - Hide Python files, show Lua files"
    echo "  python  - Hide Lua files, show Python files"
    echo ""
    echo "After running this script, you'll need to manually reconfigure CMake and rebuild."
    exit 1
}

update_vscode_settings() {
    local mode=$1

    echo -e "${YELLOW}Updating VS Code settings...${NC}"

    # Create a temporary file
    local temp_file=$(mktemp)

    # Read the settings file and update it
    if [[ "$mode" == "PYTHON" ]]; then
        # Uncomment Lua file excludes (lines 94-98), comment Python file excludes (lines 101-105)
        sed -e '94,98 s|^  // |  |' \
            -e '101,105 s|^  \([^/]\)|  // \1|' \
            "$VSCODE_SETTINGS" > "$temp_file"
    else
        # Comment Lua file excludes (lines 94-98), uncomment Python file excludes (lines 101-105)
        sed -e '94,98 s|^  \([^/]\)|  // \1|' \
            -e '101,105 s|^  // |  |' \
            "$VSCODE_SETTINGS" > "$temp_file"
    fi

    # Replace the original file
    mv "$temp_file" "$VSCODE_SETTINGS"

    echo -e "${GREEN}✓ VS Code settings updated for $mode mode${NC}"
}

update_cmake_cache() {
    local mode=$1

    if [[ ! -f "$CMAKE_CACHE" ]]; then
        echo -e "${YELLOW}CMake cache not found. You'll need to run cmake with -DSCRIPTING_LANG=$mode${NC}"
        return
    fi

    echo -e "${YELLOW}Updating CMake cache...${NC}"

    # Create a temporary file
    local temp_file=$(mktemp)

    # Update SCRIPTING_LANG in CMakeCache.txt
    sed "s|^SCRIPTING_LANG:STRING=.*|SCRIPTING_LANG:STRING=$mode|" "$CMAKE_CACHE" > "$temp_file"

    # Replace the original file
    mv "$temp_file" "$CMAKE_CACHE"

    echo -e "${GREEN}✓ CMake cache updated${NC}"
}

main() {
    # Check arguments
    if [[ $# -ne 1 ]]; then
        usage
    fi

    # Parse mode
    local mode
    local input=$(echo "$1" | tr '[:upper:]' '[:lower:]')  # Convert to lowercase
    case "$input" in
        lua)
            mode="LUA"
            ;;
        python)
            mode="PYTHON"
            ;;
        *)
            echo -e "${RED}Error: Invalid mode '$1'${NC}"
            usage
            ;;
    esac

    echo -e "${GREEN}Switching to $mode scripting mode...${NC}"
    echo ""

    # Execute steps
    update_vscode_settings "$mode"
    update_cmake_cache "$mode"

    echo ""
    echo -e "${GREEN}✓ Successfully switched to $mode mode${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Reload VS Code for file exclusions to take effect"
    echo "  2. Reconfigure: cd build && cmake .."
    echo "  3. Rebuild: make -j4"
}

main "$@"
