#!/bin/bash
set -e

echo "=== Arcanum devcontainer post-create setup ==="

# Ensure opam env is available
eval $(opam env)

# Fix permissions on mounted .claude directory
if [ -d "${HOME}/.claude" ]; then
    sudo chown -R $(id -u):$(id -g) "${HOME}/.claude" 2>/dev/null || true
fi

# Verify installations
echo ""
echo "--- Tool versions ---"
echo "Clang:       $(clang --version | head -1)"
echo "LLVM:        $(llvm-config --version)"
echo "CMake:       $(cmake --version | head -1)"
echo "Ninja:       $(ninja --version)"
echo "mlir-opt:    $(mlir-opt --version 2>&1 | head -1 || echo 'not found')"
echo "Why3:        $(why3 --version 2>&1 || echo 'not found')"
echo "Z3:          $(z3 --version 2>&1 || echo 'not found')"
echo "ripgrep:     $(rg --version | head -1)"
echo "fd:          $(fdfind --version)"
echo "fzf:         $(fzf --version)"
echo "ccache:      $(ccache --version | head -1)"
echo "Node.js:     $(node --version 2>/dev/null || echo 'not found')"
echo "Claude Code: $(claude --version 2>/dev/null || echo 'not found')"
echo "GTest:       $(pkg-config --modversion gtest 2>/dev/null || echo 'installed')"

# Detect Why3 provers
echo ""
echo "--- Why3 prover detection ---"
why3 config detect 2>&1 || true

# Git config status
echo ""
echo "--- Git configuration ---"
if [ -n "${GIT_AUTHOR_NAME}" ]; then
    echo "Author:  ${GIT_AUTHOR_NAME} <${GIT_AUTHOR_EMAIL}>"
else
    # Fall back to .gitconfig forwarded by VS Code
    GIT_NAME=$(git config --global user.name 2>/dev/null || true)
    GIT_EMAIL=$(git config --global user.email 2>/dev/null || true)
    if [ -n "${GIT_NAME}" ]; then
        echo "Author:  ${GIT_NAME} <${GIT_EMAIL}> (from .gitconfig)"
    else
        echo "Warning: git user not configured."
        echo "  Set on host: git config --global user.name 'Your Name'"
        echo "               git config --global user.email 'your@email.com'"
    fi
fi

# Claude Code auth status
echo ""
echo "--- Claude Code ---"
if [ -f "${HOME}/.claude/.credentials.json" ] || [ -f "${HOME}/.claude/credentials.json" ]; then
    echo "Auth: credentials found (mounted from host)"
else
    echo "Auth: not configured. Run 'claude' to authenticate."
fi

# SSH agent status (forwarded by VS Code automatically)
echo ""
echo "--- SSH agent ---"
if [ -n "${SSH_AUTH_SOCK}" ]; then
    echo "SSH agent: forwarded"
    ssh-add -l 2>/dev/null || echo "  (no keys loaded)"
else
    echo "SSH agent: not available"
fi

echo ""
echo "=== Arcanum devcontainer ready ==="
