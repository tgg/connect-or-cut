# connect-or-cut [![Build Status](https://travis-ci.org/tgg/connect-or-cut.svg)](https://travis-ci.org/tgg/connect-or-cut) [![AppVeyor Status](https://ci.appveyor.com/api/projects/status/github/tgg/connect-or-cut?svg=true)](https://ci.appveyor.com/project/teejeejee/connect-or-cut)

Simple network sandbox for Unix and Windows.

## Demo

[![asciicast](https://asciinema.org/a/66ggwn843r9qudwi13nyn5ru0.png)](https://asciinema.org/a/66ggwn843r9qudwi13nyn5ru0?autoplay=1&speed=2)

## What it is

connect-or-cut is a small library to inject into a program to prevent
it from connecting where it should not.

This is similar to a firewall, except that:

 * you do not need specific privileges to use it
 * only injected process and its sub-processes are affected, not
   the full system

## Installation

### Unix
Provided you have a C compiler, GNU or BSD make on a Unix box:

    $ git clone https://github.com/tgg/connect-or-cut.git
    $ cd connect-or-cut
    $ make os=$(uname -s)

If you want to compile it for 32 bits:

    $ make os=$(uname -s) bits=32

If you want to compile it with debug information:

    $ CFLAGS=-g make os=$(uname -s)

### Windows
Download binaries or compile using Visual Studio 2015.


## Using it
### Unix

    $ ./coc -d -a '*.google.com' -a '*.1e100.net' -b '*' firefox

Or to do it manually:

    $ LD_PRELOAD=./libconnect-or-cut.so COC_ALLOW='212.27.40.240:53;212.27.40.241:53;*.google.com;*.1e100.net' COC_BLOCK='*' firefox

This invokes firefox, allowing only outgoing connection to my ISP
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

### Windows

From a PowerShell console:

    PS> .\coc.ps1 -AllowDNS -Allow '*.google.com,*.1e100.net' -Block '*' '"C:\Program Files\Mozilla Firefox\firefox.exe"'

Or to do it manually, from cmd:

    > set COC_ALLOW=212.27.40.240:53;212.27.40.241:53;*.google.com;*.1e100.net
    > set COC_BLOCK=*
    > coc.exe "C:\Program Files\Mozilla Firefox\firefox.exe"

This invokes firefox, allowing only outgoing connection to my ISP
provided name servers, to all google.com and 1e100.net addresses.

Everything else will be blocked.

`coc.ps1` is a helper script that will set appropriate variables for you:

    PS> .\coc.ps1 -Help
	NAME
		coc.ps1

	SUMMARY
		coc configures connect-or-cut for the specified program.


	SYNTAX
		coc.ps1 [-AllowDNS] [-Allow <String[]>] [-Block <String[]>] [-Help] [-LogTarget <String[]>] [-LogPath <String>] 
		[-LogLevel <String>] [-Version] [-ProgramAndParams <String[]>] [<CommonParameters>]


	DESCRIPTION
		coc helper script sets environment variables then invokes coc.exe on the
		specified program. If no program is specified it outputs environment
		variables that need to be set to replicate the requested rules.

		Whenever a TCP connection is requested, the address where to connect
		is checked against the list of allowed hosts. If it's in it, the connection
		is allowed. If not, the list of blocked hosts is checked. If it's in it,
		the connection is rejected. If not, the connection is allowed.


	PARAMETERS
		-AllowDNS [<SwitchParameter>]
			Allow connections to DNS nameservers.

		-Allow <String[]>
			List of allowed hosts. Can be specified as host, host:port, host:port/bits or
			as a glob like "*.google.com". Specifying "*" allows every connection.

		-Block <String[]>
			List of blocked hosts. Can be specified as host, host:port, host:port/bits or
			as a glob like "*.google.com". Specifying "*" can block every connection.

		-Help [<SwitchParameter>]
			Shows this help message.

		-LogTarget <String[]>
			The location where to write logs. Valid values are: "stderr", which is the
			default; "syslog"; and "file". Multiple values can be specified.

		-LogPath <String>
			The path where to store logs when logging to files.

		-LogLevel <String>
			What to log. Can be one of: "silent" to discard logging, "error" to log
			only errors, "block" to also log blocked connections,"allow" to also
			log allowed connections, and "debug" to add debugging information. Default
			value is "block".

		-Version [<SwitchParameter>]
			Shows the version of connect-or-cut library.

		-ProgramAndParams <String[]>
			The program (and its arguments) to launch with the specified rules.
			If not specified, the cmd instructions to replicate these rules are displayed.

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
   * Windows 10 with Visual Studio 2015

   Portability patches welcome!

## Known issues

 * Aliases from /etc/hosts are not honoured. For instance if a machine is
   aliased and the long name conforms to a pattern it will be missed.
   The workaround for this is to add the short name to the rule set.

   This will require parsing /etc/hosts by hand, given that:
   - getnameinfo does not provide access to aliases
   - gethostbyaddr/gethostent, which do, are not reentrant on all platforms

 * On Windows, injection into UWP processes does not work. This means
   connect-or-cut cannot (yet?) work with Edge browser.

## News
 * Version 1.0.4
   * Add CIDR notation (contributed by saito tom)
   * Preliminary version for Windows

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
 * Make filtering algorithm configurable. For now it's always:
   * check against ALLOW list and ALLOW connection if it's in it;
   * else check against BLOCK list and BLOCK connection if it's in it;
   * else ALLOW by default
 * Implement `accept` filtering
