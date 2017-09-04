<#
.SYNOPSIS
coc configures connect-or-cut for the specified program.

.DESCRIPTION
coc helper script sets environment variables then invokes coc.exe on the
specified program. If no program is specified it outputs environment
variables that need to be set to replicate the requested configuration.

.PARAMETER AllowDNS
Allow connections to DNS nameservers

.PARAMETER Allow
List of allowed host. Can be specified as host, host:port or as a glob like
"*.google.com". Specifying "*" allows every connection.

.PARAMETER Block
List of blocked hosts. Can be specified as host, host:port or as a glob like
"*.google.com". Specifying "*" can block every connection.

.PARAMETER Help
Shows this help message

#>
[CmdletBinding()]
param (
[boolean]$AllowDNS = $false,
[string[]]$Allow = @(),
[string[]]$Block = @(),
[switch]$Help,
[ValidateSet("stderr","syslog","file")]
[string[]]$LogTarget = @("stderr"),
[string]$LogPath,
[ValidateSet("silent","error","block","allow","debug")]
[string]$LogLevel = "block",
[switch]$Version,
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

$scriptPath = Split-Path -parent $MyInvocation.MyCommand.Definition
$LogTargets = @{"stderr" = 1; "syslog" = 2; "file" = 3}
$LogLevels = @{"silent" = 0; "error" = 1; "block" = 2; "allow" = 3; "debug" = 4}

if ($Help -eq $True)
{
    help $MyInvocation.MyCommand.Definition -Detailed
    exit 0
}
elseif ($Version -eq $True)
{
    $lib = Join-Path -Path $scriptPath -ChildPath "connect-or-cut.dll"
    try
    {
        Write-Output "$((Get-Item $lib -ErrorAction Stop).VersionInfo.FileVersion)"
        exit 0
    }
    catch [System.Management.Automation.ItemNotFoundException]
    {
        Write-Output "Cannot find connect-or-cut.dll in $scriptPath; aborting" 
        exit 1
    }
}

Write-Verbose "COC_ALLOW was: $env:COC_ALLOW"
Write-Verbose "COC_BLOCK was: $env:COC_BLOCK"

Append-ProcessEnvironment -name "COC_ALLOW" -values $Allow
Append-ProcessEnvironment -name "COC_BLOCK" -values $Block

Write-Verbose "COC_ALLOW is now: $env:COC_ALLOW"
Write-Verbose "COC_BLOCK is now: $env:COC_BLOCK"


