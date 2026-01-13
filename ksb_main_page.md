# Kevin S. Braunsdorf (KSB) Archive

## Source: Internet Archive Wayback Machine
- Original URL: https://www.databits.net/~ksb/
- Archive Capture: September 11, 2025
- 15 total captures available (Sep 2014 - Sep 2025)

---

# The Pundits Tool-Chain from the NPC Guild (www.npcguild.org)

These tools are the ones ksb uses to keep his hosts up-to-date, to monitor configurations, resources, and track usage and escalation. Once you have them installed you'll miss them on every other host. I promise.

## Purpose

Installing these tools is a minor pain, the first time, then it gets really easy. That's because one of the key aspects of the tool-chain is that they are used to **build, install and upgrade themselves** on peer hosts. The whole purpose of these tools is to help you manage hundreds or thousands of hosts with as few as 5 people (24x7) or 3 people for business hours. We do this by:

- Sending the correct configuration to hosts (via `msrc` and `install`)
- Letting mortal users manage their own processes (via `op` and `installus`)
- Proactive close-the-loop tests (like `hostlint`)

---

## Core Packages

### msrc_base-2.29 (Production - Aug 14, 2010)
**File:** `msrc_base-2.29.tgz` (1,264,778 bytes)
**MD5:** `bc122c13f107d7fa29d2cae7b3c9639e`

Includes:
- **mmsrc** - boot-strap msrc tools
- **xapply** - extended apply and xargs replacement
- **ptbw** - parallel token broker to limit parallel execution
- **xclate** - collate output from parallel tasks
- **hxmd** - collate the output from parallel macro-expanded shell commands
- **posse** - script
- **msrc** - master source remote command driver
- **dmz** - script

### install_base-8.30 (Aug 13, 2010)
**File:** `install_base-8.30.tgz` (373,663 bytes)
**MD5:** `db6f9c8af12785368ee43c03e0a3888e`

Includes:
- **install** - update files and directories
- **installus** - install user supported software
- **instck** - check, repair, or record the modes of files
- **mk** - detect and execute shell commands in files
- **op** - grant operator access to certain commands
- **purge** - remove outdated backup copies of installed programs
- **vinst** - visually update and install a file

### level2s Build Script (v4.40)
**File:** `level2s-4.40.tgz` (10,343 bytes)
**MD5:** `561e6a7e1057f81c1ab92b3ce2c43e83`

---

## Additional Packages (Level 2)

| Package | Description |
|---------|-------------|
| oue-2.29.tgz | only unique elements filter |
| rcsvg-2.17.1.tgz | RCS version grab |
| msync-4.28.tgz | master source sync check |
| addlic-4.2.1.tgz | add a license file |
| level2s-4.40.tgz | level2 product builder |
| glob-1.7.tgz | file globbing filter |
| perl-HXMD-1.10.tgz | perl module to read/write hxmd configuration files |
| efmd-1.14.tgz | hxmd-like text filter |
| sbp-2.9.1.tgz | sync backup partitions |
| distrib-5.9.1.tgz | old master source driver |
| kruft-1.27.tgz | remove old and large files |
| tickle-1.29.tgz | notify slackers of RCS locks |
| kicker-1.22.1.tgz | turbo for cron(8) |
| binpack-1.14.1.tgz | pack files for fixed size media |
| ptree-2.19.tgz | process tree display |
| since-1.23.tgz | like tail, but across processes |
| tmbuf-1.6.1.tgz | time-based stream processing |
| sapply-1.9.1.tgz | run a screen for each command in parallel with xapply |
| curly-3.7.1.tgz | csh-line curly brace expansion and compression |
| txt2ps-3.8.1.tgz | text to PostScript filter |
| nushar-2.2.1.tgz | create an old-school shell archive |
| Tee-2.9.tgz | like tee(1) but knows about processes |
| scdpn-2.3.tgz | show Cisco Discovery protocol frames |
| daemon-1.8.1.tgz | run a process detached from the controlling terminal |
| muxcat-1.4.1.tgz | get data from a TCPMUX service |
| muxsend-1.14.1.tgz | set data to a TCPMUX service |
| recvmux-1.20.1.tgz | accept data from the above |
| passgen-2.4.1.tgz | generate a random password |
| epass-1.3.1.tgz | DES encrypt a password |
| calls-3.14.1.tgz | show structure of a C program |
| maketd-4.11.1.tgz | show depends of a C program |
| msh-1.7.tgz | lock a login out of the system with notification message |
| datecalc-1.16.1.tgz | compute values based on dates and offsets |
| tart-1.4.1.tgz | filter tar archives on the fly |
| sudop-1.8.1.tgz | make "op" look like "sudo" |
| cdecl-2.12.tgz | show the structure of a C declaration |
| haveip-1.13.1.tgz | shell test for virtual IP ownership |

