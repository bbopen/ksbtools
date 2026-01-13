#!mkcmd
# $Id: example_enum.m,v 8.7 1997/10/18 19:21:48 ksb Beta $
# Take a color on the command line and a file of temperatures.		(ksb)
# This shows off what the enum interface can do for you, a little.

require "std_help.m"

basename "rainbox" ""

# declare a rainbox enumeration
type "enum" type "color" {
	key 1 init {
		"red" "orange" "yellow" "green" "blue" "indigo" "violet"
	}
}

# bind option c to that enum type
type "color" 'c' {
	after 'printf("%%s: %%d = %%s\\n", %b, %n, %K<enum_names>1ev[%n]);'
	help "example color"
}

# try another to show we can do two at once
type "enum" type "temperature" {
	key 1 init {
		"hot" "warm" "cold" "freezing"
	}
}

# read those from a file
file ["r"] 'C' {
	named "fpColors" "pcColors"
	routine {
		type "temperature" named "cRead" "pcRead" {
			help "read a temperature from a file"
		}
		list "cRead"
		user 'printf("%%s: line %%d: temp %%d\\n", %b, %MN, cRead);'
	}
	user "%MI(%n, %N);"
	help "an example file of temperatures"
}
