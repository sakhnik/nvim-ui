param(
    [string]$script
)

& c:\tools\msys64\usr\bin\bash -l -c "${env:WORKSPACE_MSYS2}/scripts/${script}"
