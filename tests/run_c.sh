#!/bin/bash
# Run C program which might be ran with binfmtc

# parameter parsing

SOURCEFILE="$1"
CMDOPTS=$(head -1 "${SOURCEFILE}" | sed -n 's,/[*]BINFMTC: ,,p')
DEFAULT=" -O2 -Wall "
TEMPFILE=$(tempfile -m 700)
shift

gcc ${DEFAULT} ${CMDOPTS} "${SOURCEFILE}" -o ${TEMPFILE}
${TEMPFILE} "$@"
RET=$?
rm -f ${TEMPFILE}
exit ${RET}