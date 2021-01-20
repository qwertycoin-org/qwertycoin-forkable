#!/bin/bash

#
# Commands used for creating private testnet nodes and wallets
#

# Directories to use
TESTNET_PATH=
BIN_PATH=

# remove old installation
rm -rf $TESTNET_PATH

# create testnet directory
mkdir $TESTNET_PATH
mkdir $TESTNET_PATH/node_03
mkdir $TESTNET_PATH/node_02
mkdir $TESTNET_PATH/node_01

# copy binaries to testnet directory
cp -fv $BIN_PATH/src/qwertyforkd  $TESTNET_PATH
cp -fv $BIN_PATH/src/simplewallet $TESTNET_PATH
cp -fv $BIN_PATH/src/walletd $TESTNET_PATH

cd $TESTNET_PATH

# generate private wallets

#QWFi5gyjFB2XaxLNbDLLWi6xNEzzBJ2He5WSf7He8peuPt4nTyakAFyNuXqrHAGQt1PBSBonCRRj8daUtF7TPXFW42YQk4SpSh
./simplewallet --wallet-file=wallet_01.bin  --restore-deterministic-wallet --mnemonic-seed="sequence atlas unveil summon pebbles tuesday beer rudely snake rockets different fuselage woven tagged bested dented vegan hover rapid fawns obvious muppet randomly seasons randomly" --password "" --log-file wallet_01.log
$TESTNET_PATH/simplewallet --wallet-file=wallet_01.bin  --restore-deterministic-wallet --mnemonic-seed="sequence atlas unveil summon pebbles tuesday beer rudely snake rockets different fuselage woven tagged bested dented vegan hover rapid fawns obvious muppet randomly seasons randomly" --password "" --log-file wallet_01.log

#QWFi5QHxmajhg2SxRS1FhjcyoM2FTYxmSjmvaBr7uXzz1b8Ms4M2h8PiKTL2YswTz1dFrV8ry5UQmBBLrb27tecs7MuNbLGuy5
./simplewallet --wallet-file=wallet_02.bin  --restore-deterministic-wallet --mnemonic-seed="deftly large tirade gumball android leech sidekick opened iguana voice gels focus poaching itches network espionage much jailed vaults winter oatmeal eleven science siren winter" --password "" --log-file wallet_02.log
$TESTNET_PATH/simplewallet --wallet-file=wallet_02.bin  --restore-deterministic-wallet --mnemonic-seed="deftly large tirade gumball android leech sidekick opened iguana voice gels focus poaching itches network espionage much jailed vaults winter oatmeal eleven science siren winter" --password "" --log-file wallet_02.log

