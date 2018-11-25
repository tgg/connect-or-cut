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
List of allowed hosts. Can be specified as host, host:port or as a glob like
"*.google.com". Specifying "*" allows every connection.

.PARAMETER Block
List of blocked hosts. Can be specified as host, host:port or as a glob like
"*.google.com". Specifying "*" can block every connection.

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

function Split-String([string]$s)
{
    if ($s -eq $null) { @() }
    else { ,$s.Split(";") } # , prevents automatic conversion string[1] => string
}

function Join-String([string[]]$a)
{
    if ($a.Count -eq 0) { $null }
    else { [string]::Join(";", $a) }
}

function Append-ProcessEnvironment([string]$name, [string[]]$values)
{
    $current = Split-String([Environment]::GetEnvironmentVariable($name, "Process"))
    [Environment]::SetEnvironmentVariable($name, (Join-String($current + $values)), "Process")
}

function Get-DnsServers()
{
	# This is not the right thing to do because it limits to IPv4.
	# Patches welcome!
	(Get-DnsClientServerAddress -AddressFamily IPv4 | where { $_.ServerAddresses.count -gt 0 }).ServerAddresses
}

$scriptPath = Split-Path -parent $MyInvocation.MyCommand.Definition
$LogTargets = @{"stderr" = 1; "syslog" = 2; "file" = 3}
$LogLevels = @{"silent" = 0; "error" = 1; "block" = 2; "allow" = 3; "debug" = 4}

if ($Help -eq $true)
{
    help $MyInvocation.MyCommand.Definition -Detailed
    exit 0
}
elseif ($Version -eq $true)
{
    $lib = Join-Path -Path $scriptPath -ChildPath "connect-or-cut.dll"

    try
    {
        Write-Output "connect-or-cut $((Get-Item $lib -ErrorAction Stop).VersionInfo.FileVersion)"
        exit 0
    }
    catch [System.Management.Automation.ItemNotFoundException]
    {
        Write-Output "Cannot find connect-or-cut.dll in $scriptPath; aborting" 
        exit 1
    }
}

Append-ProcessEnvironment -name "COC_ALLOW" -values $Allow
Append-ProcessEnvironment -name "COC_BLOCK" -values $Block

Write-Verbose "COC_ALLOW is now: $env:COC_ALLOW"
Write-Verbose "COC_BLOCK is now: $env:COC_BLOCK"

$env:COC_LOG_LEVEL=$LogLevels[$LogLevel]
if ($LogPath) { $env:COC_LOG_PATH="$LogPath" }

if ($LogTarget) {
	$acc = 0

	foreach ($target in $LogTarget) {
		$acc = $acc -bor $LogTargets[$target]
	}

	$env:COC_LOG_TARGET="$acc"
}

# TODO add DNS servers if requested

$exe = Join-Path -Path $scriptPath -ChildPath "coc.exe"
Write-Verbose "program and params: $ProgramAndParams"
Start-Process $exe $ProgramAndParams
