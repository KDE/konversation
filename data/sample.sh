#!/bin/bash
DATA=`ls -l`
RESULT=`dcop $1 Konversation info "$DATA"`


