#!/bin/bash


GROUP=""
while read line; do {
  NEWGROUP=$(echo "$line" | sed -n -e 's/.*setGroup.*\"\([^\"]*\)\".*/\1/p' )
  ENTRY=$(echo "$line" | sed -n -e 's/.*writeEntry.*\"\([^\"]*\)\".*/\1/p' )
  if [[ -n "$NEWGROUP" ]];  then
    
    if [[ -n "$GROUP" ]]; then
      echo "  </group>"
    fi
    
    GROUP="$NEWGROUP"
    
    echo "  <group name=\"$GROUP\">"
  elif [[ -n "$ENTRY" ]] ; then 

    PREFERENCESENTRY=$(echo "$line" | sed -n -e 's/.*preferences.\(.*\)().*/\1/p' )
    if [[ -z "$PREFERENCESENTRY" ]] ; then 
      echo "<!--  Could not understand:  \"$line\" -->"
    else
      echo "preferencesentry = $PREFERENCESENTRY"
    
      TYPE=$( grep -i "$PREFERENCESENTRY *(" preferences.h | sed -n -e "s/ *\(const\)\? *\([^&]*\)\(&\)\?  *${PREFERENCESENTRY}.*/\2/p" )
      echo "type = $TYPE"
      #See http://www.kde.org/standards/kcfg/1.0/kcfg.dtd
    
      if [[ "$TYPE" == "int" ]] ; then TYPE="Int";
      elif [[ "$TYPE" == "unsigned int" ]] ; then TYPE="UInt";
      elif [[ "$TYPE" == "unsigned long" ]] ; then TYPE="UInt64";
      elif [[ "$TYPE" == "long" ]] ; then TYPE="Int64";
      elif [[ "$TYPE" == "QString" ]] ; then TYPE="String";
      elif [[ "$TYPE" == "bool" ]] ; then TYPE="Bool";
      elif [[ "$TYPE" == "QValueList<int>" ]] ; then TYPE="IntList";
      elif [[ "$TYPE" == "QValueList<bool>" ]] ; then TYPE="BoolList";
      elif [[ "$TYPE" == "QValueList<QString>" ]] ; then TYPE="StringList";
      elif [[ "$TYPE" == "QFont" ]] ; then TYPE="Font";
      elif [[ "$TYPE" == "QSize" ]] ; then TYPE="Size";
      elif [[ "$TYPE" == "QRect" ]] ; then TYPE="Rect";
      elif [[ "$TYPE" == "QColor" ]] ; then TYPE="Color";
      elif [[ "$TYPE" == "QPoint" ]] ; then TYPE="Point";
      fi
    
      echo "$PREFERENCESENTRY" | grep .toString && TYPE="String"
    
      echo "    <entry key=\"$ENTRY\" type=\"$TYPE\">"
      DEFAULT=$( grep -i "[^:]set$ENTRY *(" preferences.cpp | sed -n -e 's/.*set[^(]*[(]\(.*\)[)][^)]*/\1/p' )
  
      echo "      <default>$DEFAULT</default>"
      echo "      <label></label>"
      echo "      <whatsthis></whatsthis>"
      echo "    </entry>"
    fi
     
  fi
}
done

if [[ -n "$GROUP" ]]; then
  echo "  </group>"
fi


