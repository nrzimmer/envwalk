add-zsh-hook -d preexec zenv_exec
add-zsh-hook -d chpwd zenv_chpwd

typeset -g ZENV_PREV_PWD="$PWD"
zenv_chpwd() {
    eval "$(%s cd \"$ZENV_PREV_PWD\")"
    ZENV_PREV_PWD="$PWD"
}
add-zsh-hook chpwd zenv_chpwd

autoload -Uz add-zsh-hook
zenv_exec() {
    ZENV_PREV_PWD="$PWD"
    local cmd="$1"
    local base=${cmd%% *}
    local -a skip_cmds=(zoxide cd)
    if (( ${skip_cmds[(Ie)$base]} )); then
        return
    fi
    eval "$(%s)"
}
add-zsh-hook preexec zenv_exec
