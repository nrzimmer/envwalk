add-zsh-hook -d preexec envwalk_exec
add-zsh-hook -d chpwd envwalk_chpwd

typeset -g ENVWALK_PREV_PWD="$PWD"
envwalk_chpwd() {
    eval "$(%s cd \"$ENVWALK_PREV_PWD\")"
    ENVWALK_PREV_PWD="$PWD"
}
add-zsh-hook chpwd envwalk_chpwd

autoload -Uz add-zsh-hook
envwalk_exec() {
    ENVWALK_PREV_PWD="$PWD"
    local cmd="$1"
    local base=${cmd%% *}
    local -a skip_cmds=(zoxide cd)
    if (( ${skip_cmds[(Ie)$base]} )); then
        return
    fi
    eval "$(%s)"
}
add-zsh-hook preexec envwalk_exec
