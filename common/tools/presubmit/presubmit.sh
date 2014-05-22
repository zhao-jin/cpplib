#!/bin/bash
# author: simonwang <simonwang@tencent.com>
# presubmit helper script

BUILD_MACHINE_IP="10.6.193.143"
REMOTE_USER=presubmit
REMOTE_PASSWD=presubmit_admin
PATCH_REMOTE_BASE_DIR="/data4/presubmit/code/patch"
PATCH_REMOTE_DIR=""
PATCH_LOCAL_BASE_DIR="/tmp"
PATCH_LOCAL_DIR=""
USER_NAME=`whoami`
IP_ADDRESS=`/sbin/ifconfig -a | grep inet | grep -v 127.0.0.1 | awk '{print $2}' | tr -d "addr:"`
CURRENT_TIME_STR=`date +%Y_%m_%d_%H_%M_%S`
SVN_TYPE="trunk"
SVNDIR_LIST="/trunk/src/common
/trunk/src/poppy
/trunk/src/thirdparty
/branches/poppy/20110907
/branches/common/20110809-001
/branches/thirdparty/20110809-001"
PATCH_SUBJECT=""
CODE_CHANGE_FILE="change.info"
SUBJECT_INFO_FILE="subject.info"
SUBMIT_USER_INFO_FILE="user.info"
SUBMIT_TIME_INFO_FILE="time.info"

# help message
function help_msg()
{
    cat >&2 <<END
This script helps you to upload your patch to build machine to build and test before you submit
Usage: run `basename $0` under your code directory or \"`basename $0` --help\" to get help

It first scans all the directories under BLADE_ROOT, update them to the latest version of svn
if there exists some conflicts, it exit and you should resolve the conflicts manually
if there are no conflicts, it generates the all the patches of all directories under BLADE_ROOT
and upload them to the remote build machine to build and test.
If build break happens , the system will send you a email to tell you the status"
END
    exit 1
}

# find BLADE_ROOT's directory
function find_project_root()
{
    local dir
    dir=$PWD
    while [ "$dir" != "/" ]; do
        if [ -f "$dir/BLADE_ROOT" ]; then
            echo "$dir"
            return 0
        fi;
        dir=`dirname "$dir"`
    done
    return 1
}

# test whether $1 is a svn working copy directory or not
function is_svn_working_copy_dir()
{
    if [ -z "$1" ]; then
        echo "you should specify directory to test" >&2
        exit 1
    fi

    svn info $1 2>&1 | grep "not a working copy" >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        return 1
    else
        return 0
    fi
}

