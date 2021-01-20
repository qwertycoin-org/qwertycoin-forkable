
SET TESTNET_PATH=%USERPROFILE%\Desktop\QwertyFork\Testnet

start cmd /k "%TESTNET_PATH%\simplewallet.exe --daemon-port 58081 --wallet-file %TESTNET_PATH%\wallet_01.bin --password "" --log-file %TESTNET_PATH%\wallet_01.log"

start cmd /k "%TESTNET_PATH%\simplewallet.exe --daemon-port 38081 --wallet-file %TESTNET_PATH%\wallet_02.bin --password "" --log-file %TESTNET_PATH%\wallet_02.log"