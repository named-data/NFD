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
