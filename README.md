# Experiment Notes for Quantum SSH

## Setup

We assume that the experiment directory is `~/pq-ssh/`. Otherwise you must first adjust `ssh` and `sshd` configuration files.
This directory contains the folders `sntrup761-config/`, `sntrup761-cpa-config/`, `mlkem768-config`, `mlkem768-cpa-config`, `data/`, `patches/`, and the script `analyse.py`.

### Dependencies

Make sure your system has the following programs:

* `git` to download software and apply patches, alternatively `patch`,
* `autoconf`, `make`,
* OpenSSH dependencies: a C compiler, standard library and headers, and `libcrypto` from either LibreSSL or OpenSSL,
* `tshark` (i.e. `wireshark-cli`) - make sure your user is in the `wireshark` group,
* a Python 3 interpreter.

### Get liboqs

Checkout the code with `git`:
```bash
git clone https://github.com/open-quantum-safe/liboqs.git
cp -r liboqs liboqs-cpa
```

There should now be two directories `liboqs/` and `liboqs-cpa/` with identical contents.
In `liboqs`, build and install the library:
```bash
cd liboqs
dir=$(pwd)
mkdir build install
cd build
cmake -GNinja -DCMAKE_POSITION_INDEPENDENT_CODE=yes -DCMAKE_INSTALL_PREFIX=$dir/install ..
ninja
ninja install
```

In `liboqs-cpa`, first patch the library, then build and install:
```bash
cd liboqs-cpa
git apply ../patches/liboqs-mlkem768-cpa.patch
dir=$(pwd)
mkdir build install
cd build
cmake -GNinja -DCMAKE_POSITION_INDEPENDENT_CODE=yes -DCMAKE_INSTALL_PREFIX=$dir/install ..
ninja
ninja install
```

### Get OpenSSH

You can either checkout the source code with `git`, or get a release version. We cover the `git` approach, the release version is analogous.

Run
```bash
git clone https://github.com/openssh/openssh-portable
cp -r openssh-portable openssh-portable-sntrup761
cp -r openssh-portable openssh-portable-sntrup761-cpa
cp -r openssh-portable openssh-portable-mlkem768
cp -r openssh-portable openssh-portable-mlkem768-cpa
```
There should now be four directories `openssh-portable{sntrup761,sntrup761-cpa,mlkem768,mlkem768-cpa}/` with identical contents.

In `openssh-portable-sntrup761/`, build and install version `9.9p1` of the binaries:
```bash
cd openssh-portable-sntrup761
git checkout V_9_9_P1
mkdir build

autoreconf
./configure --prefix=$PWD/build
make -j
make install
```

In `openssh-portable-sntrup761-cpa/`, do the same but apply the patch `sntrup761-cpa.patch` before building:
```bash
cd openssh-portable-sntrup761-cpa
git checkout V_9_9_P1
mkdir build

git apply ../patches/sntrup761-cpa.patch

autoreconf
./configure --prefix=$PWD/build
make -j
make install
```

In the other two directories, we will also install `liboqs`.
In `openssh-portable-mlkem768`, build and install version `9.9p1` of the binaries linked against `liboqs`.
```bash
cd openssh-portable-mlkem768
git checkout V_9_9_P1
mkdir build

autoreconf
./configure --prefix=$PWD/build

cp -r ../liboqs/install liboqs
git apply ../patches/kexmlkem768x25519-liboqs.patch
git apply ../patches/Makefile-liboqs.patch

make -j
make install
```

In `openssh-portable-mlkem768-cpa`, build and install version `9.9p1` of the binaries linked against the patches `liboqs`.
```bash
cd openssh-portable-mlkem768
git checkout V_9_9_P1
mkdir build

autoreconf
./configure --prefix=$PWD/build

cp -r ../liboqs-cpa/install liboqs
git apply ../patches/kexmlkem768x25519-liboqs.patch
git apply ../patches/Makefile-liboqs.patch
cp ../patches/crypto_api-cpa.h crypto_api.h

make -j
make install
```

### Install Configuration Files

Back in the base directory, copy the configuration files to each installation of OpenSSH.
```bash
cp sntrup761-config/* openssh-portable-sntrup761/build/etc/
cp sntrup761-cpa-config/* openssh-portable-sntrup761-cpa/build/etc/
cp mlkem768-config/* openssh-portable-mlkem768/build/etc/
cp mlkem768-cpa-config/* openssh-portable-mlkem768-cpa/build/etc/
```

