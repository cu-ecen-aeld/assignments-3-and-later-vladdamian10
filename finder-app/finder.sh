#!/bin/sh

# FILES_DIR="$1"
# SEARCH_STR="$2"

get_args() {
#	echo "args num = $#"
    if [ $# -lt "2" ]
    then
        return 1;
    fi
    
    if [ ! -d "$1" ]
    then
	    echo "$1 not a dir."
	    return 1;
    else
	    FILES_DIR="$1"
    fi

    if [ -z "$2" ]
    then
	    echo "String not provided or empty."
	    return 1;
    else
	    SEARCH_STR="$2"
	    echo "$SEARCH_STR"
    fi

    return 0;
}

get_num_files() {
	GREP=$(egrep -R -m 1 "${SEARCH_STR}" "${FILES_DIR}" | wc -l)
	
	if [ "${GREP}"x == ""x ]
	then
		NUM_FILES=0;
        else

		NUM_FILES=$(echo "${GREP}" | cut -d " " -f 1 );
	fi
}

get_num_lines() {
        GREP=$(egrep -R "${SEARCH_STR}" "${FILES_DIR}" | wc -l)

        if [ "${GREP}"x == ""x ]
        then
                NUM_LINES=0;
        else

                NUM_LINES=$(echo "${GREP}" | cut -d " " -f 1 );
        fi
}

### Main starts here ####

get_args $@
ret=$?
if [ $ret == 1 ]
then
	echo "Error while parsing args. Exit with error $ret".
	exit $ret;
fi

get_num_files

get_num_lines

echo "The number of files are ${NUM_FILES} and the number of matching lines are $NUM_LINES"
exit 0;

