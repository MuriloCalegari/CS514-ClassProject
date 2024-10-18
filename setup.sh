#!/bin/zsh

# Update Homebrew to the latest version
brew update

# Install CMake using Homebrew
brew install cmake

# Verify the installation
if brew list cmake &>/dev/null; then
    echo "cmake has been successfully installed."
else
    echo "There was an error installing cmake."
fi

