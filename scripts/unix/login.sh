#!/bin/sh
# ============================================================
# GDEI 校园网（H3C iMC Portal）自动认证 - Linux / macOS / OpenWrt 版
# 仅依赖 curl、base64、od、sed（OpenWrt 自带）。
#
# 用法:
#   ./login.sh status     检测是否已联网
#   ./login.sh login      上线（默认）
#   ./login.sh logout     下线
#
# 账号密码读取顺序: 环境变量 PORTAL_USER / PORTAL_PASS -> config.json
# 与 Windows 版共用同目录 config.json（口令为可逆混淆，非明文）。
# ============================================================

DIR="$(cd "$(dirname "$0")" && pwd)"
CONFIG="$DIR/config.json"

HOST="nac.gdei.edu.cn"
IP="210.38.64.39"
NASIP="192.168.100.8"
KEY="gdei-portal-2026"
UA="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36"

# ---------- 配置读取（无 jq，用 sed 提取） ----------
cfg_get() {
    [ -f "$CONFIG" ] || return 0
    sed -n 's/.*"'"$1"'"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$CONFIG" | head -n 1
}

cfg_host="$(cfg_get host)";   [ -n "$cfg_host" ] && HOST="$cfg_host"
cfg_ip="$(cfg_get ip)";       [ -n "$cfg_ip" ] && IP="$cfg_ip"
cfg_nas="$(cfg_get nasip)";   [ -n "$cfg_nas" ] && NASIP="$cfg_nas"
USER="${PORTAL_USER:-$(cfg_get username)}"

# ---------- 口令混淆解码（与 Python / PowerShell 一致: XOR 密钥 -> Base64） ----------
decode_ob() {
    ob="$1"
    [ -z "$ob" ] && { printf ''; return; }
    hex="$(printf '%s' "$ob" | base64 -d 2>/dev/null | od -An -tu1 -v | tr -s ' ' '\n' | grep -v '^$')"
    out=""; i=0
    for b in $hex; do
        j=$(( i % ${#KEY} ))
        kb=$(printf '%d' "'$(printf '%s' "$KEY" | cut -c$((j+1)))")
        out="$out$(printf '\\x%02x' $(( b ^ kb )))"
        i=$(( i + 1 ))
    done
    printf '%b' "$out"
}

PASS="${PORTAL_PASS:-$(cfg_get password)}"
[ -z "$PASS" ] && PASS="$(decode_ob "$(cfg_get password_ob)")"

BASE="https://$HOST"
JAR="$(mktemp 2>/dev/null || echo /tmp/portal_cookie_$$)"

urlencode_b64() { printf '%s' "$1" | base64 | tr -d '\n'; }

urldecode() {
    printf '%b' "$(printf '%s' "$1" | sed 's/+/ /g; s/%\([0-9A-Fa-f][0-9A-Fa-f]\)/\\x\1/g')"
}

decode_payload() {
    t="$1"
    [ -z "$t" ] && { printf ''; return; }
    dec="$(printf '%s' "$t" | base64 -d 2>/dev/null)"
    urldecode "$dec"
}

# ---------- 业务 ----------
is_online() {
    code="$(curl -s -o /dev/null -w '%{http_code}' --max-time 6 -A "Microsoft NCSI" \
        "http://www.msftconnecttest.com/connecttest.txt" 2>/dev/null)"
    [ "$code" = "200" ]
}

open_entry() {
    curl -sk -A "$UA" -c "$JAR" -b "$JAR" -o /dev/null "$BASE/portal?nasip=$NASIP"
    curl -sk -A "$UA" -c "$JAR" -b "$JAR" -o /dev/null "$BASE/portal/2021/login.jsp?nasip=$NASIP"
}

post_auth() {
    action="$1"   # li | lo
    pwd_b64="$(urlencode_b64 "$PASS")"
    curl -sk -A "$UA" -b "$JAR" -c "$JAR" \
        -H "X-Requested-With: XMLHttpRequest" \
        --data-urlencode "freeOTP_secret_hidden=" \
        --data-urlencode "userName=$USER" \
        --data-urlencode "userPwd=$pwd_b64" \
        --data-urlencode "userurl=" \
        --data-urlencode "userip=" \
        --data-urlencode "basip=$NASIP" \
        --data-urlencode "language=English" \
        --data-urlencode "usermac=null" \
        --data-urlencode "wlannasid=" \
        --data-urlencode "wlanssid=" \
        --data-urlencode "entrance=null" \
        --data-urlencode "loginVerifyCode=" \
        --data-urlencode "userDynamicPwddd=" \
        --data-urlencode "customPageId=105" \
        --data-urlencode "send_dynamic_pwd_type=0" \
        --data-urlencode "pwdMode=0" \
        --data-urlencode "portalProxyIP=$IP" \
        --data-urlencode "portalProxyPort=50200" \
        --data-urlencode "dcPwdNeedEncrypt=1" \
        --data-urlencode "assignIpType=0" \
        --data-urlencode "appRootUrl=$BASE/portal/" \
        --data-urlencode "manualUrl=" \
        --data-urlencode "manualUrlEncryptKey=" \
        "$BASE/portal/pws?t=$action&ifEmailAuth=false"
}

report() {
    body="$1"; label="$2"
    json="$(decode_payload "$body")"
    en="$(printf '%s' "$json" | sed -n 's/.*"errorNumber"[[:space:]]*:[[:space:]]*"\?\([0-9]*\)"\?.*/\1/p')"
    if [ "$en" = "1" ]; then
        echo "$label成功。"
        return 0
    fi
    echo "$label失败：${json:-（空响应）}"
    return 1
}

do_login() {
    if [ -z "$USER" ] || [ -z "$PASS" ]; then
        echo "未配置账号或密码（设置 PORTAL_USER/PORTAL_PASS 或在 config.json 中保存）。"
        return 1
    fi
    echo "正在连接门户 $BASE ..."
    open_entry
    echo "正在提交认证请求 ..."
    body="$(post_auth li)"
    report "$body" "认证"
}

do_logout() {
    echo "正在提交注销请求 ..."
    open_entry
    body="$(post_auth lo)"
    report "$body" "注销"
}

case "${1:-login}" in
    status)  is_online && echo "在线" || echo "未上线（需要认证）" ;;
    login)   do_login ;;
    logout)  do_logout ;;
    *)       echo "用法: $0 [status|login|logout]" ;;
esac

rm -f "$JAR" 2>/dev/null
