<#
.SYNOPSIS
    GDEI 校园网（H3C iMC Portal）自动认证 - 原生 PowerShell 实现（无需 Python）。

.DESCRIPTION
    抓包结论（2026-11-21 实测 /portal/2021/login.jsp）：
      门户: https://nac.gdei.edu.cn  (备用直连 IP 210.38.64.39)
      NAS : 192.168.100.8
      登录: POST /portal/pws?t=li&ifEmailAuth=false
      注销: POST /portal/pws?t=lo
      密码: 标准 Base64(明文)
      返回: Base64 -> URL 解码 -> JSON；errorNumber == "1" 为成功。

    本脚本与 campus_core.py 共用同目录 config.json，口令混淆规则一致（可逆，非强加密）。
    需要 PowerShell 7（pwsh）；-SkipCertificateCheck 仅 PS7 支持。

.PARAMETER Action
    gui（默认，图形界面）| login | logout | status

.EXAMPLE
    pwsh -ExecutionPolicy Bypass -File portal.ps1 -Action gui
    pwsh -ExecutionPolicy Bypass -File portal.ps1 -Action login
#>

param(
    [ValidateSet("gui", "login", "logout", "status")]
    [string]$Action = "gui"
)

$ErrorActionPreference = "Stop"

# --------------------------------------------------------------------------- #
# 兼容性: PowerShell 7 支持 -SkipCertificateCheck；5.1 用证书回调跳过校验
# --------------------------------------------------------------------------- #
$Script:IsPs7 = $PSVersionTable.PSVersion.Major -ge 7
if (-not $Script:IsPs7) {
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        [Net.ServicePointManager]::ServerCertificateCustomValidationCallback = { $true }
    } catch { }
}

# --------------------------------------------------------------------------- #
# 常量
# --------------------------------------------------------------------------- #
$Script:DefaultHost = "nac.gdei.edu.cn"
$Script:DefaultIp   = "210.38.64.39"
$Script:DefaultNas  = "192.168.100.8"
$Script:Key         = [Text.Encoding]::UTF8.GetBytes("gdei-portal-2026")

$Script:EntryPath    = "/portal?nasip={0}"
$Script:LoginJspPath = "/portal/2021/login.jsp?nasip={0}"
$Script:LoginPath    = "/portal/pws?t=li&ifEmailAuth=false"
$Script:LogoutPath   = "/portal/pws?t=lo"

# 表单默认隐藏域（顺序与浏览器 serializeArray() 一致）
$Script:DefaultFields = [ordered]@{
    "freeOTP_secret_hidden" = ""
    "userName"              = ""
    "userPwd"               = ""
    "userurl"               = ""
    "userip"                = ""
    "basip"                 = $Script:DefaultNas
    "language"              = "English"
    "usermac"               = "null"
    "wlannasid"             = ""
    "wlanssid"              = ""
    "entrance"              = "null"
    "loginVerifyCode"       = ""
    "userDynamicPwddd"      = ""
    "customPageId"          = "105"
    "send_dynamic_pwd_type" = "0"
    "pwdMode"               = "0"
    "portalProxyIP"         = "210.38.64.39"
    "portalProxyPort"       = "50200"
    "dcPwdNeedEncrypt"      = "1"
    "assignIpType"          = "0"
    "appRootUrl"            = "https://nac.gdei.edu.cn/portal/"
    "manualUrl"             = ""
    "manualUrlEncryptKey"   = ""
}

function Get-ConfigPath {
    return (Join-Path $PSScriptRoot "config.json")
}

# --------------------------------------------------------------------------- #
# 口令混淆（与 Python 版一致：XOR 固定密钥 -> Base64）
# --------------------------------------------------------------------------- #
function Protect-Secret {
    param([string]$Plain)
    if ([string]::IsNullOrEmpty($Plain)) { return "" }
    $b = [Text.Encoding]::UTF8.GetBytes($Plain)
    for ($i = 0; $i -lt $b.Length; $i++) {
        $b[$i] = $b[$i] -bxor $Script:Key[$i % $Script:Key.Length]
    }
    return [Convert]::ToBase64String($b)
}

function Unprotect-Secret {
    param([string]$Ob)
    if ([string]::IsNullOrEmpty($Ob)) { return "" }
    try {
        $b = [Convert]::FromBase64String($Ob)
        for ($i = 0; $i -lt $b.Length; $i++) {
            $b[$i] = $b[$i] -bxor $Script:Key[$i % $Script:Key.Length]
        }
        return [Text.Encoding]::UTF8.GetString($b)
    } catch { return "" }
}

