
#
# Commands used for createing private testnet nodes and wallets
#

# Directories to use
TESTNET_PATH=/media/exploshot/Data/Blockchains/QwertyNote/Testnet/
BIN_PATH=/media/exploshot/Cloud/ExploCloud/Development/Playground/qwertycoin-forkable/cmake-build-debug

# remove old installation
rm -rf $TESTNET_PATH

# create testnet directory
mkdir $TESTNET_PATH
mkdir $TESTNET_PATH/node_03
mkdir $TESTNET_PATH/node_02
mkdir $TESTNET_PATH/node_01

# copy binaries to testnet directory
cp -fv $BIN_PATH/src/qwertycoind  $TESTNET_PATH
cp -fv $BIN_PATH/src/miner $TESTNET_PATH
cp -fv $BIN_PATH/src/simplewallet $TESTNET_PATH
cp -fv $BIN_PATH/src/walletd $TESTNET_PATH

cd $TESTNET_PATH

# generate private wallets

#QvLU1uD686HdkUDqwdCLyGShXpEfYgTJHhei27aeW5a7axtZWWtmcU9iMShKzbv8TVSuSPArsAYK3WAZia59XrEA2uSzuLTv5
./simplewallet --wallet-file=wallet_01.bin  --restore-deterministic-wallet --mnemonic-seed="sequence atlas unveil summon pebbles tuesday beer rudely snake rockets different fuselage woven tagged bested dented vegan hover rapid fawns obvious muppet randomly seasons randomly" --password "" --log-file wallet_01.log

#QvLSkFgnLkJ89s7qCQVdmY5YcWVLk1TrnYMgeLP21c18PAvVFpXf8B825bjoUNhBeXZax87RSQe4sdVPuP1tmDwk2r8EFWZRh
./simplewallet --wallet-file=wallet_02.bin  --restore-deterministic-wallet --mnemonic-seed="deftly large tirade gumball android leech sidekick opened iguana voice gels focus poaching itches network espionage much jailed vaults winter oatmeal eleven science siren winter" --password "" --log-file wallet_02.log