---

## PEG Support & Monitoring Tools

| Package | Description |
|---------|-------------|
| rrdup-1.11.1.tgz | send data to PEG |
| rrdisk-2.37.1.tgz | send disk stats to PEG |
| netlint-1.65.tgz | report on network configuration |
| netlint_plugins-1.18.tgz | standard modules for netlint |
| hostlint-1.20.tgz | download a script to check the local host for junk |
| memstats-1.6.1.tgz | report on memory use |
| mxstats-1.5.1.tgz | report on mail exchanger use |
| namedstats-1.6.1.tgz | report on name service use |
| networkstats-1.10.1.tgz | report on a network interface |
| oraclestats-1.10.1.tgz | report on Oracle use |
| pollcisco-1.27.tgz | poll a switch for all port stats |
| pollcsm-2.9.1.tgz | poll a CSM for vip usage |
| slapdstats-1.5.1.tgz | report on LDAP use |
| unixstats-2.11.1.tgz | report on CPU use |
| vcsstats-1.18.1.tgz | report VCS ownership |
| FXPegMon-1.16.1.tgz | start any of the above |

---

## File Entombing (v3.11)
**File:** `entomb_base-3.11.tgz` (91,225 bytes)
**MD5:** `3e07da241354489db0b08fb1aac12929`

Provides file recovery for users who accidentally remove files.

Includes:
- **unrm** - restore files eaten by rm, mv, or cp
- **preend** - age off entombed files
- **entomb** - common shell interface to entombing
- **libtomb.a** - C interface to entombing
- **untmp** - remove junk files from /tmp
- **rmfile** - like rm, but better about special characters

Additional: `entombing-1.2.1.tgz` - recompile rm, mv, and cp on FreeBSD

---

## Console Server
Kevin's version (alternative to conserver.com):
- **conserver-8.9.1.tgz** - remotely log+manage system serial/virtual consoles
- **autologin-2.5.1.tgz** - put a shell on your consoles at boot

---

## Spares (for systems missing these tools)
- quot-1.7.1.tgz - report disk hogs by name/group
- tcpmux-1.15.1.tgz - replace missing RFC 1278
- jot-1.9.tgz - replace missing jot(1)
- stat-2.9.tgz - kinda replace stat(1)
- flock-2.10.1.tgz - shell access to flock(2)
- remote-1.23.1.tgz - use someone else's workstation as a dumb terminal

---

## Technical Notes

### UID Convention
All tar archives assume a login with uid 810 ("source" login). This makes building as non-root easier.

### Prerequisites
- xinetd (for tcpmux services)
- perl-ExtUtils-MakeMaker, perl-devel, h2phperl
- pdksh or ksh
- sysstat
- perl-5.8+
- glibc-headers, flex, bison
- libiconv, gdbm-devel
- libpcap and libpcap-devel

---

## Archive Sources
- Main page: https://web.archive.org/web/20250911180226/https://www.databits.net/~ksb/
- MSRC directory: https://web.archive.org/web/*/https://www.databits.net/~ksb/msrc/
