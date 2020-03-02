#! /bin/bash

RED="\033[1;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE="\033[1;34m"
PURPLE="\033[1;35m"
CYAN="\033[1;36m"
WHITE="\033[1;37m"
RESET="\033[0m"

QUESTION_FLAG="${GREEN}?"
WARNING_FLAG="${YELLOW}!"
NOTICE_FLAG="${CYAN}❯"


if [ "$#" -lt 1 ]; then 
    echo "Usage: $0 major.minor[.micro.nano]"
    exit 1
fi

#      MAJOR MINOR MICRO NANO
V_SPEC=("0" "0" "0" "0")

BASE_LIST=(`echo $1 | tr '.' ' '`)
for i in "${!BASE_LIST[@]}"
do
    V_SPEC[$i]=${BASE_LIST[$i]}
   # do whatever on $i
done

echo -ne "About to set version to ${RED}${V_SPEC[0]}.${V_SPEC[1]}.${V_SPEC[2]}.${V_SPEC[3]}${RESET} in ${CYAN}configure.ac${RESET}. Is that ok? ${WHITE}[y/N]${RESET}: "
read RESPONSE
if [ "$RESPONSE" = "" ]; then RESPONSE="n"; fi
if [ "$RESPONSE" = "Y" ]; then RESPONSE="y"; fi
if [ "$RESPONSE" = "Yes" ]; then RESPONSE="y"; fi
if [ "$RESPONSE" = "yes" ]; then RESPONSE="y"; fi
if [ "$RESPONSE" = "YES" ]; then RESPONSE="y"; fi
if [ "$RESPONSE" = "y" ]; then
    sed -i "/m4_define(teo_major_version/c\\m4_define(teo_major_version,[${V_SPEC[0]}])" configure.ac
    sed -i "/m4_define(teo_minor_version/c\\m4_define(teo_minor_version,[${V_SPEC[1]}])" configure.ac
    sed -i "/m4_define(teo_micro_version/c\\m4_define(teo_micro_version,[${V_SPEC[2]}])" configure.ac
    sed -i "/m4_define(teo_nano_version/c\\m4_define(teo_nano_version,[${V_SPEC[3]}])" configure.ac
fi

