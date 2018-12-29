$scriptPath = Split-Path -parent $MyInvocation.MyCommand.Definition
cd $scriptPath

# TODO: Sanity checks

$LogTargets = @{"stderr" = 1; "syslog" = 2; "file" = 3}
$LogLevels = @{"silent" = 0; "error" = 1; "block" = 2; "allow" = 3; "debug" = 4}
$ecount = 0

function Test-Rule([string]$machine, [string]$port, [string]$level, [string]$allow, [string]$block) {
	$env:COC_ALLOW=$allow
	$env:COC_BLOCK=$block
	$env:COC_LOG_LEVEL=$LogLevels[$level]
	$env:COC_LOG_TARGET=$LogTargets["stderr"]
	Write-Host -NoNewline "Checking if we $level connection to ${machine}:$port with rules: {allow=$allow; block=$block} "
	&{ .\coc.exe .\tcpcontest.exe $machine $-port } 2>&1 | Select-String $level.ToUpper() >$null
	if (!$?) { 
		Write-Host "KO"
		$ecount++ 
	} else {
		Write-Host "OK"
	}
}

function Test-Allow([string]$machine, [string]$port, [string]$allow, [string]$block) {
	Test-Rule -machine $machine -port $port -level "allow" -allow $allow -block $block
}

function Test-Block([string]$machine, [string]$port, [string]$allow, [string]$block) {
	Test-Rule -machine $machine -port $port -level "block" -allow $allow -block $block
}

Test-Allow -machine 127.0.0.1 -port 80 -allow 127.0.0.1 -block "*"
Test-Allow -machine ::1 -port 50 -allow ::1 -block "*"
Test-Allow -machine ::1 -port 50 -allow [::1] -block "*"
Test-Allow -machine ::1 -port 50 -allow [::1]:50 -block "*"
Test-Block -machine ::1 -port 50 -allow [::1]:49 -block "*"
Test-Allow -machine ::1 -port 21 -allow [::1]:ftp -block "*"
Test-Allow -machine a::f -port 50 -allow a::f -block "*"
Test-Block -machine a::f -port 50 with args -b [a::f]:50
Test-Allow -machine 2001:db8:0:1:1:1:1:1 -port 50 -allow 2001:db8:0:1:1:1:1:1 -block "*"
Test-Allow -machine 127.0.0.1 -port 50 -allow 127.0.0.1:50 -block "*"
Test-Allow -machine 127.0.0.1 -port 50 -allow 127.0.0.1 -block "*"
Test-Block -machine 127.0.0.1 -port 50 -allow 127.0.0.1:49 -block "*"
Test-Allow -machine 127.0.0.1 -port 50
Test-Allow -machine 127.0.0.1 -port 50 -allow "*"
Test-Block -machine 127.0.0.1 -port 50 -block "*"
Test-Allow -machine local-machine -port 50 -allow local-machine -block "*"
Test-Block -machine local-machine -port 50 -allow localhost:49 -block "*"
Test-Allow -machine 127.0.0.1 -port 80 -allow ::ffff:7f00:1 -block "*"
Test-Block -machine 127.0.0.2 -port 80 -allow ::ffff:7f00:1 -block "*"
Test-Allow -machine 127.0.0.1 -port 80 -allow [::ffff:7f00:1]:80 -block "*"
Test-Block -machine 127.0.0.1 -port 80 -allow [::ffff:7f00:1]:81 -block "*"
Test-Allow -machine ::ffff:7f00:1 -port 80 -allow 127.0.0.1 -block "*"
Test-Block -machine ::ffff:7f00:2 -port 80 -allow 127.0.0.1 -block "*"
Test-Allow -machine ::ffff:7f00:1 -port 80 -allow 127.0.0.1:80 -block "*"
Test-Block -machine ::ffff:7f00:1 -port 80 -allow 127.0.0.1:81 -block "*"
#Test-Allow -machine www.google.com -port 80 with args -d -a \'*.google.com\' -a \'*.1e100.net\' -block "*"