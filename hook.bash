__envwalk_preexec_running=0
ENVWALK_PREV_PWD="$PWD"

_envwalk_preexec() {
    ENVWALK_PREV_PWD="$PWD"
    local cmd="$BASH_COMMAND"
    local base="${cmd%% *}"

    case "$base" in
        zoxide|cd)
        return
        ;;
    esac

    eval "$(%s)"
}

_envwalk_chpwd() {
    eval "$(%s cd \"$ENVWALK_PREV_PWD\")"
    ENVWALK_PREV_PWD="$PWD"
}

__envwalk_hook_preexec() {
    if [[ __envwalk_preexec_running -eq 0 ]]; then
        _envwalk_preexec
        __envwalk_preexec_running=1
    fi
}

__envwalk_hook_precmd() {
    __envwalk_preexec_running=0
    if [[ "$PWD" != "$ENVWALK_PREV_PWD" ]]; then
        _envwalk_chpwd
    fi
}

trap '__envwalk_hook_preexec' DEBUG
PROMPT_COMMAND="__envwalk_hook_precmd${PROMPT_COMMAND:+; $PROMPT_COMMAND}"