## Network Timing Experiment

We quantify the improvement by timing network packets of a simple SSH exchange. The test is performed on `localhost` to minimise sample variance due to network latency.

For `[SUF]` in `{sntrup761,sntrup761-cpa,mlkem768,mlkem768-cpa}/` do the following.

Navigate to the folder `openssh-portable-[SUF]/build/` and open 3 terminal windows.
In one, run the server:
```bash
$PWD/sbin/sshd -D -e -f $PWD/etc/sshd_config
```

In another, listen on port `2222`:
```bash
tshark -i any -f "port 2222" -w [SUF].pcap
```

In the last one, run the client many times, e.g. here we perform `10 000` experiments:
```bash
for i in `seq 10000`; do $PWD/bin/ssh -F $PWD/etc/ssh_config test true; sleep 0.1; done
```

After the loop has finished, go to the listener window and stop it with `Ctrl-C`.
Extract the timings into a `csv` file:
```bash
tshark -r [SUF].pcap -T fields -e frame.time_relative -e tcp.completeness.syn -E separator=, -E quote=n > ../../data/[SUF].csv
```
The `.csv` extension is important.

Lastly, back in the base directory, analyse:
```bash
./analyse.py data/*.csv
```

If `CPA_MEAN` is the mean value reported by analysing `{sntrup761,mlkem768}-cpa.csv`, and `CCA_MEAN` is the mean value reported by analysing `{sntrup761,mlkem768}.csv`, the improvement is computed as
```python
1 - CPA_MEAN / CCA_MEAN
```

## KEM and KEX CPU Clock Experiment

We quantify the improvement by recording CPU timings of the base KEM without the network. Namely, we are timing the successive execution of `KGen`, `Encaps`, and `Decaps`.
Analogously, we record CPU timings of the hybrid KEX without the network.

### Install the Test Programs

The test program must first be installed and compiled.

Install the source into relevant OpenSSH directories:
```bash
cp patches/test_sntrup761.c patches/test_sntrup761x25519.c openssh-portable-sntrup761
cp patches/test_sntrup761.c patches/test_sntrup761x25519.c openssh-portable-sntrup761-cpa
cp patches/test_mlkem768.c patches/test_mlkem768x25519.c openssh-portable-mlkem768
cp patches/test_mlkem768.c patches/test_mlkem768x25519.c openssh-portable-mlkem768-cpa
```

In all OpenSSH directories, modify the `Makefile` by running:
```bash
cat patches/Makefile.sntrup761 >> openssh-portable-sntrup761/Makefile
cat patches/Makefile.sntrup761 >> openssh-portable-sntrup761-cpa/Makefile
cat patches/Makefile.mlkem768 >> openssh-portable-mlkem768/Makefile
cat patches/Makefile.mlkem768 >> openssh-portable-mlkem768-cpa/Makefile
```

Then compile the tests and link them against OpenSSH in all OpenSSH directories.
In `openssh-portable-sntrup761[-cpa]/` run:
```bash
make clean
make -j test_sntrup761
make -j test_sntrup761x25519
```
and int `openssh-portable-mlkem768[-cpa]` run:
```bash
make clean
make -j test_mlkem768
make -j test_mlkem768x25519
```

### Run the Experiment

Here we collect `10 000` timings.
For `[SUF]` in `{sntrup761,sntrup761-cpa}` navigate to `openssh-portable-[SUF]` and run:
```bash
./test_sntrup761 10000 > ../data/[SUF]-kem.times
./test_sntrup761x25519 10000 > ../data/[SUF]-kex.times
```

Repeat for `[SUF]` in `{mlkem768,mlkem768-cpa}`:
```bash
./test_mlkem768 10000 > ../data/[SUF]-kem.times
./test_mlkem768x25519 10000 > ../data/[SUF]-kex.times
```
The `.times` extention is important.

Lastly, back in the base directory, we analyse the KEM data and the KEX data:
```bash
./analyse.py data/*-kem.times
./analyse.py data/*-kex.times
```
The improvement is computed like in the network timing experiment.

## Some Data

In the `data/` directory, there is the collected experiment data presented in the paper. To analyse it, run `analyse.py` on it like in the end of every experiment.