function Get-Config {
    $cfg = [ordered]@{
        username     = ""
        password_ob  = ""
        host         = $Script:DefaultHost
        ip           = $Script:DefaultIp
        scheme       = "https"
        port         = 443
        nasip        = $Script:DefaultNas
    }
    $p = Get-ConfigPath
    if (Test-Path $p) {
        try {
            $json = Get-Content $p -Raw -Encoding UTF8 | ConvertFrom-Json
            foreach ($prop in $json.PSObject.Properties) { $cfg[$prop.Name] = $prop.Value }
        } catch { }
    }
    return [pscustomobject]$cfg
}

function Save-Config {
    param($Cfg)
    $Cfg | ConvertTo-Json -Depth 5 | Set-Content (Get-ConfigPath) -Encoding UTF8
}

# --------------------------------------------------------------------------- #
# HTTP 会话（IWR，自动 Cookie / gzip / 跳转，跳过证书校验）
# --------------------------------------------------------------------------- #
function New-PortalContext {
    param($Cfg)
    $session = New-Object Microsoft.PowerShell.Commands.WebRequestSession
    $scheme = if ($Cfg.scheme) { $Cfg.scheme } else { "https" }
    $port = if ($Cfg.port) { [int]$Cfg.port } else { 443 }
    $base = "$scheme`://$($Cfg.host)"
    if (($scheme -eq "https" -and $port -ne 443) -or ($scheme -eq "http" -and $port -ne 80)) {
        $base += ":$port"
    }
    return [pscustomobject]@{ Session = $session; Base = $base }
}

function Invoke-PortalRequest {
    param($Ctx, [string]$Path, [string]$Body)
    $url = $Ctx.Base + $Path
    $params = @{
        Uri = $url
        WebSession = $Ctx.Session
        UseBasicParsing = $true
        TimeoutSec = 10
    }
    if ($Script:IsPs7) { $params["SkipCertificateCheck"] = $true }
    if ($Body) {
        $params["Method"] = "Post"
        $params["Body"] = $Body
        $params["ContentType"] = "application/x-www-form-urlencoded; charset=UTF-8"
        $params["Headers"] = @{ "X-Requested-With" = "XMLHttpRequest" }
    }
    return Invoke-WebRequest @params
}

function Build-FormBody {
    param($Cfg, [string]$Username, [string]$Password)
    $fields = [ordered]@{}
    foreach ($k in $Script:DefaultFields.Keys) { $fields[$k] = $Script:DefaultFields[$k] }
    $fields["userName"] = $Username
    $fields["userPwd"]  = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($Password))
    $fields["basip"]    = $Cfg.nasip
    $pairs = foreach ($k in $fields.Keys) {
        "{0}={1}" -f [Uri]::EscapeDataString($k), [Uri]::EscapeDataString([string]$fields[$k])
    }
    return ($pairs -join "&")
}

function Read-PortalPayload {
    param([string]$Text)
    if ([string]::IsNullOrWhiteSpace($Text)) { return $null }
    try {
        $bytes = [Convert]::FromBase64String($Text.Trim())
        $dec = [Text.Encoding]::UTF8.GetString($bytes)
        $dec = [Uri]::UnescapeDataString($dec)
        try { return ($dec | ConvertFrom-Json) } catch { return $dec }
    } catch { return $Text }
}

function Get-ErrorMsg {
    param($Obj)
    if ($null -eq $Obj) { return "（空响应）" }
    if ($Obj -is [string]) { return $Obj }
    $ed = $Obj.e_d
    if ($ed -and $Obj.$ed) { return [string]$Obj.$ed }
    if ($Obj.errorMessage) { return [string]$Obj.errorMessage }
    return ($Obj | ConvertTo-Json -Compress)
}

