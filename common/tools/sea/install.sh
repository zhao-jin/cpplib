./ut.sh auto/; echo $?
if [ $? -ne 0 ]; then
    echo "auto package unittest is not passed,please correct it before install!"
    exit 1
fi

./ut.sh com/; echo $?
if [ $? -ne 0 ]; then
    echo "com package unittest is not passed,please correct it before install!"
    exit 1
fi

pushd `pwd`
cd auto && python setup.py install && rm -rf build dist auto.egg-info
popd

pushd `pwd`
cd com && python setup.py install && rm -rf build dist com.egg-info
popd
