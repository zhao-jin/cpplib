while true
do
echo new
date +%M-%S-%N
mv test.tar /data/hudson
curl http://10.6.222.88:8080/view/PreSubmit/job/common-pre/build?token=test
date +%M-%S-%N
sleep 2
done
