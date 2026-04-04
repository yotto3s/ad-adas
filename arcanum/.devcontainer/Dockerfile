FROM arcanum-ci:latest

ARG LLVM_VERSION=22
ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

ENV DEBIAN_FRONTEND=noninteractive

# ============================================================
# LSP (clangd - not needed in CI)
# ============================================================
RUN apt-get update && apt-get install -y --no-install-recommends \
    clangd-${LLVM_VERSION} \
    && update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_VERSION} 100 \
    && rm -rf /var/lib/apt/lists/*

# ============================================================
# Development tools
# ============================================================
RUN apt-get update && apt-get install -y --no-install-recommends \
    git-lfs \
    zsh \
    tmux \
    # Search & navigation
    ripgrep \
    fd-find \
    fzf \
    silversearcher-ag \
    tree \
    jq \
    # Editors
    neovim \
    # Debug
    gdb \
    lldb-${LLVM_VERSION} \
    valgrind \
    strace \
    # Documentation
    doxygen \
    graphviz \
    # Misc
    htop \
    less \
    man-db \
    unzip \
    zip \
    openssh-client \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# ============================================================
# Create non-root user
# ============================================================
RUN userdel -r ubuntu 2>/dev/null || true \
    && groupadd --gid ${USER_GID} ${USERNAME} 2>/dev/null || true \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} -s /usr/bin/zsh \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

USER ${USERNAME}
WORKDIR /home/${USERNAME}

# ============================================================
# Why3 for dev user (reinstall in user scope)
# ============================================================
RUN opam init --auto-setup --disable-sandboxing \
    && eval $(opam env) \
    && opam install -y why3 alt-ergo \
    && eval $(opam env) \
    && why3 config detect

# ============================================================
# Shell setup (zsh + oh-my-zsh)
# ============================================================
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended \
    && git clone --depth=1 https://github.com/zsh-users/zsh-autosuggestions ${HOME}/.oh-my-zsh/custom/plugins/zsh-autosuggestions \
    && git clone --depth=1 https://github.com/zsh-users/zsh-syntax-highlighting ${HOME}/.oh-my-zsh/custom/plugins/zsh-syntax-highlighting \
    && sed -i 's/plugins=(git)/plugins=(git fzf ripgrep zsh-autosuggestions zsh-syntax-highlighting)/' ~/.zshrc

# Add opam env to shell
RUN echo 'eval $(opam env)' >> ~/.zshrc \
    && echo 'eval $(opam env)' >> ~/.bashrc

# ccache config
RUN mkdir -p ~/.config/ccache \
    && echo "max_size = 10G" > ~/.config/ccache/ccache.conf \
    && echo "compiler_check = content" >> ~/.config/ccache/ccache.conf

WORKDIR /workspaces/arcanum
