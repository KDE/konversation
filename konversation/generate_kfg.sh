#!/bin/bash


GROUP=""
while read line; do {
  NEWGROUP=$(echo "$line" | sed -n -e 's/.*setGroup.*\"\([^\"]*\)\".*/\1/p' )
  ENTRY=$(echo "$line" | sed -n -e 's/.*writeEntry.*\"\([^\"]*\)\".*/\1/p' )
  if [[ -n "$NEWGROUP" ]];  then
    
    if [[ -n "$GROUP" ]]; then
      echo "  </group>"
    fi
    
    GROUP=$NEWGROUP
    
    echo "  <group name=\"$GROUP\">"
  elif [[ -n "$ENTRY" ]] ; then 

    PREFERENCESENTRY=$(echo "$line" | sed -n -e 's/.*preferences.\(.*\)().*/\1/p' )
    echo $PREFERENCESENTRY
    TYPE=$( grep -i ":$PREFERENCESENTRY *(" preferences.h | sed -n -e "s/ *\(const\)\? *\(.*\)\(&\)\? *${PREFERENCESENTRY}.*/\2/p" )
    
    if [[ "$TYPE" == "int" ]] ; then TYPE="Int";
    elif [[ "$TYPE" == "QString" ]] ; then TYPE="String";
    elif [[ "$TYPE" == "bool" ]] ; then TYPE="Bool";
    elif [[ "$TYPE" == "QValueList<int>" ]] ; then TYPE="IntList";
    elif [[ "$TYPE" == "QValueList<bool>" ]] ; then TYPE="BoolList";
    elif [[ "$TYPE" == "QValueList<QString>" ]] ; then TYPE="StringList";
    elif [[ "$TYPE" == "QFont" ]] ; then TYPE="Font";
    fi

    echo "    <entry key=\"$ENTRY\" type=\"$TYPE\">"
    DEFAULT=$( grep -i "[^:]set$ENTRY *(" preferences.cpp | sed -n -e 's/.*set[^(]*[(]\(.*\)[)][^)]*/\1/p' )

    echo "      <default>$DEFAULT</default>"
    echo "      <label></label>"
    echo "      <whatsthis></whatsthis>"
    echo "    </entry>"
    
  fi
}
done

if [[ -n "$GROUP" ]]; then
  echo "  </group>"
fi


