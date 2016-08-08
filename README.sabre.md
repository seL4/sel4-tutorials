# Install qemu with sabrelite support

### Prerequisites

```
sudo apt-get install pkg-config
sudo apt-get install libpixman-1-dev
sudo apt-get install zlib1g-dev
sudo apt-get install libfdt-dev
```

### Get QEMU source and build it

Instructions from: http://nairobi-embedded.org/qemu_system_arm_howto.html

```
git clone git://git.qemu.org/qemu.git
export QEMU_SRC=`pwd`/qemu
mkdir {qtemp,qinstall}
cd qtemp
${QEMU_SRC}/configure --target-list=arm-softmmu --prefix=${PWD}/../qinstall
make
make install
export PATH=`pwd`/qinstall/bin:$PATH
```
