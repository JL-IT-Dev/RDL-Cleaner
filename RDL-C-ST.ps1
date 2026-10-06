$ErrorActionPreference = "Stop"

$TaskName = "RDL Cleaner - User Logon"
$ExecutablePath = "C:\Program Files\Redial\RDLCleaner\RDL_Cleaner.exe"
$WorkingDirectory = "C:\Program Files\Redial\RDLCleaner"
$CurrentUser = [System.Security.Principal.WindowsIdentity\]::GetCurrent().Name

if ($CurrentUser -eq "NT AUTHORITY\SYSTEM") {
    Write-Error "El script debe ejecutarse en contexto de usuario, no como SYSTEM."
    exit 1
}

if (-not (Test-Path -LiteralPath $ExecutablePath -PathType Leaf)) {
    Write-Error "No se encontró el ejecutable: $ExecutablePath"
    exit 1
}

$Action = New-ScheduledTaskAction `
    -Execute $ExecutablePath `
    -Argument "--execute" `
    -WorkingDirectory $WorkingDirectory

$Trigger = New-ScheduledTaskTrigger `
    -AtLogOn `
    -User $CurrentUser

$Trigger.Delay = "PT30S"

$Principal = New-ScheduledTaskPrincipal `
    -UserId $CurrentUser `
    -LogonType Interactive `
    -RunLevel Limited

$Settings = New-ScheduledTaskSettingsSet `
    -AllowStartIfOnBatteries `
    -DontStopIfGoingOnBatteries `
    -StartWhenAvailable `
    -ExecutionTimeLimit (New-TimeSpan -Minutes 15)

$Settings.MultipleInstances = "IgnoreNew"

Register-ScheduledTask `
    -TaskName $TaskName `
    -Description "Ejecuta RDL Cleaner al iniciar sesión del usuario." `
    -Action $Action `
    -Trigger $Trigger `
    -Principal $Principal `
    -Settings $Settings `
    -Force | Out-Null

Write-Host "Tarea creada: $TaskName"
Write-Host "Usuario: $CurrentUser"

exit 0