#!/usr/bin/csh 

tar -cf regsetBackup fTest* load_unload* buildTest*

echo Processing fTest
./newStars.exe -p fTest0 >tmpRegress
mv tmpRegress fTest0
./newStars.exe -g fTest0 >fTest1
./newStars.exe -g fTest1 >fTest2
./newStars.exe -g fTest2 >fTest3
./newStars.exe -g fTest3 >fTest4

echo Processing load_unload
./newStars.exe -p load_unload_test_0 >tmpRegress
mv tmpRegress load_unload_test_0
./newStars.exe -g load_unload_test_0 >load_unload_test_1
./newStars.exe -g load_unload_test_1 >load_unload_test_2
./newStars.exe -g load_unload_test_2 >load_unload_test_3
./newStars.exe -g load_unload_test_3 >load_unload_test_4

echo Processing buildTest
./newStars.exe -p buildTest0.xml >tmpRegress
mv tmpRegress buildTest0.xml
./newStars.exe -g buildTest0.xml > buildTest1.xml
./newStars.exe -g buildTest1.xml > buildTest2.xml
./newStars.exe -g buildTest2.xml > buildTest3.xml
./newStars.exe -g buildTest3.xml > buildTest4.xml
./newStars.exe -g buildTest4.xml > buildTest5.xml

echo Processing shipBuildTest
./newStars.exe -p shipBuildTest0.xml > tmpRegress
mv tmpRegress shipBuildTest0.xml
./newStars.exe -g shipBuildTest0.xml > shipBuildTest1.xml
./newStars.exe -g shipBuildTest1.xml > shipBuildTest2.xml

echo Processing followTest
./newStars.exe -p followTest0.xml >tmpRegress
mv tmpRegress followTest0.xml
./newStars.exe -g followTest0.xml >followTest1.xml
./newStars.exe -g followTest1.xml >followTest2.xml

echo Processing colonizeTest
./newStars.exe -p colonizeTest0.xml >tmpRegress
mv tmpRegress colonizeTest0.xml
./newStars.exe -g colonizeTest0.xml >colonizeTest1.xml

echo Processing shipBuildTestFail
./newStars.exe -p shipBuildTestFail0.xml > tmpRegress
mv tmpRegress shipBuildTestFail0.xml
./newStars.exe -g shipBuildTestFail0.xml > shipBuildTestFail1.xml

echo Processing battleTest
./newStars.exe -p battleTest0_0.xml > tmpRegress
mv tmpRegress battleTest0_0.xml
./newStars.exe -g battleTest0_0.xml > battleTest0_1.xml
./newStars.exe -p battleTest1_0.xml > tmpRegress
mv tmpRegress battleTest1_0.xml
./newStars.exe -g battleTest1_0.xml > battleTest1_1.xml
./newStars.exe -p battleTest2_0.xml > tmpRegress
mv tmpRegress battleTest2_0.xml
./newStars.exe -g battleTest2_0.xml > battleTest2_1.xml
./newStars.exe -p battleTest3_0.xml > tmpRegress
mv tmpRegress battleTest3_0.xml
./newStars.exe -g battleTest3_0.xml > battleTest3_1.xml
./newStars.exe -p battleTest4_0.xml > tmpRegress
mv tmpRegress battleTest4_0.xml
./newStars.exe -g battleTest4_0.xml > battleTest4_1.xml


echo Processing minelayingTest
./newStars.exe -p minelayingTest1_0.xml >tmpRegress
mv tmpRegress minelayingTest1_0.xml
./newStars.exe -p minelayingTest2_0.xml >tmpRegress
mv tmpRegress minelayingTest2_0.xml
./newStars.exe -p minelayingTest3_0.xml >tmpRegress
mv tmpRegress minelayingTest3_0.xml
./newStars.exe -g minelayingTest1_0.xml > minelayingTest1_1.xml
./newStars.exe -g minelayingTest2_0.xml >minelayingTest2_1.xml
./newStars.exe -g minelayingTest3_0.xml >minelayingTest3_1.xml
./newStars.exe -g minelayingTest3_1.xml >minelayingTest3_2.xml

echo Processing bombingTest
./newStars.exe -p bombingTest1_0.xml >tmpRegress
mv tmpRegress bombingTest1_0.xml
./newStars.exe -g bombingTest1_0.xml >bombingTest1_1.xml
./newStars.exe -g bombingTest1_1.xml >bombingTest1_2.xml
