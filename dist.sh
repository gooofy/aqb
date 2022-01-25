#!/bin/bash

VERSION=0.8.2
WORKDIR=`pwd`/target/m68k-amigaos/dist
LHA=${WORKDIR}/aqb-${VERSION}.lha

rm -rf ${WORKDIR}/aqb
rm -rf ${WORKDIR}/aqb.lha
mkdir -p ${WORKDIR}/aqb/lib
mkdir -p ${WORKDIR}/aqb/examples
mkdir -p ${WORKDIR}/aqb/examples/bench
mkdir -p ${WORKDIR}/aqb/examples/demo
mkdir -p ${WORKDIR}/aqb/tutorial

cp target/m68k-amigaos/bin/aqb                   ${WORKDIR}/aqb/
cp src/lib/_brt/_brt.sym                         ${WORKDIR}/aqb/lib/
cp src/lib/_brt/_brt.a                           ${WORKDIR}/aqb/lib/
cp src/lib/_aqb/_aqb.sym                         ${WORKDIR}/aqb/lib/
cp src/lib/_aqb/_aqb.a                           ${WORKDIR}/aqb/lib/
cp src/lib/IFFSupport/IFFSupport.sym             ${WORKDIR}/aqb/lib/
cp src/lib/IFFSupport/IFFSupport.a               ${WORKDIR}/aqb/lib/
cp src/lib/AnimSupport/AnimSupport.sym           ${WORKDIR}/aqb/lib/
cp src/lib/AnimSupport/AnimSupport.a             ${WORKDIR}/aqb/lib/
cp src/lib/UISupport/UISupport.sym               ${WORKDIR}/aqb/lib/
cp src/lib/UISupport/UISupport.a                 ${WORKDIR}/aqb/lib/
cp src/lib/OSGadToolsConsts/OSGadToolsConsts.sym ${WORKDIR}/aqb/lib/
cp src/lib/startup.o                             ${WORKDIR}/aqb/lib/
cp README.guide                                  ${WORKDIR}/aqb/
cp CHANGELOG.md                                  ${WORKDIR}/aqb/CHANGELOG

cp dist/amiga/Icons/aqb_topdir.info              ${WORKDIR}/aqb.info
cp dist/amiga/Icons/aqb.info                     ${WORKDIR}/aqb/aqb.info
cp dist/amiga/Icons/examples.info                ${WORKDIR}/aqb/examples.info
cp dist/amiga/Icons/bench.info                   ${WORKDIR}/aqb/examples/bench.info
cp dist/amiga/Icons/demo.info                    ${WORKDIR}/aqb/examples/demo.info
cp dist/amiga/Icons/README.guide.info            ${WORKDIR}/aqb/
cp dist/amiga/Icons/CHANGELOG.info               ${WORKDIR}/aqb/CHANGELOG.info
cp dist/amiga/Icons/tutorial.info                ${WORKDIR}/aqb/

for EX in examples/bench/*.bas ; do
    cp $EX ${WORKDIR}/aqb/examples/bench/
    cp dist/amiga/Icons/`basename $EX`.info ${WORKDIR}/aqb/examples/bench/
done

for EX in examples/demo/*.bas ; do
    cp $EX ${WORKDIR}/aqb/examples/demo/
    cp dist/amiga/Icons/`basename $EX`.info ${WORKDIR}/aqb/examples/demo/
done

for EX in tutorial/*.bas ; do
    cp $EX ${WORKDIR}/aqb/tutorial/
    cp dist/amiga/Icons/`basename $EX`.info ${WORKDIR}/aqb/tutorial/
done
cp -r tutorial/imgs ${WORKDIR}/aqb/tutorial/
cp -r examples/demo/imgs ${WORKDIR}/aqb/examples/demo/

# remove unfinished examples from distribution

rm -f ${WORKDIR}/aqb/examples/demo/fplot*
rm -f ${WORKDIR}/aqb/examples/demo/banana*

cp -r help ${WORKDIR}/aqb/

cp -r dist/amiga/Fonts ${WORKDIR}/aqb/tutorial
cp -r dist/amiga/8svx  ${WORKDIR}/aqb/tutorial
cp -r dist/amiga/imgs  ${WORKDIR}/aqb/tutorial
cp -r dist/amiga/Fonts ${WORKDIR}/aqb/examples/demo
cp -r dist/amiga/8svx  ${WORKDIR}/aqb/examples/demo
cp -r dist/amiga/imgs  ${WORKDIR}/aqb/examples/demo

pushd ${WORKDIR}
lha a ${LHA} aqb.info aqb
popd

echo "${LHA} created."

LAMIGA=$XAMIGA/..
echo cp ${LHA} $XAMIGA
cp ${LHA} $XAMIGA

if [ -e "$LAMIGA/Apps" ]; then

    pushd "$LAMIGA/Apps"
    rm -rf aqb*
    lha x -f ${LHA}
    popd

fi

#EAMIGA=/mnt/amiga
#if [ -e "$EAMIGA/Apps" ]; then
#
#    cp ${LHA} $EAMIGA/x/
#    pushd "$EAMIGA/Apps"
#    rm -rf aqb*
#    lha x ${LHA}
#    popd
#
#fi

EXPORT=$XAMIGA/../../export
cp ${LHA} ${EXPORT}

