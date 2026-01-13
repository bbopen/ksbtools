# Kevin S. Braunsdorf (KSB) Code Archive

## Memorial Archive - Updated January 2026

This archive preserves the software tools and documentation created by Kevin S. Braunsdorf (ksb), who passed away on July 24, 2024, after a long illness. Kevin was a prolific systems programmer who created "The Pundits Tool-Chain" - a comprehensive suite of Unix system administration tools developed through the NPC Guild.

Kevin studied at Purdue University and worked there as a sysadmin from 1986-1994. He later joined FedEx where his tooling enabled "a team of less than 10 people to run more than 3,200 instances" without operational failures.

---

## Archive Statistics

| Category | Count |
|----------|-------|
| **Total Files** | 2,383 |
| **C Source Files** | 235 |
| **Archive Size** | ~19 MB |
| **Tarballs Retrieved** | 25 |
| **HTML Documentation** | 25+ |

---

## Sources Retrieved

### 1. Databits.net (Internet Archive 2010-2025)
Latest versions of the tools

### 2. NPC Guild Archive (Internet Archive 2000-2006)
Historical versions from Kevin's time at FedEx

### 3. GitHub Mirror
- [razvanm/xapply](https://github.com/razvanm/xapply) - Modern build with Bazel

### 4. Blog Memorial
- [Leah Neukirchen's tribute](https://leahneukirchen.org/blog/archive/2025/08/remembering-the-work-of-kevin-s-braunsdorf-and-the-pundits-tool-chain.html)

---

## Complete Package List

### Core Build System
| Package | Version | Description |
|---------|---------|-------------|
| **msrc0** | 0.7 | Master source boot helpers (makeme, rsrc) |
| **mkcmd** | 8.14 | Command-line parser and manual page generator |
| **msrc_base** | 2.29 | Core tools (xapply, xclate, ptbw, hxmd, msrc, mmsrc) |
| **msrc** | 4.20 | Master source tools (distrib, rcsvg, Admin) |

### Configuration Management
| Package | Version | Description |
|---------|---------|-------------|
| **install_base** | 8.30 | Install, installus, instck, mk, op, purge, vinst |
| **install** | 8.5 | Earlier version with backout capability |

### Console & Terminal
| Package | Version | Description |
|---------|---------|-------------|
| **conserver** | 8.4, 8.5 | Serial console server (Kevin's version!) |
| **ptyd** | 1.8 | Pseudo-tty allocation daemon |
| **shells** | 1.3 | Special login shells (tpsh, remote, msh) |

### File Recovery
| Package | Version | Description |
|---------|---------|-------------|
| **entomb_base** | 3.11 | File recovery (unrm, preend, entomb, libtomb) |
| **entomb** | 3.9 | Earlier version |
| **rm** | 1.1 | Entomb add-ons (rm, mv, cp with recovery) |

### Parallel Processing & Pipes
| Package | Version | Description |
|---------|---------|-------------|
| **xapply** | 3.4 | Pipe tools (xapply, curly, uncurly, flock, Tee) |

### Monitoring & System Admin
| Package | Version | Description |
|---------|---------|-------------|
| **labwatch** | 4.3 | Host monitoring (labwatch, labstat, labd, labc) |
| **hostlint** | 1.20 | Host configuration checker |
| **modec** | 0.5 | Mode canon tools (sbp, vinst, modecanon) |

### Development Tools
| Package | Version | Description |
|---------|---------|-------------|
| **calls** | 3.12 | C support (calls, maketd, cdecl) |
| **cdecl** | 2.12 | C declaration analyzer |
| **perl-HXMD** | 1.10 | Perl module for hxmd configs |

### Networking & Services
| Package | Version | Description |
|---------|---------|-------------|
| **scdpn** | 2.3 | Show Cisco Discovery Protocol frames |
| **rcls** | 2.2 | Remote customer login services |

### Job Management
| Package | Version | Description |
|---------|---------|-------------|
| **qsys** | 1.7 | CPU-bound job queue |
| **turnin** | 5.1 | Electronic assignment submission |

### Miscellaneous
| Package | Version | Description |
|---------|---------|-------------|
| **gnu** | 0.1 | Example msrc for GNU configure-based software |

---

## Key Tools Explained

### mkcmd - The Foundation
mkcmd is the tool Kevin used to build all his other tools. It takes a short description of command-line options and generates integrated C source files with parsing and man pages.

### xapply - "The Most Powerful Tool"
Kevin considered xapply his most powerful creation. It's an extended apply/xargs replacement that enables sophisticated parallel processing of data streams.

### conserver - Console Server
Kevin's version of the console server from his Purdue days. Allows multiple users to watch serial consoles, with logging and write-access control. This is his implementation, distinct from conserver.com.

### op - Operator Access
Grants controlled superuser access to specific commands, enabling "mortal users to manage their own processes" safely.

### The Entombing System
A file recovery system that must be installed before you need it. Intercepts rm, mv, and cp to enable undelete functionality.

---

## Directory Structure

```
archive/
├── ARCHIVE_SUMMARY.md
├── ksb_main_page.md
├── tarballs/                    # Databits.net packages (2010)
│   ├── msrc_base-2.29.tgz
│   ├── install_base-8.30.tgz
│   ├── entomb_base-3.11.tgz
│   └── ...
├── npcguild_tarballs/           # NPC Guild packages (2000)
│   ├── mkcmd-8.14.tgz
│   ├── conserver-8.5.tgz
│   ├── labwatch-4.3.tgz
│   └── ...
├── extracted/                   # Databits packages extracted
├── npcguild_extracted/          # NPC Guild packages extracted
│   ├── mkcmd-8.14/
│   ├── conserver-8.5/
│   ├── labwatch-4.3/
│   └── ...
├── github_xapply/               # GitHub mirror
├── docs/                        # HTML documentation
│   ├── qstart.html              # Quick start guide
│   ├── msrc.html
│   ├── op.html
│   └── ...
├── source/                      # Additional source files
├── npcguild_index.html          # Package descriptions
└── npcguild_What.html           # Additional documentation
```

---

## What's Still Missing

Despite extensive searching, these packages were never archived:
- netlint, netlint_plugins (network configuration linting)
- Most *stats packages (memstats, mxstats, oraclestats, etc.)
- ptree (process tree display)
- since, Tee (newer versions)
- pollcisco, pollcsm (switch polling)
- Many others from the databits.net listing

---

## License

Kevin used a permissive MIT-style license:

> Permission is hereby granted, free of charge, to any person obtaining
> a copy of this software and associated documentation files (the
> "Software"), to deal in the Software without restriction...

His requests:
1. Keep versions distinct - mark modifications
2. Send bug reports back
3. Make software more portable, not less

---

## Building the Tools

From Kevin's README:
```bash
# Install msrc0 first (makeme)
# Then mkcmd
# Then install_base (for install, op, mk)
# Then msrc_base (for xapply, msrc, etc.)
```

See `docs/qstart.html` for the complete quick start guide.

---

## References

- [Leah Neukirchen's Memorial](https://leahneukirchen.org/blog/archive/2025/08/remembering-the-work-of-kevin-s-braunsdorf-and-the-pundits-tool-chain.html)
- [GitHub xapply mirror](https://github.com/razvanm/xapply)
- [FreeBSD mkcmd port](https://www.freshports.org/devel/mkcmd/) (deleted 2019)

---

*In memory of Kevin S. Braunsdorf (ksb)*
*Purdue 1986-1994, FedEx thereafter*
*Died July 24, 2024*
