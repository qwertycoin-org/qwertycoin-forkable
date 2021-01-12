#!/bin/bash

TESTNET_PATH=/media/exploshot/Data/Blockchains/QwertyNote/Testnet/
# tmux session name
SN=PRIVQWC

tmux kill-session -t $SN

cd $TESTNET_PATH

# nodes window
# start node_01 (initial session)
tmux new-session -d -s $SN -n nodes -- sh -ic './qwertycoind --data-dir node_01 --log-file qwc_node_01.log --p2p-bind-port 58080 --rpc-bind-port 58081  --restricted-rpc --enable-cors=*  --enable-blockchain-indexes --hide-my-port --p2p-bind-ip 127.0.0.1 --log-level 3 --add-exclusive-node 127.0.0.1:38080 --add-exclusive-node 127.0.0.1:48080 || read WHATEVER'

# start node_02
tmux split-window -dv
tmux select-pane -t 1
tmux split-window -dv
tmux send './qwertycoind --data-dir node_02 --log-file qwc_node_02.log --p2p-bind-port 38080 --rpc-bind-port 38081 --hide-my-port --log-level 3 --p2p-bind-ip 127.0.0.1 --add-exclusive-node 127.0.0.1:58080 --add-exclusive-node 127.0.0.1:48080 || read WHATEVER' ENTER

# start node_03
tmux select-pane -t 2
tmux send './qwertycoind --data-dir node_03 --log-file qwc_node_03.log --p2p-bind-port 48080 --rpc-bind-port 48081 --hide-my-port --log-level 3 --p2p-bind-ip 127.0.0.1 --add-exclusive-node 127.0.0.1:58080 --add-exclusive-node 127.0.0.1:38080 || read WHATEVER' ENTER

# konsole -e '/bin/bash /media/exploshot/Cloud/ExploCloud/Development/Playground/qwertycoin-forkable/scripts/wallets_testnet.sh'

tmux a -t $SN
