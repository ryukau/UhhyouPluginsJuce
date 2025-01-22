cl.exe /EHsc /std:c++20 /O2 .\test.cpp
test.exe | Out-File -FilePath .\test.json -Encoding 'utf8'
python compare.py
