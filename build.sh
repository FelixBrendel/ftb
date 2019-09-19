TIMEFORMAT=%3lR
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
pushd $SCRIPTPATH > /dev/null

#  _DEBUG
# time g++ -fpermissive src/main.cpp -g -o ./bin/slime --std=c++17 || exit 1
time clang++ -D_DEBUG -D_PROFILING -fpermissive test.cpp -g -o ./ftb --std=c++17 || exit 1

echo ""
time ./ftb

popd > /dev/null
unset TIMEFORMAT
