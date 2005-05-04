#!/bin/bash 


GROUP=""
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
echo "<kcfg xmlns=\"http://www.kde.org/standards/kcfg/1.0\""
echo "      xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
echo "      xsi:schemaLocation=\"http://www.kde.org/standards/kcfg/1.0"
echo "      http://www.kde.org/standards/kcfg/1.0/kcfg.xsd\" >"
echo "  <include>qfont.h</include>"
echo "  <include>qsize.h</include>"
echo "  <include>kdialog.h</include>"
echo "  <kcfgfile name=\"konversationrc\" />"

cat konversationapplication.cpp | while read line; do {
  NEWGROUP2=$(echo "$line" | sed -n -e 's/^ *[^/].*setGroup.*\"\([^\"]*\)\".*/\1/p' )
  ENTRY=$(echo "$line" | sed -n -e 's/^ *[^/].*writeEntry.*\"\([^\"]*\)\".*/\1/p' )

  if [[ -n "${NEWGROUP2}" ]]; then
    NEWGROUP="${NEWGROUP2}"
  elif [[ -n "$ENTRY" ]] ; then 
    if [[ -n "$NEWGROUP" ]] ; then
      if [[ -n "$GROUP" ]]; then
        echo "  </group>"
      fi
      GROUP="$NEWGROUP"
      NEWGROUP=""
      echo "  <group name=\"$GROUP\">"
    fi
     
    PREFERENCESENTRY=$(echo "$line" | sed -n -e 's/^.*preferences.\(.*\)().*$/\1/p' )
    if [[ -z "$PREFERENCESENTRY" ]] ; then 
      echo "<!--  Could not understand:  \"$line\" -->"
    else
      echo "    <!-- preferencesentry = $PREFERENCESENTRY -->"
    
      TYPE=$( grep -i "$PREFERENCESENTRY *(" preferences.h | sed -n -e "s/ *\(const\)\? *\([^&]*\)\(&\)\?  *${PREFERENCESENTRY}.*/\2/p" )
      echo "    <!-- type = $TYPE  -->"
      #See http://www.kde.org/standards/kcfg/1.0/kcfg.dtd
    
      if [[ "$TYPE" == "int" ]] ; then TYPE="Int";
      elif [[ "$TYPE" == "unsigned int" ]] ; then TYPE="UInt";
      elif [[ "$TYPE" == "uint" ]] ; then TYPE="UInt";
      elif [[ "$TYPE" == "unsigned long" ]] ; then TYPE="UInt64";
      elif [[ "$TYPE" == "ulong" ]] ; then TYPE="UInt64";
      elif [[ "$TYPE" == "long" ]] ; then TYPE="Int64";
      elif [[ "$TYPE" == "QString" ]] ; then TYPE="String";
      elif [[ "$TYPE" == "bool" ]] ; then TYPE="Bool";
      elif [[ "$TYPE" == "QValueList<int>" ]] ; then TYPE="IntList";
      elif [[ "$TYPE" == "QValueList<bool>" ]] ; then TYPE="BoolList";
      elif [[ "$TYPE" == "QValueList<QString>" ]] ; then TYPE="StringList";
      elif [[ "$TYPE" == "QStringList" ]] ; then TYPE="StringList";
      elif [[ "$TYPE" == "QFont" ]] ; then TYPE="Font";
      elif [[ "$TYPE" == "QSize" ]] ; then TYPE="Size";
      elif [[ "$TYPE" == "QRect" ]] ; then TYPE="Rect";
      elif [[ "$TYPE" == "QColor" ]] ; then TYPE="Color";
      elif [[ "$TYPE" == "QPoint" ]] ; then TYPE="Point";
      else TYPE="";
      fi
    
      echo "$PREFERENCESENTRY" | grep .toString > /dev/null && TYPE="String"
    
      DEFAULT=$( grep -i "[^:]set$ENTRY *(" preferences.cpp | sed -n -e 's/.*set[^(]*[(]\(.*\)[)][^)]*/\1/p' )
      
      DEFAULT2=$(echo "$DEFAULT" | sed -n -e "s/^\(\"\(.*\)\"\|\([0-9][0-9]*\|true\|false\)\)$/\2\3/p" )
      
      if [[ -z "$TYPE" ]]; then
        ISSTRING=$(echo "$DEFAULT" | sed -n -e "s/^\"\(.*\)\"$/\1/p" )
	if [[ -n "$ISSTRING" ]]; then
	  TYPE="String"
	fi
	ISNUM=$(echo "$DEFAULT" | sed -n -e "s/^\([0-9][0-9]*\)$/\1/p" )
	if [[ -n "$ISNUM" ]] ; then
	  TYPE="Int"
	fi
	ISBOOL=$(echo "$DEFAULT" | sed -n -e "s/^\(true|false\)$/\1/p" )
	if [[ -n "$ISBOOL" ]] ; then
	  TYPE="Bool"
	fi

        echo "$PREFERENCESENTRY" | grep Color > /dev/null && TYPE="Color"
      fi
      
      
      echo "    <entry key=\"$ENTRY\" type=\"$TYPE\">"
      if [[ -n "${DEFAULT2}" ]] ; then 
        echo "      <default>${DEFAULT2}</default>"
      elif [[ -n "$DEFAULT}" ]] ; then
        echo "      <default code=\"true\">${DEFAULT}</default>"
      else
        echo "      <default></default>"
      fi
      LABEL=$(egrep "cstring.*$ENTRY" *.ui -h -A 10 | grep "<string>" | head -n 1 | sed -n -e 's/^.*<string> *\([^ ].*\) *<\/string>.*$/\1/p')
      echo "      <label>$LABEL</label>"
      echo "      <whatsthis></whatsthis>"
      echo "    </entry>"
    fi
     
  fi
}
done

echo "  </group>"

echo "</kcfg>"
