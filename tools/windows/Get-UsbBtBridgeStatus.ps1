[CmdletBinding()]
param(
    [string]$VendorId = "0669",
    [string[]]$ProductIds = @("0225", "0226")
)

$ErrorActionPreference = "Stop"

function Get-HardwareIds {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Device
    )

    if ($null -eq $Device.HardwareID) {
        return @()
    }

    if ($Device.HardwareID -is [System.Array]) {
        return $Device.HardwareID
    }

    return @($Device.HardwareID)
}

function Test-BridgeHardwareId {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$HardwareIds
    )

    foreach ($hardwareId in $HardwareIds) {
        foreach ($productId in $ProductIds) {
            if ($hardwareId -match "VID_$VendorId&PID_$productId") {
                return $true
            }
        }
    }

    return $false
}

$bridgePnP = Get-CimInstance Win32_PnPEntity | Where-Object {
    $hardwareIds = @(Get-HardwareIds $_)
    if ($hardwareIds.Count -eq 0) {
        return $false
    }

    return (Test-BridgeHardwareId $hardwareIds)
}

if (-not $bridgePnP) {
    Write-Host "No USB Audio + BT bridge board found." -ForegroundColor Yellow
    Write-Host "Expected VID $VendorId and PID(s): $($ProductIds -join ', ')"
    exit 0
}

$bridgeComPorts = $bridgePnP | Where-Object {
    $_.Name -match '\(COM\d+\)' -or $_.PNPClass -eq 'Ports'
}

$bridgeAudio = $bridgePnP | Where-Object {
    $_.PNPClass -eq 'AudioEndpoint' -or $_.Service -match 'usbaudio'
}

$bridgeBluetooth = $bridgePnP | Where-Object {
    $_.PNPClass -eq 'Bluetooth' -or $_.Service -match 'BTH'
}

Write-Host "Matched bridge-related PnP devices:" -ForegroundColor Cyan
$bridgePnP |
    Sort-Object Name |
    Select-Object Name, PNPClass, Service, DeviceID |
    Format-Table -AutoSize

Write-Host ""
Write-Host "COM interfaces:" -ForegroundColor Cyan
if ($bridgeComPorts) {
    $bridgeComPorts |
        Sort-Object Name |
        Select-Object Name, Service |
        Format-Table -AutoSize
} else {
    Write-Host "None"
}

Write-Host ""
Write-Host "Audio interfaces:" -ForegroundColor Cyan
if ($bridgeAudio) {
    $bridgeAudio |
        Sort-Object Name |
        Select-Object Name, Service |
        Format-Table -AutoSize
} else {
    Write-Host "None"
}

Write-Host ""
Write-Host "Bluetooth interfaces on this board:" -ForegroundColor Cyan
if ($bridgeBluetooth) {
    $bridgeBluetooth |
        Sort-Object Name |
        Select-Object Name, Service |
        Format-Table -AutoSize
} else {
    Write-Host "None"
    Write-Host ""
    Write-Host "Current firmware exposes USB Audio + CDC ACM on Windows." -ForegroundColor Yellow
    Write-Host "Windows can use the microphone and COM port, but it will not create a system Bluetooth radio from the CDC ACM port by itself." -ForegroundColor Yellow
    Write-Host "See tools/windows/README.md for the two supported Windows directions." -ForegroundColor Yellow
}