function svn_update()
{
    if [ -z "$1" ]; then
        echo "you should specify root directory" >&2
        exit 1
    fi

    local file
    local svn_conflict_files
    for file in $1/*
    do
        if [ -d $file ]; then
            is_svn_working_copy_dir $file
            if [ $? -eq 0 ]; then
                svn update $file >/dev/null 2>&1
                svn_conflict_files=`svn st $file | awk '{if($1=="C") print $2}'`
                if [ "$svn_conflict_files" != "" ]; then
                    echo "$file exists conflicts to the svn version, please resolve it before precommit" >&2
                    echo "conflict files: " >&2
                    echo "$svn_conflict_files" >&2
                    exit 1
                else
                    echo "$file no conflicts" >&2
                fi
            fi
        fi
    done
}

function determineSvnType()
{
    if [ -z "$1" ]; then
        echo "you should specify svn url" >&2
        exit 1
    fi

    local svnurl=$1
    echo "$svnurl" | grep -q "/trunk/"
    if [ $? -eq 0 ]; then
        SVN_TYPE="trunk"
    else
        echo "$svnurl" | grep -q "/branches/"
        if [ $? -eq 0 ]; then
            SVN_TYPE="branches"
        else
            echo "can't determine svn type for $svnurl" >&2
            exit 1
        fi
    fi
}

function generate_patch()
{
    if [ -z "$1" ]; then
        $1="."
    fi

    PATCH_LOCAL_DIR="${PATCH_LOCAL_BASE_DIR}/${IP_ADDRESS}_${USER_NAME}_${CURRENT_TIME_STR}"
    local svndir_name
    local svnurl
    cd $1 >/dev/null 2>&1
    for dir in $1/*
    do
        echo "$dir" >&2
        if [ -d "$dir" ]; then
            # if dir is a symbol link, replace it with its destination dir
            if [ -h "$dir" ]; then
                dir=`ls -l $dir | awk '{print $10}'`
            fi
            svn info "$dir" >/dev/null 2>&1
            if [ $? -eq 0 ]; then
                svnurl=`svn info "$dir" | grep "^URL:"`
                determineSvnType "$svnurl"
                for i in $SVNDIR_LIST
                do
                    echo "$svnurl" | grep -q "$i"
                    if [ $? -eq 0 ]; then
                        case "$i" in
                            "/trunk/src/common")
                                svndir_name="common"
                                ;;
                            "/trunk/src/poppy")
                                svndir_name="poppy"
                                ;;
                            "/trunk/src/thirdparty")
                                svndir_name="thirdparty"
                                ;;
                            "/branches/poppy/20110907")
                                svndir_name="poppy"
                                ;;
                            "/branches/common/20110809-001")
                                svndir_name="common"
                                ;;
                            "/branches/thirdparty/20110809-001")
                                svndir_name="thirdparty"
                                ;;
                        esac
                        local patch_name="${svndir_name}".patch
                        if [ ! -e "${PATCH_LOCAL_DIR}" ]; then
                            mkdir -p "${PATCH_LOCAL_DIR}"
                        fi
                        # deal the situation when dir is a symbol link
                        # cd `dirname $dir` >/dev/null 2>&1
                        cd $dir >/dev/null 2>&1
                        svn diff  >"${PATCH_LOCAL_DIR}/$patch_name"
                        svn diff | grep "^Index:" | awk '{print $2}' >>"${PATCH_LOCAL_DIR}/$CODE_CHANGE_FILE"
                        cd - >/dev/null 2>&1
                    fi
                done
            fi
        fi
    done
    echo "generate patch finish" >&2
    cd - >/dev/null 2>&1
}

function upload_patch()
{
    local remote_dir_name="${IP_ADDRESS}_${USER_NAME}_${CURRENT_TIME_STR}"
    PATCH_REMOTE_DIR="${PATCH_REMOTE_BASE_DIR}/${SVN_TYPE}"

    # mkdir tgz of current patch dir
    if [ ! -e "$PATCH_LOCAL_DIR" ]; then
        echo "$PATCH_LOCAL_DIR not exist, please check you have permission to create folder" >&2
        exit 1
    fi
    cd $PATCH_LOCAL_BASE_DIR >/dev/null 2>&1
    echo $PATCH_SUBJECT >`basename $PATCH_LOCAL_DIR`/$SUBJECT_INFO_FILE
    echo $USER_NAME >`basename $PATCH_LOCAL_DIR`/$SUBMIT_USER_INFO_FILE
    echo `date "+%Y-%m-%d %H:%M:%S"` >`basename $PATCH_LOCAL_DIR`/$SUBMIT_TIME_INFO_FILE
    tar czvf ${remote_dir_name}.tgz `basename $PATCH_LOCAL_DIR`
    rm -rf `basename $PATCH_LOCAL_DIR`
    cd - >/dev/null 2>&1
    ~/bin/work_by_ip.sh "$BUILD_MACHINE_IP" "$REMOTE_USER" "$REMOTE_PASSWD" copyto "${PATCH_LOCAL_BASE_DIR}/${remote_dir_name}.tgz" "$PATCH_REMOTE_DIR"
    if [ $? -ne 0 ]; then
        echo "Some error happended, upload patch error" >&2
    else
        echo "You patch has been uploaded to the build server" >&2
        echo "after build, the system will send you a email to report the build status" >&2
        echo "please wait paiently" >&2
    fi
    rm ${PATCH_LOCAL_BASE_DIR}/${remote_dir_name}.tgz
}

if [ "$1" == "--help" ]; then
    help_msg
    exit 1
fi

if PROJECT_ROOT=`find_project_root`; then
    echo -n "Enter the code change subject: " >&2
    read PATCH_SUBJECT
    if [ -z "$PATCH_SUBJECT" ]; then
        echo "you should specify code change subject" >&2
        exit 1
    fi
    echo "project root: $PROJECT_ROOT" >&2
    svn_update "$PROJECT_ROOT"
    generate_patch "$PROJECT_ROOT"
    upload_patch
else
    echo "Can't find BLADE_ROOT, are you in a correct source dir tree?" >&2
fi
