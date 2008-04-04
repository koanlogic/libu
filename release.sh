#!/bin/sh
#
# LibU release script

# set these !
REL_TAG="LIBU_REL_1_2_0"
REL_VERSION="1.2.0"

# cvs 
REL_ROOT="KL/libu"

# should match our (i.e. stewy, tat, tho) local directory hierarchy
# we all have a ~/work/ dir with KL CVS root inside no ? :)
REL_WD="${HOME}/work/${REL_ROOT}"

REL_DIR="${REL_WD}/DIST"
REL_TMP_DIR="${REL_DIR}/tmp"
REL_OUT_DIR="${REL_DIR}/out"

REL_TARGET="root@gonzo.koanlogic.com:/var/www-anemic/www/download/libu/"

make_pre() 
{
    cd ${REL_WD}
    rm -r ${REL_TMP_DIR}
    mkdir -p ${REL_TMP_DIR}
    mkdir -p ${REL_OUT_DIR}
}

make_tag()
{
    echo "==> tagging as \"${REL_TAG}\""
    cvs tag -F ${REL_TAG}
}

make_export()
{
    echo "==> exporting tagged tree"
    pushd .
    cd ${REL_TMP_DIR}
    cvs export -r ${REL_TAG} ${REL_ROOT}
    popd
}

make_dist()
{
    echo "==> creating dist"

    pushd .
    cd ${REL_TMP_DIR}/${REL_ROOT}
    makl-conf
    makl -f Makefile.dist dist
    cp ChangeLog libu-${REL_VERSION}.* ${REL_OUT_DIR}
    popd
}

make_upload()
{
    echo "uploading release"
    scp -r ${REL_OUT_DIR}/libu-${REL_VERSION}.* ChangeLog ${REL_TARGET}
}

make_clean()
{
    rm -r ${REL_DIR}
}

make_pre
make_tag
make_export
make_dist
make_upload
make_clean
