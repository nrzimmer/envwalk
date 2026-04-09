__zenv_preexec_running=0
ZENV_PREV_PWD="$PWD"

_zenv_preexec() {
    ZENV_PREV_PWD="$PWD"
    local cmd="$BASH_COMMAND"
    local base="${cmd%% *}"

    case "$base" in
        zoxide|cd)
        return
        ;;
    esac

    eval "$(%s)"
}

_zenv_chpwd() {
    eval "$(%s cd \"$ZENV_PREV_PWD\")"
    ZENV_PREV_PWD="$PWD"
}

__zenv_hook_preexec() {
    if [[ __zenv_preexec_running -eq 0 ]]; then
        _zenv_preexec
        __zenv_preexec_running=1
    fi
}

__zenv_hook_precmd() {
    __zenv_preexec_running=0
    if [[ "$PWD" != "$ZENV_PREV_PWD" ]]; then
        _zenv_chpwd
    fi
}

trap '__zenv_hook_preexec' DEBUG
PROMPT_COMMAND="__zenv_hook_precmd${PROMPT_COMMAND:+; $PROMPT_COMMAND}"
