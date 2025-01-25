#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
USAGE_MGS="USAGE: testboy.sh <ROM_PATH> <TEST_SUITE_ID> [GEMUBOY_PATH]"

GEKKIO_SUCCESS_SEQ="358132134"
GEKKIO_FAILURE_SEQ="666666666666"

fd_re='^[0-9]+$'
cleanup_and_wait() {
    if [[ ${COPROC[1]} =~ $fd_re ]] ; then
        eval "exec ${COPROC[1]}<&-"
        echo "waiting for $filename to finish" >&2
        wait $COPROC_PID
    fi
}

rom_path=$1
test_suite_id=$2
gemuboy_path=$3
success_seq=""
failure_seq=""

if [ -z "$rom_path" ]; then
    echo $USAGE_MGS 
    exit 10
elif [ -z "$gemuboy_path" ]; then
    gemuboy_path="$SCRIPT_DIR/../build/gemuboy"
fi

case "$test_suite_id" in
    1)
        success_seq="$GEKKIO_SUCCESS_SEQ"
        failure_seq="$GEKKIO_FAILURE_SEQ"
        ;;
    *)
        echo $USAGE_MGS
        exit 11
        ;;
esac

res=""
exit_code=2

exec 3< <(timeout --preserve-status 60 $gemuboy_path $rom_path -l)

while IFS= read -n1 c; do
    res+="$c"

    if [ "$res" = "$success_seq" ]; then
        exit_code=0
        echo "SUCCESS"
        break
    elif [ "$res" = "$failure_seq" ]; then
        exit_code=1
        echo "FAILURE"
        break
    fi
done <&3
exec 3<&-

pkill -P $!

exit $exit_code