# --------------------------------------------------------------------------- #
# 业务动作
# --------------------------------------------------------------------------- #
function Test-Online {
    try {
        $r = Invoke-WebRequest -Uri "http://www.msftconnecttest.com/connecttest.txt" `
            -UseBasicParsing -TimeoutSec 6 -Headers @{ "User-Agent" = "Microsoft NCSI" }
        return ($r.Content -match "Microsoft")
    } catch { return $false }
}

function Invoke-PortalLogin {
    param($Cfg)
    $username = $Cfg.username
    $password = Unprotect-Secret $Cfg.password_ob
    if ([string]::IsNullOrEmpty($username) -or [string]::IsNullOrEmpty($password)) {
        return @{ Ok = $false; Message = "未配置账号或密码，请先在图形界面中保存。" }
    }

    $ctx = New-PortalContext $Cfg
    try {
        [void](Invoke-PortalRequest $ctx ($Script:EntryPath -f $Cfg.nasip) "")
        [void](Invoke-PortalRequest $ctx ($Script:LoginJspPath -f $Cfg.nasip) "")

        $body = Build-FormBody -Cfg $Cfg -Username $username -Password $password
        $resp = Invoke-PortalRequest $ctx $Script:LoginPath $body
        $obj = Read-PortalPayload ([string]$resp.Content)

        if ($null -ne $obj -and [string]$obj.errorNumber -eq "1") {
            return @{ Ok = $true; Message = "认证成功，已上线。" }
        }
        return @{ Ok = $false; Message = ("认证失败：" + (Get-ErrorMsg $obj)) }
    } catch {
        return @{ Ok = $false; Message = "请求异常：$_" }
    }
}

function Invoke-PortalLogout {
    param($Cfg)
    $ctx = New-PortalContext $Cfg
    try {
        [void](Invoke-PortalRequest $ctx ($Script:EntryPath -f $Cfg.nasip) "")
        [void](Invoke-PortalRequest $ctx ($Script:LoginJspPath -f $Cfg.nasip) "")

        $body = Build-FormBody -Cfg $Cfg -Username $Cfg.username -Password (Unprotect-Secret $Cfg.password_ob)
        $resp = Invoke-PortalRequest $ctx $Script:LogoutPath $body
        $obj = Read-PortalPayload ([string]$resp.Content)

        if ($null -ne $obj -and [string]$obj.errorNumber -eq "1") {
            return @{ Ok = $true; Message = "已下线。" }
        }
        return @{ Ok = $false; Message = ("注销结果：" + (Get-ErrorMsg $obj)) }
    } catch {
        return @{ Ok = $false; Message = "请求异常：$_" }
    }
}

# --------------------------------------------------------------------------- #
# 控制台模式
# --------------------------------------------------------------------------- #
function Invoke-Console {
    param([string]$Act)
    $cfg = Get-Config
    switch ($Act) {
        "status" { Write-Host ($(if (Test-Online) { "在线" } else { "未上线（需要认证）" })) }
        "login"  { $r = Invoke-PortalLogin $cfg; Write-Host $r.Message; if (-not $r.Ok) { exit 1 } }
        "logout" { $r = Invoke-PortalLogout $cfg; Write-Host $r.Message; if (-not $r.Ok) { exit 1 } }
    }
}

# --------------------------------------------------------------------------- #
# 图形界面（WinForms，无需 Python）
# --------------------------------------------------------------------------- #
function Invoke-Gui {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    [System.Windows.Forms.Application]::EnableVisualStyles()

    $cfg = Get-Config
    $secret = Unprotect-Secret $cfg.password_ob

    $form = New-Object System.Windows.Forms.Form
    $form.Text = "GDEI 校园网自动认证"
    $form.FormBorderStyle = "FixedDialog"
    $form.MaximizeBox = $false
    $form.Width = 460
    $form.Height = 430
    $form.StartPosition = "CenterScreen"
    $font = New-Object System.Drawing.Font("Microsoft YaHei UI", 9)

    function New-Label($Text, $X, $Y, $W) {
        $l = New-Object System.Windows.Forms.Label
        $l.Text = $Text; $l.Location = New-Object System.Drawing.Point($X, $Y)
        $l.Width = $W; $l.Font = $font
        $form.Controls.Add($l); return $l
    }
    function New-TextBox($X, $Y, $W, $Text) {
        $t = New-Object System.Windows.Forms.TextBox
        $t.Location = New-Object System.Drawing.Point($X, $Y); $t.Width = $W
        $t.Text = $Text; $t.Font = $font
        $form.Controls.Add($t); return $t
    }

    New-Label "账号：" 12 16 60
    $txtUser = New-TextBox 78 13 240 $cfg.username

    New-Label "密码：" 12 50 60
    $txtPwd = New-TextBox 78 47 240 $secret
    $txtPwd.PasswordChar = "*"

    $chkShow = New-Object System.Windows.Forms.CheckBox
    $chkShow.Text = "显示"; $chkShow.Location = New-Object System.Drawing.Point(330, 47); $chkShow.Font = $font
    $chkShow.Add_CheckedChanged({ $txtPwd.PasswordChar = $(if ($chkShow.Checked) { [char]0 } else { [char]'*' }) })
    $form.Controls.Add($chkShow)

    $chkRemember = New-Object System.Windows.Forms.CheckBox
    $chkRemember.Text = "记住账号密码"
    $chkRemember.Location = New-Object System.Drawing.Point(78, 78)
    $chkRemember.Font = $font
    $chkRemember.Checked = -not [string]::IsNullOrEmpty($cfg.password_ob)
    $form.Controls.Add($chkRemember)

    $grp = New-Object System.Windows.Forms.GroupBox
    $grp.Text = "高级设置（一般无需修改）"
    $grp.Location = New-Object System.Drawing.Point(12, 104); $grp.Width = 420; $grp.Height = 86
    $grp.Font = $font; $form.Controls.Add($grp)

    $lblHost = New-Label "门户主机" 10 26 70
    $txtHost = New-TextBox 86 23 150 $cfg.host
    $lblIp = New-Label "备用IP" 250 26 50
    $txtIp = New-TextBox 300 23 110 $cfg.ip
    $lblNas = New-Label "NAS IP" 10 54 70
    $txtNas = New-TextBox 86 51 150 $cfg.nasip
    $grp.Controls.AddRange(@($lblHost, $txtHost, $lblIp, $txtIp, $lblNas, $txtNas))

    $btnStatus = New-Object System.Windows.Forms.Button
    $btnStatus.Text = "检测状态"; $btnStatus.Location = New-Object System.Drawing.Point(12, 200); $btnStatus.Width = 90; $btnStatus.Font = $font
    $btnLogin = New-Object System.Windows.Forms.Button
    $btnLogin.Text = "上线"; $btnLogin.Location = New-Object System.Drawing.Point(112, 200); $btnLogin.Width = 90; $btnLogin.Font = $font
    $btnLogout = New-Object System.Windows.Forms.Button
    $btnLogout.Text = "下线"; $btnLogout.Location = New-Object System.Drawing.Point(212, 200); $btnLogout.Width = 90; $btnLogout.Font = $font
    $btnPortal = New-Object System.Windows.Forms.Button
    $btnPortal.Text = "打开门户"; $btnPortal.Location = New-Object System.Drawing.Point(312, 200); $btnPortal.Width = 90; $btnPortal.Font = $font
    $form.Controls.AddRange(@($btnStatus, $btnLogin, $btnLogout, $btnPortal))

    $txtLog = New-Object System.Windows.Forms.TextBox
    $txtLog.Multiline = $true; $txtLog.ReadOnly = $true; $txtLog.ScrollBars = "Vertical"
    $txtLog.Location = New-Object System.Drawing.Point(12, 238); $txtLog.Width = 420; $txtLog.Height = 110
    $txtLog.Font = New-Object System.Drawing.Font("Consolas", 9)
    $form.Controls.Add($txtLog)

    $lblStatus = New-Object System.Windows.Forms.Label
    $lblStatus.Text = "状态：未知"; $lblStatus.Location = New-Object System.Drawing.Point(12, 356); $lblStatus.Font = $font
    $form.Controls.Add($lblStatus)

    function Write-Log($Msg) {
        $txtLog.AppendText($Msg + "`r`n")
    }

    function Build-CfgFromForm {
        $c = [ordered]@{
            username    = $txtUser.Text.Trim()
            password_ob = $(if ($chkRemember.Checked) { Protect-Secret $txtPwd.Text } else { "" })
            host        = $(if ($txtHost.Text.Trim()) { $txtHost.Text.Trim() } else { $Script:DefaultHost })
            ip          = $(if ($txtIp.Text.Trim()) { $txtIp.Text.Trim() } else { $Script:DefaultIp })
            scheme      = "https"
            port        = 443
            nasip       = $(if ($txtNas.Text.Trim()) { $txtNas.Text.Trim() } else { $Script:DefaultNas })
        }
        return [pscustomobject]$c
    }

    function Refresh-Status {
        if (Test-Online) {
            $lblStatus.Text = "状态：已联网"; $lblStatus.ForeColor = [System.Drawing.Color]::FromArgb(26, 127, 55)
        } else {
            $lblStatus.Text = "状态：未上线（需要认证）"; $lblStatus.ForeColor = [System.Drawing.Color]::FromArgb(192, 57, 43)
        }
    }

    $btnStatus.Add_Click({ Refresh-Status; Write-Log ($(if (Test-Online) { "当前：在线" } else { "当前：未上线（需要认证）" })) })
    $btnLogin.Add_Click({
        if ([string]::IsNullOrEmpty($txtUser.Text.Trim()) -or [string]::IsNullOrEmpty($txtPwd.Text)) {
            [System.Windows.Forms.MessageBox]::Show("请输入账号和密码。") | Out-Null; return
        }
        $c = Build-CfgFromForm; Save-Config $c
        Write-Log "—— 上线 ——"
        $r = Invoke-PortalLogin $c
        Write-Log $r.Message
        Refresh-Status
        if (-not $r.Ok) { [System.Windows.Forms.MessageBox]::Show($r.Message) | Out-Null }
    })
    $btnLogout.Add_Click({
        $c = Build-CfgFromForm; Save-Config $c
        Write-Log "—— 下线 ——"
        $r = Invoke-PortalLogout $c
        Write-Log $r.Message
        Refresh-Status
    })
    $btnPortal.Add_Click({
        Start-Process ("https://{0}/portal?nasip={1}" -f $txtHost.Text.Trim(), $txtNas.Text.Trim())
    })

    Refresh-Status
    [System.Windows.Forms.Application]::Run($form)
}

# --------------------------------------------------------------------------- #
switch ($Action) {
    "gui"   { Invoke-Gui }
    default { Invoke-Console $Action }
}
