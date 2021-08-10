#!/bin/bash

WORKDIR=target/m68k-amigaos/dist

rm -rf ${WORKDIR}/aqb
rm -rf ${WORKDIR}/aqb.lha
mkdir -p ${WORKDIR}/aqb/lib
mkdir -p ${WORKDIR}/aqb/examples
mkdir -p ${WORKDIR}/aqb/examples/bench
mkdir -p ${WORKDIR}/aqb/examples/demo

cp target/m68k-amigaos/bin/aqb ${WORKDIR}/aqb/
cp src/lib/_brt/_brt.sym ${WORKDIR}/aqb/lib/
cp src/lib/_brt/_brt.a   ${WORKDIR}/aqb/lib/
cp src/lib/_aqb/_aqb.sym ${WORKDIR}/aqb/lib/
cp src/lib/_aqb/_aqb.a   ${WORKDIR}/aqb/lib/
cp src/lib/startup.o     ${WORKDIR}/aqb/lib/
cp README.md             ${WORKDIR}/aqb/README

cp dist/amiga/Icons/aqb_topdir.info ${WORKDIR}/aqb.info
cp dist/amiga/Icons/aqb.info ${WORKDIR}/aqb/aqb.info
cp dist/amiga/Icons/examples.info ${WORKDIR}/aqb/examples.info
cp dist/amiga/Icons/bench.info ${WORKDIR}/aqb/examples/bench.info
cp dist/amiga/Icons/demo.info ${WORKDIR}/aqb/examples/demo.info
cp dist/amiga/Icons/README.info ${WORKDIR}/aqb/README.info

for EX in examples/bench/*.bas ; do
    cp $EX ${WORKDIR}/aqb/examples/bench/
    cp dist/amiga/Icons/`basename $EX`.info ${WORKDIR}/aqb/examples/bench/
done

for EX in examples/demo/*.bas ; do
    cp $EX ${WORKDIR}/aqb/examples/demo/
    cp dist/amiga/Icons/`basename $EX`.info ${WORKDIR}/aqb/examples/demo/
done

pushd ${WORKDIR}
lha a aqb.lha aqb.info aqb
cp aqb.lha /home/guenter/media/emu/amiga/FS-UAE/hdd/system/x/
popd

#sudo cp -r Fonts/aqb /mnt/amiga/Fonts/
#sudo cp Fonts/aqb.font /mnt/amiga/Fonts/
