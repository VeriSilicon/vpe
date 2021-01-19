#/bin/bash
cmd=`ps -ef | grep ptest.sh | grep -v grep | awk '{print $2}' `
if [ "$cmd" != "" ]; then
    echo $cmd | xargs kill -9
fi

cmd=`ps -ef | grep stest.sh | grep -v grep | awk '{print $2}' `
if [ "$cmd" != "" ]; then
    echo $cmd | xargs kill -9
fi

cmd=`ps -ef | grep ffmpeg | grep -v grep | awk '{print $2}' `
if [ "$cmd" != "" ]; then
    echo $cmd | xargs kill -9
fi

cmd=`ps -ef | grep srmtool | grep -v grep | awk '{print $2}' `
if [ "$cmd" != "" ]; then
    echo $cmd | xargs kill -9
fi
