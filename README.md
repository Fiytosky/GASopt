# **AsmXoM**

AsmXoM is an enhanced compilation toolchain designed to enforce strict code–data separation at the assembly level. It aims to render x86-64 programs compatible with Execute-Only Memory (XoM) defense mechanisms through this strict isolation. The AsmXoM prototype is implemented based on  GCC 15.2.0 and a modified version of GNU Binutils 2.44.

### How to Use

### Prerequisites

- An x86-64 CPU supporting Intel Memory Protection Keys (MPK)
- A minimum of 20 GB of available disk space for building GCC and GNU Binutils

#### Building AsmXoM Locally

Compile binutils-2.44 and gcc-15.2.0 in your working directory (e.g., `Workplace`).

```bash
# 1. compile binutils-2.44 after downloading the source code
../configure --prefix=/Workplace/binutils-2.44/build

# 2. compile gcc-15.2.0 using bintuils-2.44
wget https://ftp.gnu.org/pub/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.gz
../configure --prefix=/Workplace/gcc-15.2.0/build --enable-languages=c,c++ --with-as=/Workplace/binutils-2.44/build/bin/as --with-ld=/Workplace/binutils-2.44/build/bin/ld --disable-multilib --disable-nls
```

#### Using GAS

The modified `GAS` assembler provides two compilation flags to enable the customized modules:

- `--dcollect` collect all embedded data information within the program.
- `--dsplit` separate embedded data into the read-only data section.   

```bash
# 1. collect embedded data information
/Workplace/binutils-2.44/build/bin/as --keep-locals --dcollect test.s -o test.o

# 2. dump collected information
/Workplace/binutils-2.44/datadump/datadump test.o --section xom --log /tmp/dump.log

# 3. separate embedded data
/Workplace/binutils-2.44/build/bin/as --keep-locals --dsplit test.s -o test.o
```

##### Output of dump.log

```yaml
...
--- [ Header ] ---
Magic      : xom
Name Size  : 22 bytes
File Name  : crypto/bn/rsaz-avx2.s

hardcoded_bytes_per_file  : 235  bytes
bytes_frag_num  : 15
direct_code_num  : 2
direct_data_num  : 2
fall_through_num : 6
--- [ Metadata Entries ] ---
[Entry #0]
  Frag Address : 0x0000000000529d80
  Frag Index   : 44
  Frag Size    : 32 (0x20) bytes
  Frag Flags   : 0x00000000
  Symbol Size  : 11 bytes
  Symbol Name  : .Land_mask
----------------------------------------
--- [ Metadata Entries ] ---
[Entry #1]
  Frag Address : 0x0000000000529da0
  Frag Index   : 45
  Frag Size    : 32 (0x20) bytes
  Frag Flags   : 0x00000000
  Symbol Size  : 16 bytes
  Symbol Name  : .Lscatter_permd
...
```

#### Using GCC

You can pass custom flags to the assembler invoked by GCC using the `-Wa` option.   

```bash
/Workplace/gcc-15.2.0/build/bin/gcc -Wa,--keep-locals -Wa,--dcollect test.c -o test
/Workplace/gcc-15.2.0/build/bin/gcc -Wa,--keep-locals -Wa,--dsplit test.c -o test
```

### How to Evaluation

We use `OpenSSL` as an example to demonstrate the functionality of AsmXoM and its XOM compatibility.

#### Functional Evaluation

```bash
# 1. download OpenSSL from https://openssl-library.org/source/
wget https://github.com/openssl/openssl/releases/download/openssl-3.3.5/openssl-3.3.5.tar.gz
tar -xf openssl-3.3.5.tar.gz

# 2. Configure
export CUSTOM_CC=/your_path/gcc
export EXTRA_CFLAGS="-Wa,--keep-locals -Wa,--dsplit"
export EXTRA_LDFLAGS="-Wl,-T,/Workplace/binutils-2.44/xom-tools/ldscript.ld"

# 3. Compile OpenSSL using AsmXoM
cd openssl-3.3.5
mkdir build && cd build
../Configure no-shared --prefix="$PWD" CC="$CUSTOM_CC" CFLAGS="-O3 -fPIC $EXTRA_CFLAGS" LDFLAGS="$EXTRA_LDFLAGS"

make -j($nproc)
make install # openssl will be installed in ./build/apps

# 4. Test (Proxies cause certain network tests to fail.)
env -u http_proxy -u HTTP_PROXY -u https_proxy -u HTTPS_PROXY -u all_proxy -u ALL_PROXY \
make test

# expected output
All tests successful.
Files=316, Tests=3244, 417 wallclock secs ( 7.00 usr  0.73 sys + 328.01 cusr 90.40 csys = 426.14 CPU)
Result: PASS
```

#### XoM compatibility Evaluation

We enforce XoM protection on the program via Intel MPK, and compile and execute OpenSSL using AsmXoM and the baseline GCC, respectively.   

##### Compilation and execution via AsmXoM

```bash
# 1. using sha1 as an example
MPK_XOM_SCOPE=main MPK_XOM_VERBOSE=0 /root/xom/test/tools/speccpu_test/xom_test/xom_run ./apps/openssl speed sha1

# expected output
compiler: /AsmXoM/gcc-15.2.0/build/bin/gcc -fPIC -pthread -m64 -Wa,--noexecstack -O3 -Wa,--keep-locals -Wa,--fiy-dsplit -fPIC -DOPENSSL_USE_NODELETE -DL_ENDIAN -DOPENSSL_PIC -DOPENSSL_BUILDING_OPENSSL -DNDEBUG
CPUINFO: OPENSSL_ia32cap=0x7ffef3ffffebffff:0x40417f5ef3bfb7ef
The 'numbers' are in 1000s of bytes per second processed.
type             16 bytes     64 bytes    256 bytes   1024 bytes   8192 bytes  16384 bytes
sha1             93599.05k   302036.20k   736971.95k  1155202.05k  1393827.84k  1425184.09k
```

##### Compilation and execution via baseline GCC   

```bash
# 1. using sha1 as an example
MPK_XOM_SCOPE=main MPK_XOM_VERBOSE=0 /Workplace/binutils-2.44/xom-tools/xom_run ./apps/openssl speed sha1

# expected output (The coredump log records the embedded data access location.)
Doing sha1 ops for 3s on 16 size blocks: Segmentation fault (core dumped)
```

