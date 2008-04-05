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


yesno ()
{
    /bin/echo -n "$1 " 
    
    while [ true ] 
    do 
        read answer
        case ${answer} in
            [Yy]) return 0 ;;
            [nN]) return 1 ;;
            *) /bin/echo -n "please say [yY] or [nN]: " ;; 
        esac
    done 
}       

err ()
{
    /bin/echo $@
    exit 1
}

make_pre() 
{
    rm -r ${REL_TMP_DIR}
    mkdir -p ${REL_TMP_DIR} || err "can't create directory ${REL_TMP_DIR}"
    mkdir -p ${REL_OUT_DIR} || err "can't create directory ${REL_OUT_DIR}"
}

make_tag()
{
    yesno "using tag \"${REL_TAG}\", do you want to continue ?" \
        || err "bailing out on client request"

    echo "==> tagging as \"${REL_TAG}\""
    cvs tag -F ${REL_TAG} || err "cvs tag command failed"
}

make_export()
{
    echo "==> exporting tagged tree"
    pushd .
    cd ${REL_TMP_DIR} || err "can't cd to ${REL_TMP_DIR}"
    cvs export -r ${REL_TAG} ${REL_ROOT} || err "cvs export command failed"
    popd
}

make_dist()
{
    echo "==> creating dist"

    pushd .
    cd ${REL_TMP_DIR}/${REL_ROOT} \
        || err "can't cd to ${REL_TMP_DIR}/${REL_ROOT}"

    # we need doxygen path
    makl-conf || err "makl-conf failed"
    makl -f Makefile.dist dist || "dist target failed"

    cp ChangeLog libu-${REL_VERSION}.* ${REL_OUT_DIR} \
        || err "can't copy dist files to ${REL_OUT_DIR}"
    popd
}

make_upload()
{
    echo "uploading release"
    scp -r ${REL_OUT_DIR}/libu-${REL_VERSION}.* ChangeLog ${REL_TARGET} \
        || err "can't copy dist files to ${REL_TARGET}"
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
