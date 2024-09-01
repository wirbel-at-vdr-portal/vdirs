#!/bin/bash

################################################################################
# a shell script to import your video dir structure.
# NOTE: expects svdrpsend binary to be '/usr/bin/svdrp', otherwise edit.
################################################################################


# modify here to fit your import dir, ie. SRC="/mnt/video0"
SRC="/mnt/video0"




################################################################################
# SVDRP:
#  IMPORT_NEXT <top_folder>
#      get next directory name in top_folder
#  IMPORT_ONE <top_folder>
#      triggers import of next directory in top_folder including its subdirs
#      into plugins recordings list. This includes moving of large files, 
#      creates symlinks and so on. Should not be triggered again until its
#      work is done.
################################################################################

DIRECTORY="        "
for i in `seq 1 10000`;
do
  if [ ! -d "$DIRECTORY" ]
  then
     for LINE in $(/usr/bin/svdrp plug vdirs IMPORT_NEXT $SRC) ; do
         if [[ $LINE == $SRC* ]] ; then
            LINE=$(echo "$LINE" | sed -e 's/\r//g')
            DIRECTORY="$LINE"
            break
         fi
     done;
     echo "$(date) : $DIRECTORY"
     /usr/bin/svdrp plug vdirs IMPORT_ONE $SRC
   fi
   sleep 6
done
  