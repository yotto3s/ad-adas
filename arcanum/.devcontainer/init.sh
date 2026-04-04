#!/bin/bash
# Runs on the HOST before the container is created.
# Ensures bind mount source directories exist.

# Claude Code auth directory
if [ ! -d "${HOME}/.claude" ]; then
    mkdir -p "${HOME}/.claude"
    echo "Created ${HOME}/.claude for Claude Code auth persistence"
fi

# SSH directory
if [ ! -d "${HOME}/.ssh" ]; then
    mkdir -p "${HOME}/.ssh"
    chmod 700 "${HOME}/.ssh"
    echo "Created ${HOME}/.ssh for SSH key mounting"
fi
