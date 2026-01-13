#!mkcmd
# $Id: enum.m,v 8.8 1999/06/12 00:43:58 ksb Exp $
# enums must be valid C identifiers in a key, see "color.m"		(ksb)
#

require "enum.mh" "enum.mi" "enum.mc"

unsigned type "enum" base "enum_clients" {
	type param "token"
	type help "specify a new token from the list, ? for the list"
	type dynamic
	type update "%n = %K<EnumCvt>/ef;"
	type verify "%K<enum_listed>/ef;"
	key 1 init {
	}
}

# API interface to text -> enum function
key "EnumCvt" 1 init {
	"EnumCvt" "%N" "%K<enum_names>1ev"
}

# [1] the map, [2] the typedef,
# [3] the max name, [4] the header protection token
# [5] the table protect token
key "enum_names" 1 init {
	"%H/%ZxZn/K<enum_map>/ed"
	"%H/%ZxZn/K<enum_typedef>/ed"
	"%H/%ZxZn/K<enum_max>/ed"
	"%H/%ZxZn/K<enum_token>/ed"
	"%H/%ZxZn/K<enum_mtoken>/ed"
}

# Set %a to an expression for the type name: these keys provide the
# C identifiers to make each enum unique.  All expand with "/ed".

# Provide the map name.
key "enum_map" 1 init {
	"apcMap" "%D/%a/ee1Uv%D/%a/ee2-$lv" ""
}

# Provide the typedef name (/ed to exand this).
key "enum_typedef" 1 init {
	"" "%D/%a/eev" "_t"
}
# Provide the max value define
key "enum_max" 1 init {
	"" "%D/%a/eev" "_max"
}
# Provide the mulitple include protection define name
key "enum_token" 1 init {
	"ENUM_DECL__" "%D/%a/eev" ""
}
# Provide the mulitple map table protection define name
key "enum_mtoken" 1 init {
	"ENUM_MAP__" "%D/%a/eev" ""
}
