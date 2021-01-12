#!/bin/bash

TESTNET_PATH=/media/exploshot/Data/Blockchains/QwertyNote/Testnet/
# tmux session name
SN=PRIVQWC1

tmux kill-session -t $SN

cd $TESTNET_PATH

# wallets window

# start wallet_01 (first pane in new window)
tmux new-session -d -s $SN -n wallets -- sh -ic './simplewallet --daemon-port 58081 --wallet-file wallet_01.bin --password "" --log-file wallet_01.log || read WHATEVER'

# start wallet_02
tmux split-window -dv
tmux select-pane -t 1
tmux send './simplewallet --daemon-port 38081 --wallet-file wallet_02.bin --password "" --log-file wallet_02.log || read WHATEVER' ENTER


# open second (wallets) tmux window
tmux select-window -t 1

# open tmux for this session
tmux a -t $SN