has() {
    local p=$1
    shift
    local x
    for x in "$@"; do
        [[ "${x}" == "${p}" ]] && return 0
    done
    return 1
}
