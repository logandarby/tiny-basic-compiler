#!/bin/bash

# Git hooks installation script for tiny-basic-compiler
# This script installs the project's git hooks into .git/hooks/

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔗 Installing git hooks for tiny-basic-compiler...${NC}"
echo ""

# Check if we're in a git repository
if [ ! -d ".git" ]; then
    echo -e "${RED}❌ Error: Not in a git repository root${NC}"
    echo "Please run this script from the project root directory"
    exit 1
fi

# Check if hooks directory exists
if [ ! -d "hooks" ]; then
    echo -e "${RED}❌ Error: hooks/ directory not found${NC}"
    echo "Please run this script from the project root directory"
    exit 1
fi

# Create .git/hooks directory if it doesn't exist
mkdir -p .git/hooks

# Install each hook
hooks_installed=0

for hook_file in hooks/*; do
    # Skip this script and any non-executable files
    if [[ "$hook_file" == "hooks/install.sh" ]] || [[ ! -f "$hook_file" ]]; then
        continue
    fi
    
    hook_name=$(basename "$hook_file")
    destination=".git/hooks/$hook_name"
    
    # Check if hook already exists
    if [ -f "$destination" ]; then
        echo -e "${YELLOW}⚠️  Hook $hook_name already exists${NC}"
        echo -e "   Do you want to overwrite it? (y/N): \c"
        read -r response
        if [[ ! "$response" =~ ^[Yy]$ ]]; then
            echo -e "   ${BLUE}⏭️  Skipping $hook_name${NC}"
            continue
        fi
    fi
    
    # Copy and make executable
    cp "$hook_file" "$destination"
    chmod +x "$destination"
    
    echo -e "${GREEN}✅ Installed $hook_name${NC}"
    hooks_installed=$((hooks_installed + 1))
done

echo ""
if [ $hooks_installed -eq 0 ]; then
    echo -e "${YELLOW}⚠️  No hooks were installed${NC}"
else
    echo -e "${GREEN}🎉 Successfully installed $hooks_installed git hook(s)!${NC}"
    echo ""
    echo -e "${BLUE}📋 Installed hooks:${NC}"
    ls -la .git/hooks/ | grep -E '^-rwx' | awk '{print "   - " $9}' | grep -v '\.sample$' || true
    echo ""
    echo -e "${BLUE}💡 What these hooks do:${NC}"
    
    if [ -f ".git/hooks/pre-commit" ]; then
        echo "   🔍 pre-commit:  Checks formatting, builds project, runs basic validation"
    fi
    
    if [ -f ".git/hooks/pre-push" ]; then
        echo "   🚀 pre-push:    Runs full test suite and build validation before push"
    fi
    
    if [ -f ".git/hooks/commit-msg" ]; then
        echo "   📝 commit-msg:  Validates commit message format (conventional commits)"
    fi
fi

echo ""
echo -e "${BLUE}🛠️  To uninstall hooks later, run:${NC}"
echo "   rm .git/hooks/pre-commit .git/hooks/pre-push .git/hooks/commit-msg"
echo "" 