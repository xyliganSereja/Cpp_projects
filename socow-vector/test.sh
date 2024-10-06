arr=(Release SanitizedDebug RelWithDebInfo Debug)

for MOD in ${arr[@]}
do
    ./ci-extra/build.sh $MOD || exit 1
done
for MOD in ${arr[@]}
do
        './cmake-build-'$MOD'/tests' || exit 1
done
