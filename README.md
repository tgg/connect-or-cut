# connect-or-cut [![Build Status](https://travis-ci.org/tgg/connect-or-cut.svg)](https://travis-ci.org/tgg/connect-or-cut)[![AppVeyor Status](https://ci.appveyor.com/api/projects/status/github/tgg/connect-or-cut?branch=master&svg=true)](https://ci.appveyor.com/project/teejeejee/connect-or-cut)

Stateless LD_PRELOAD based poor man's firewall.

## Demo

[![asciicast](https://asciinema.org/a/66ggwn843r9qudwi13nyn5ru0.png)](https://asciinema.org/a/66ggwn843r9qudwi13nyn5ru0?autoplay=1&speed=2)

## What it is

connect-or-cut is a small library to interpose with LD_PRELOAD to a
program to prevent it from connecting where it should not.

This is similar to a firewall, except that:

 * you do not need to be root to use it
 * only processes launched after `LD_PRELOAD` is set are affected, not
   the full system

## Installation

Provided you have a C compiler, GNU or BSD make on a Unix box:

    $ git clone https://github.com/tgg/connect-or-cut.git

    $ cd connect-or-cut

    $ make os=$(uname -s)

If you want to compile it for 32 bits:

    $ make os=$(uname -s) bits=32

If you want to compile it with debug information:

    $ CFLAGS=-g make os=$(uname -s)


## Using it

    $ ./coc -d -a '*.google.com' -a '*.1e100.net' -b '*' iceweasel

Or to do it manually:

    $ LD_PRELOAD=./libconnect-or-cut.so COC_ALLOW='212.27.40.240:53;212.27.40.241:53;*.google.com;*.1e100.net' COC_BLOCK='*' iceweasel

This invokes iceweasel, allowing only outgoing connection to my ISP
provided name servers, to all google.com and 1e100.net addresses.

Everything else will be blocked, with connection blocking messages
displayed on stderr.

`coc` is a helper script that will set appropriate variables for you:

    $ ./coc -h
    Usage: coc [OPTION]... [--] COMMAND [ARGS]
    Prevent connections to blocked addresses in COMMAND.
    
    If no COMMAND is specified but some addresses are configured to be allowed or
    blocked, then shell snippets to set the chosen configuration are displayed.
    
    OPTIONS:
     -d, --allow-dns                   	Allow connections to DNS nameservers.
     -a, --allow=ADDRESS[/BITS][:PORT] 	Allow connections to ADDRESS[/BITS][:PORT].
     -b, --block=ADDRESS[/BITS][:PORT] 	Prevent connections to ADDRESS[/BITS][:PORT].
                                       	BITS is the number of bits in CIDR notation
                                       	prefix. When BITS is specified, the rule
                                       	matches the IP range.
     -h, --help                        	Print this help message.
     -t, --log-target=LOG              	Where to log. LOG is a comma-separated list
                                       	that can contain the following values:
                                       	  - stderr      This is the default
                                       	  - syslog      Write to syslog
                                       	  - file        Write to COMMAND.coc file
     -p, --log-path=PATH               	Path for file log.
     -l, --log-level=LEVEL             	What to log. LEVEL can contain one of the
                                       	following values:
                                       	  - silent      Do not log anything
                                       	  - error       Log errors
                                       	  - block       Log errors and blocked
                                       	                connections
                                       	  - allow       Log errors, blocked and
                                       	                allowed connections
                                       	  - debug       Log everything
     -v, --version                     	Print connect-or-cut version.


## Use cases

You can use connect-or-cut to:

 1. Sandbox an untrusted application to prevent all ourgoing connections from it
 2. Monitor where an application is connecting to understand how it works
 3. Filter out advertising sites during your web navigation

## Environment variables

Again, `coc` helper script should be used to do the heavy-lifting here.

 * `COC_ALLOW` is a comma separated list of addresses to allow
 * `COC_BLOCK` is a comma separated list of addresses to block
 * `COC_LOG_LEVEL` defines the level of log, from `0` (silent) to `4` (debug)
 * `COC_LOG_TARGET` is a bitwise between the following values:
   * `1` log to stderr
   * `2` log to syslog
   * `4` log to a file

## Limitations

 * connect-or-cut does not work for programs:
   * performing connect syscall directly;
   * statically linked (e.g. Go binaries)
 * Only outgoing connection using TCP over IPv4 or IPv6 work for now.
 * Tested on:
   * Debian GNU/Linux with gcc and clang
   * FreeBSD 11.0-STABLE with clang
   * NetBSD 7 with gcc
   * Solaris 10
   * macOS 10.4

   Portability patches welcome!

## Known issues

 * Aliases from /etc/hosts are not honoured. For instance if a machine is
   aliased and the long name conforms to a pattern it will be missed.
   The workaround for this is to add the short name to the rule set.

   This will require parsing /etc/hosts by hand, given that:
   - getnameinfo does not provide access to aliases
   - gethostbyaddr/gethostent, which do, are not reentrant on all platforms

## News
 * Version 1.0.3 (2017-03-21)
   * Fix testsuite on SunOS
   * Fix RPM compilation in 32 bits

 * Version 1.0.2 (2017-03-21)
   * Implement IPv6 filtering
   * Fixes for macOS and Solaris
   * Add a testsuite
   * Bugs fixed:
     * Crash before initialization when SELinux is enabled
     * Crash when log file could not be created
     * Wrong connect() return value when blocking calls

 * Version 1.0.1 (2016-11-01)
   * Add some consistency checks
   * Use a SONAME (this broke Mac OS X compilation)
   * Provide RPM package
   * `coc` script now display LD_PRELOAD when used in dry mode

 * Version 1.0.0 (2016-04-12)

## Roadmap

 * WARN if localhost is not allowed
 * Port on Windows
 * Make filtering algorithm configurable. For now it's always:
   * check against ALLOW list and ALLOW connection if it's in it;
   * else check against BLOCK list and BLOCK connection if it's in it;
   * else ALLOW by default
 * Implement `accept` filtering
