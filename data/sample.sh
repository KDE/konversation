#!/bin/bash
DATA=`echo This is a test script`
RESULT=`dcop $1 Konversation info "$DATA"`


