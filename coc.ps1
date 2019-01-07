<#
.SYNOPSIS
coc configures connect-or-cut for the specified program.

.DESCRIPTION
coc helper script sets environment variables then invokes coc.exe on the
specified program. If no program is specified it outputs environment
variables that need to be set to replicate the requested rules.

Whenever a TCP connection is requested, the address where to connect
is checked against the list of allowed hosts. If it's in it, the connection
is allowed. If not, the list of blocked hosts is checked. If it's in it,
the connection is rejected. If not, the connection is allowed.

.PARAMETER AllowDNS
Allow connections to DNS nameservers.

.PARAMETER Allow
List of allowed hosts. Can be specified as host, host:port, host:port/bits or
as a glob like "*.google.com". Specifying "*" allows every connection.

.PARAMETER Block
List of blocked hosts. Can be specified as host, host:port, host:port/bits or
as a glob like "*.google.com". Specifying "*" can block every connection.

.PARAMETER Help
Shows this help message.

.PARAMETER LogTarget
The location where to write logs. Valid values are: "stderr", which is the
default; "syslog"; and "file". Multiple values can be specified.

.PARAMETER LogPath
The path where to store logs when logging to files.

.PARAMETER LogLevel
What to log. Can be one of: "silent" to discard logging, "error" to log
only errors, "block" to also log blocked connections,"allow" to also
log allowed connections, and "debug" to add debugging information. Default
value is "block".

.PARAMETER Version
Shows the version of connect-or-cut library.

.PARAMETER ProgramAndParams
The program (and its arguments) to launch with the specified rules.
If not specified, the cmd instructions to replicate these rules are displayed.

#>
[CmdletBinding(PositionalBinding=$false)]
param (
	[switch]
	$AllowDNS = $false,

	[string[]]
	$Allow = @(),

	[string[]]
	$Block = @(),

	[switch]
	$Help,

	[ValidateSet("stderr","syslog","file")]
	[string[]]
	$LogTarget = @("stderr"),

	[string]
	$LogPath,

	[ValidateSet("silent","error","block","allow","debug")]
	[string]
	$LogLevel = "block",

	[switch]
	$Version,

	[Parameter(ValueFromRemainingArguments=$true)]
	[string[]]$ProgramAndParams
)

function Split-String([string]$s) {
    if ($s -eq $null) { @() }
    else { ,$s.Split(";") } # , prevents automatic conversion string[1] => string
}

function Join-String([string[]]$a) {
    if ($a.Count -eq 0) { $null }
    else { [string]::Join(";", $a) }
}

function Append-ProcessEnvironment([string]$name, [string[]]$values) {
    $current = Split-String([Environment]::GetEnvironmentVariable($name, "Process"))

	# De-duplication and removal of empty vars happens here
	$set = New-Object System.Collections.Generic.HashSet[string]
	foreach ($v in $current) {
		if ($v) { [void]$set.Add($v) }
	}
	foreach ($v in $values) {
		if ($v) { [void]$set.Add($v) }
	}

	$current = New-Object 'string[]' $set.Count
	$set.CopyTo($current)

    [Environment]::SetEnvironmentVariable($name, (Join-String($current)), "Process")
}

function Get-DnsServers() {
    try {
	    # This is not the right thing to do because it limits to IPv4.
	    # Patches welcome!
	    (Get-DnsClientServerAddress -AddressFamily IPv4 | where { $_.ServerAddresses.count -gt 0 }).ServerAddresses

    } catch [System.Management.Automation.CommandNotFoundException] {
        # Get-DnsClientServerAddress is available only in Windows >= 8
        # Adapted from Get-DNSServers.ps1 written by Sitaram Pamarthi (http://techibee.com)
        Write-Debug "Using Get-WmiObject to find DNS servers"

        try {
			$Networks = Get-WmiObject -Class Win32_NetworkAdapterConfiguration `
							-Filter IPEnabled=TRUE `
							-ComputerName localhost `
							-ErrorAction Stop
		} catch {
			Write-Error "Unable to find DNS server for localhost; aborting!"
			exit 1
		}

        $DNSServers = @()

		foreach ($Network in $Networks) {
			if ($Network.DNSServerSearchOrder) {
                $DNSServers += $Network.DNSServerSearchOrder
            }
        }

        $DNSServers
    }
}

$scriptPath = Split-Path -parent $MyInvocation.MyCommand.Definition
$LogTargets = @{"stderr" = 1; "syslog" = 2; "file" = 4}
$LogLevels = @{"silent" = 0; "error" = 1; "block" = 2; "allow" = 3; "debug" = 4}

if ($Help -eq $true) {
    help $MyInvocation.MyCommand.Definition -Detailed
    exit 0

} elseif ($Version -eq $true) {
    $lib = Join-Path -Path $scriptPath -ChildPath "connect-or-cut.dll"

    try {
        Write-Output "connect-or-cut $((Get-Item $lib -ErrorAction Stop).VersionInfo.FileVersion)"
        exit 0

    } catch [System.Management.Automation.ItemNotFoundException] {
        Write-Error "Cannot find connect-or-cut.dll in $scriptPath!" 
        exit 1
    }
}

if ($AllowDNS) {
	Get-DnsServers | ForEach-Object { 
		Append-ProcessEnvironment -name "COC_ALLOW" -values "${_}:53"
	}
}

Append-ProcessEnvironment -name "COC_ALLOW" -values $Allow
Append-ProcessEnvironment -name "COC_BLOCK" -values $Block

Write-Verbose "COC_ALLOW is: $env:COC_ALLOW"
Write-Verbose "COC_BLOCK is: $env:COC_BLOCK"

$env:COC_LOG_LEVEL=$LogLevels[$LogLevel]
if ($LogPath) { $env:COC_LOG_PATH="$LogPath" }

if ($LogTarget) {
	$acc = 0

	foreach ($target in $LogTarget) {
		$acc = $acc -bor $LogTargets[$target]
	}

	$env:COC_LOG_TARGET="$acc"
}

if ($ProgramAndParams) {
	$exe = Join-Path -Path $scriptPath -ChildPath "coc.exe"
	Write-Verbose "Starting: $ProgramAndParams"
	Start-Process -FilePath $exe -Wait -NoNewWindow -ArgumentList $ProgramAndParams

} else {
	# If no program was specified but we have COC_ variables set we print them.
	if ($env:COC_ALLOW -or $env:COC_BLOCK) {
		foreach ($e in @("COC_ALLOW","COC_BLOCK","COC_LOG_TARGET","COC_LOG_LEVEL","COC_LOG_PATH")) {
			$v = [Environment]::GetEnvironmentVariable($e, "Process")
			if ($v) { Write-Host "set $e=$v" }
		}
		exit 0

	} else {
		Write-Error "missing command!"
		help $MyInvocation.MyCommand.Definition
		exit 1
	}
}
