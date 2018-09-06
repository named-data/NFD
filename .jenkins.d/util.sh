has() {
    local saved_xtrace
    [[ $- == *x* ]] && saved_xtrace=-x || saved_xtrace=+x
    set +x

    local p=$1
    shift
    local i ret=1
    for i in "$@"; do
        if [[ "${i}" == "${p}" ]]; then
            ret=0
            break
        fi
    done

    set ${saved_xtrace}
    return ${ret}
}

sudo_preserve_env() {
    local saved_xtrace
    [[ $- == *x* ]] && saved_xtrace=-x || saved_xtrace=+x
    set +x

    local vars=()
    while [[ $# -gt 0 ]]; do
        local arg=$1
        shift
        case ${arg} in
            --) break ;;
            *)  vars+=("${arg}=${!arg}") ;;
        esac
    done

    set ${saved_xtrace}
    sudo env "${vars[@]}" "$@"
}
