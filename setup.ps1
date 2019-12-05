Set-StrictMode -Version latest
$ErrorActionPreference = "Stop"


# Taken from psake https://github.com/psake/psake
<#
.SYNOPSIS
  This is a helper function that runs a scriptblock and checks the PS variable $lastexitcode
  to see if an error occcured. If an error is detected then an exception is thrown.
  This function allows you to run command-line programs without having to
  explicitly check the $lastexitcode variable.
.EXAMPLE
  exec { svn info $repository_trunk } "Error executing SVN. Please verify SVN command-line client is installed"
#>
function Exec
{
    [CmdletBinding()]
    param(
        [Parameter(Position=0,Mandatory=1)][scriptblock]$cmd,
        [Parameter(Position=1,Mandatory=0)][string]$errorMessage = ("Error executing command {0}" -f $cmd)
    )
    & $cmd
    if ($lastexitcode -ne 0) {
        throw ("Exec: " + $errorMessage)
    }
}

Try {

    # create a new virtual env
    Exec { virtualenv --clear venv } "ERROR virtualenv failed. Is virtualenv installed?"
    Exec { .\venv\Scripts\activate }

    # install west in the venv
    Exec { pip install west }

    # delete deps folder to get a clean checkout
    Remove-Item deps -Recurse -Force -ErrorAction SilentlyContinue

    # fetch dependencies defined in manifest/west.yaml
    Exec { west update }

    # hack until nrfutil v6 is released
    Exec { python scripts\fix-nrf-requirements.py }

    # install python dependencies in venv
    Exec { pip install --no-cache-dir -r deps\zephyr\scripts\requirements.txt }
    Exec { pip install --no-cache-dir -r deps\nrf\scripts\requirements.txt }
    Exec { pip install --no-cache-dir -r deps\mcuboot\scripts\requirements.txt }

} Catch {
    # tell the caller it has all gone wrong
    $host.SetShouldExit(-1)
    throw
}
