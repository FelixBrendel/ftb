TIMEFORMAT=%3lR
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
pushd $SCRIPTPATH > /dev/null

#  _DEBUG
# time g++ -fpermissive src/main.cpp -g -o ./bin/slime --std=c++17 || exit 1
time clang++ -rdynamic -D_DEBUG -D_PROFILING -fpermissive main.cpp -g -o ./ftb --std=c++17 || exit 1
# time clang++ -D_DEBUG -D_PROFILING -fpermissive cpu_info.cpp -g -o ./cpu_info --std=c++17 || exit 1

echo ""
# time valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all ./ftb
time ./ftb
# time ./cpu_info

popd > /dev/null
unset TIMEFORMAT
