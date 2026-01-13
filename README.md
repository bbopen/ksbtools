# The Pundits Tool-Chain

*In memory of Kevin S. Braunsdorf, "KSB" (1964-2024)*

```
$Id: README,v 1.0 2026/01/13 ksb memorial edition $
```

---

KSB was a true genius who loved his family and colleagues. I learned
a lot from him about technology and cybernetics. These tools were his
passion - about 125,000 lines of nifty C code he shared with the world.

He passed away on July 24, 2024, after a long illness.

This archive preserves what we could recover. The ideas and concepts
here are still applicable in many areas of system management today.

---

## What Is This?

As KSB wrote:

> These are the tools and a brief description of the tool... This source
> code is published as reference documents only. This is an overview of
> the code used to run various system admin functions by me (K S
> Braunsdorf). It doesn't represent anything useful.
>
> In fact it might be bad. I wouldn't recommend that you download it or
> run any part of it. Just read it.

(He was being modest. At FedEx, his tooling enabled "a team of less than
10 people to run more than 3,200 instances" without operational failures.)


## Philosophy

KSB believed:

> The UNIX maxim that the strongest solution comes from a set of parts
> organized into an exact fit for each specific problem. The best of
> these tool kits are made of parts that always work well with each
> other, in most any combination.

And:

> Processes engineered with msrc are automated, repeatable, faster, and
> scale better than ad hoc implementations.


## The Tools

### mkcmd - The Foundation

The program that writes all his user interface code for him. This is
the key to all his stuff. It contains a library of small components
that KSB reused over and over to write all this stuff.

Takes a short description of command-line options and builds an
integrated C source file with parsing and man pages.

### xapply - "The Most Powerful Tool"

KSB wrote:

> I think xapply is the most powerful tool I have.

A replacement for xargs(1) and apply(1). Will keep N jobs running in
parallel, will read the list of items from the command line, stdin, or
will match lines in multiple files to control jobs.

### msrc - Master Source

The system KSB used to write all this and keep the release/version/revision
work straight. He had a model for software construction that he used to
build all this, and these tools made that work.

The main purpose of msrc is to automate running the same command with
the same data files and parameters on lots of hosts.

### conserver - Console Server

The serial console service he coded at Purdue. Allows multiple users to
watch serial consoles, with logging and write-access control.

### op - Operator Access

Grants controlled superuser access to specific commands. As KSB noted,
use op(1) rather than sudo(1), really.

### The Entombing System

> Helps recover lost files, must be installed and configured before you
> run "rm".

A file recovery system that intercepts rm, mv, cp to enable undelete.

### install - A Much Better Product Installer

These tools help you keep a backup path when you make changes to a host.
Also allows normal people to maintain some products in a "User supported"
area of your hosts.

### labwatch - Host Monitoring

> Helps you keep track of clients that might "walk away", or monitor
> CPU load / login load of hosts real time.

### qsys - Job Queue

> A very light weight job scheduler. Now days hosts are so fast it
> hardly seems worth it. It has a pleasing (non-root) design in any case.

### Other Tools

- **calls** - C support tools (calls, maketd, cdecl)
- **ptyd** - Pseudo-tty allocation daemon (1989-1995 at Purdue)
- **shells** - Special login shells (tpsh, remote, msh)
- **turnin** - Electronic assignment submission system
- **hxmd** - Host execution meta-data driver
- **flock** - BSD-style file locking wrapper
- **curly** - csh-style brace expansion: a{1,2,3}.c -> a1.c a2.c a3.c
- **Tee** - tee(1) that popens as well


## Archive Contents

```
archive/
  tarballs/           # Databits.net packages (2010-2025)
  npcguild_tarballs/  # NPC Guild packages (2000-2006)
  extracted/          # Extracted databits packages
  npcguild_extracted/ # Extracted NPC Guild packages
  github_xapply/      # GitHub mirror (razvanm/xapply)
  docs/               # HTML documentation
  source/             # Additional source files
```

### Packages Retrieved

From databits.net:
- msrc_base-2.29, install_base-8.30, entomb_base-3.11
- cdecl-2.12, hostlint-1.20, perl-HXMD-1.10, scdpn-2.3

From NPC Guild (historical):
- mkcmd-8.14, msrc-4.20, msrc0-0.7
- conserver-8.4, conserver-8.5
- labwatch-4.3, calls-3.12, entomb-3.9
- xapply-3.4, install-8.5, ptyd-1.8
- qsys-1.7, shells-1.3, turnin-5.1
- modec-0.5, rcls-2.2, rm-1.1, gnu-0.1


## Building

From KSB's README:

```sh
# Install msrc0 first (makeme)
# Then mkcmd
# Then install_base (for install, op, mk)
# Then msrc_base (for xapply, msrc, etc.)
```

Prerequisites:
- m4 with -I and -D options (GNU m4 on Solaris)
- BSD-compatible install
- C compiler (cc or gcc)
- make that understands -C

See `docs/qstart.html` for the complete quick start guide.


## License

KSB used a permissive MIT-style license with three requests:

1. **Keep versions distinct** - if you make changes, mark them
2. **Send bug reports back** - help improve the code
3. **Make this Software more portable, not less** - avoid vendor-specific calls

He wrote:

> Please contribute to this Software in a constructive light.


## What's Missing

Despite extensive searching, many packages were never archived:
- netlint, netlint_plugins
- Most *stats packages (memstats, mxstats, oraclestats)
- ptree, pollcisco, pollcsm
- Many others from the original databits.net listing


## References

- [Leah Neukirchen's Memorial](https://leahneukirchen.org/blog/archive/2025/08/remembering-the-work-of-kevin-s-braunsdorf-and-the-pundits-tool-chain.html)
- [GitHub xapply mirror](https://github.com/razvanm/xapply)


---

```
Kevin S. Braunsdorf
Purdue University, 1986-1994
FedEx, thereafter
Died July 24, 2024

"Thank you very much for the time you spent reading this." --ksb
```
