[CmdletBinding()]
param(
    [string]$ComPort,
    [int]$BaudRate = 115200,
    [int]$TimeoutMs = 3000
)

$ErrorActionPreference = "Stop"

function Find-BridgeComPort {
    $serialPorts = Get-CimInstance Win32_SerialPort | Where-Object {
        $_.PNPDeviceID -match 'VID_0669&PID_(0225|0226)'
    }

    if (-not $serialPorts) {
        return $null
    }

    return ($serialPorts | Select-Object -First 1 -ExpandProperty DeviceID)
}

function Read-ByteExact {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.Ports.SerialPort]$Port
    )

    return [byte]$Port.ReadByte()
}

function Read-BytesExact {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.Ports.SerialPort]$Port,
        [Parameter(Mandatory = $true)]
        [int]$Count
    )

    $buffer = New-Object byte[] $Count
    $offset = 0

    while ($offset -lt $Count) {
        $read = $Port.Read($buffer, $offset, $Count - $offset)
        if ($read -le 0) {
            throw "Serial read timed out."
        }
        $offset += $read
    }

    return $buffer
}

if (-not $ComPort) {
    $ComPort = Find-BridgeComPort
}

if (-not $ComPort) {
    Write-Host "No bridge CDC ACM COM port found." -ForegroundColor Yellow
    Write-Host "Expected a USB serial device with VID 0669 and PID 0225 or 0226." -ForegroundColor Yellow
    exit 1
}

$port = New-Object System.IO.Ports.SerialPort $ComPort, $BaudRate, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$port.ReadTimeout = $TimeoutMs
$port.WriteTimeout = $TimeoutMs
$port.DtrEnable = $true
$port.RtsEnable = $true

try {
    $port.Open()
    $port.DiscardInBuffer()
    $port.DiscardOutBuffer()
    Start-Sleep -Milliseconds 200

    $hciReset = [byte[]](0x01, 0x03, 0x0C, 0x00)
    $port.Write($hciReset, 0, $hciReset.Length)

    $packetType = Read-ByteExact -Port $port
    if ($packetType -ne 0x04) {
        throw ("Unexpected HCI packet type 0x{0:X2}" -f $packetType)
    }

    $eventHeader = Read-BytesExact -Port $port -Count 2
    $eventCode = $eventHeader[0]
    $parameterLength = [int]$eventHeader[1]
    $parameters = Read-BytesExact -Port $port -Count $parameterLength

    Write-Host ("Received HCI event on {0}" -f $ComPort) -ForegroundColor Cyan
    Write-Host ("PacketType=0x{0:X2} EventCode=0x{1:X2} ParamLength={2}" -f $packetType, $eventCode, $parameterLength)
    Write-Host ("Parameters={0}" -f (($parameters | ForEach-Object { '{0:X2}' -f $_ }) -join ' '))

    if ($eventCode -eq 0x0E -and $parameterLength -ge 4) {
        $opcode = [uint16]($parameters[1] -bor ($parameters[2] -shl 8))
        $status = [byte]$parameters[3]

        if ($opcode -eq 0x0C03 -and $status -eq 0x00) {
            Write-Host "Bluetooth HCI bridge is responding correctly." -ForegroundColor Green
            exit 0
        }
    }

    throw "Received an HCI event, but it was not a successful HCI Reset Command Complete."
}
finally {
    if ($port.IsOpen) {
        $port.Close()
    }
}
