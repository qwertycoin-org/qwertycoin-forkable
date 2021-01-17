
SET TESTNET_PATH=%USERPROFILE%\Desktop\QwertyFork\Testnet

:: cd %TESTNET_PATH%

:: start node_01
start cmd /k "%TESTNET_PATH%\qwertyforkd.exe --data-dir %TESTNET_PATH%\node_01 --log-file %TESTNET_PATH%\qwc_node_01.log --p2p-bind-port 58080 --rpc-bind-port 58081  --restricted-rpc --enable-cors=*  --enable-blockchain-indexes --hide-my-port --p2p-bind-ip 127.0.0.1 --log-level 3 --add-exclusive-node 127.0.0.1:38080 --add-exclusive-node 127.0.0.1:48080"

:: start node_02
start cmd /k "%TESTNET_PATH%\qwertyforkd.exe --data-dir %TESTNET_PATH%\node_02 --log-file %TESTNET_PATH%\qwc_node_02.log --p2p-bind-port 38080 --rpc-bind-port 38081 --hide-my-port --log-level 3 --p2p-bind-ip 127.0.0.1 --add-exclusive-node 127.0.0.1:58080 --add-exclusive-node 127.0.0.1:48080"

:: start node_03
start cmd /k "%TESTNET_PATH%\qwertyforkd.exe --data-dir %TESTNET_PATH%\node_03 --log-file %TESTNET_PATH%\qwc_node_03.log --p2p-bind-port 48080 --rpc-bind-port 48081 --hide-my-port --log-level 3 --p2p-bind-ip 127.0.0.1 --add-exclusive-node 127.0.0.1:58080 --add-exclusive-node 127.0.0.1:38080"
