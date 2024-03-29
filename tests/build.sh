#!/bin/bash
#
TIMEFORMAT=%3lR
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
pushd $SCRIPTPATH > /dev/null

if [[ -z "${FTB_NO_SIMD_TESTS}" ]]; then
    CLANG_DEFS=""
else
    echo "skipping tests including SIMD insructions"
    CLANG_DEFS="-DFTB_NO_SIMD_TESTS"
fi

#  _DEBUG
# time g++ -fpermissive src/main.cpp -g -o ./bin/slime --std=c++17 || exit 1
time clang++ -fsanitize=undefined -rdynamic $CLANG_DEFS -D_DEBUG -D_PROFILING -fpermissive main.cpp -gdwarf-4 -o ./ftb --std=c++17 || exit 1
# time clang++ -D_DEBUG -D_PROFILING -fpermissive cpu_info.cpp -g -o ./cpu_info --std=c++17 || exit 1

echo ""
# time valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all ./ftb
valgrind ./ftb
# time ./ftb || exit 1
# time ./cpu_info

popd > /dev/null
unset TIMEFORMAT
