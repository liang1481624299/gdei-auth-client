# -*- coding: utf-8 -*-
"""
广二师 GDEI 校园网（H3C iMC Portal）自动认证核心模块。

抓包结论（2026-11-21 实测门户 /portal/2021/login.jsp）：
- 门户地址: https://nac.gdei.edu.cn  (备用解析 IP 210.38.64.39)
- NAS(AC) : 192.168.100.8
- 登录接口: POST /portal/pws?t=li&ifEmailAuth=false
- 注销接口: POST /portal/pws?t=lo
- 密码处理: base64(明文密码)，标准 Base64（与页面 imc_portal_function_base64 一致）
- 返回报文: body 为 Base64，解码后再 URL 解码，得到 JSON；errorNumber == "1" 表示成功。
- 表单字段见 DEFAULT_FIELDS。

本模块只依赖 Python 标准库，可被 campus_gui.py 调用，也可直接命令行使用：
    python campus_core.py status
    python campus_core.py login
    python campus_core.py logout
"""

import base64
import gzip
import http.client
import json
import os
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request
import zlib

# --------------------------------------------------------------------------- #
# 默认配置（来自实测门户隐藏域，一般无需修改）
# --------------------------------------------------------------------------- #
DEFAULT_HOST = "nac.gdei.edu.cn"
DEFAULT_IP = "210.38.64.39"          # 系统 DNS 解析失败时的备用直连 IP
DEFAULT_SCHEME = "https"
DEFAULT_PORT = 443
DEFAULT_NASIP = "192.168.100.8"

ENTRY_PATH = "/portal?nasip={nasip}"
LOGIN_JSP_PATH = "/portal/2021/login.jsp?nasip={nasip}"
LOGIN_PATH = "/portal/pws?t=li&ifEmailAuth=false"
LOGOUT_PATH = "/portal/pws?t=lo"

# 门户表单默认隐藏域（顺序与浏览器 serializeArray() 一致）
DEFAULT_FIELDS = [
    ("freeOTP_secret_hidden", ""),
    ("userName", ""),
    ("userPwd", ""),
    ("userurl", ""),
    ("userip", ""),
    ("basip", ""),
    ("language", "English"),
    ("usermac", "null"),
    ("wlannasid", ""),
    ("wlanssid", ""),
    ("entrance", "null"),
    ("loginVerifyCode", ""),
    ("userDynamicPwddd", ""),
    ("customPageId", "105"),
    ("send_dynamic_pwd_type", "0"),
    ("pwdMode", "0"),
    ("portalProxyIP", "210.38.64.39"),
    ("portalProxyPort", "50200"),
    ("dcPwdNeedEncrypt", "1"),
    ("assignIpType", "0"),
    ("appRootUrl", "https://nac.gdei.edu.cn/portal/"),
    ("manualUrl", ""),
    ("manualUrlEncryptKey", ""),
]

CONFIG_NAME = "config.json"
# 本地口令混淆用的固定密钥（可逆，仅避免明文落在配置文件里，不是强加密）
_OBF_KEY = b"gdei-portal-2026"


# --------------------------------------------------------------------------- #
# 配置读写
# --------------------------------------------------------------------------- #
def config_path():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), CONFIG_NAME)


def _xor(data: bytes) -> bytes:
    return bytes(b ^ _OBF_KEY[i % len(_OBF_KEY)] for i, b in enumerate(data))


def obfuscate(plain: str) -> str:
    return base64.b64encode(_xor(plain.encode("utf-8"))).decode("ascii")


def deobfuscate(ob: str) -> str:
    if not ob:
        return ""
    try:
        return _xor(base64.b64decode(ob)).decode("utf-8")
    except Exception:
        return ""


def load_config() -> dict:
    p = config_path()
    cfg = {
        "username": "",
        "password_ob": "",
        "host": DEFAULT_HOST,
        "ip": DEFAULT_IP,
        "scheme": DEFAULT_SCHEME,
        "port": DEFAULT_PORT,
        "nasip": DEFAULT_NASIP,
    }
    if os.path.exists(p):
        try:
            with open(p, "r", encoding="utf-8") as f:
                cfg.update(json.load(f))
        except Exception:
            pass
    return cfg


def save_config(cfg: dict):
    with open(config_path(), "w", encoding="utf-8") as f:
        json.dump(cfg, f, ensure_ascii=False, indent=2)


def get_password(cfg: dict) -> str:
    return deobfuscate(cfg.get("password_ob", ""))


# --------------------------------------------------------------------------- #
# 极简 HTTP 传输（支持 IP 直连 + Host 头、Cookie、跟随跳转、跳过证书校验）
# --------------------------------------------------------------------------- #
class PortalClient:
    def __init__(self, cfg: dict, timeout: int = 10):
        self.host = cfg.get("host") or DEFAULT_HOST
        self.ip = cfg.get("ip") or DEFAULT_IP
        self.scheme = cfg.get("scheme") or DEFAULT_SCHEME
        self.port = int(cfg.get("port") or (443 if self.scheme == "https" else 80))
        self.timeout = timeout
        self.cookies = {}
        self.ctx = ssl._create_unverified_context()

    def _conn(self):
        if self.scheme == "https":
            return http.client.HTTPSConnection(self.ip or self.host, self.port,
                                               timeout=self.timeout, context=self.ctx)
        return http.client.HTTPConnection(self.ip or self.host, self.port,
                                          timeout=self.timeout)

    def _cookie_header(self) -> str:
        return "; ".join(f"{k}={v}" for k, v in self.cookies.items())

    def _store_cookies(self, headers: http.client.HTTPMessage):
        for k, v in headers.items():
            if k.lower() == "set-cookie":
                first = v.split(";")[0]
                if "=" in first:
                    name, val = first.split("=", 1)
                    self.cookies[name.strip()] = val.strip()

    def request(self, method: str, path: str, body: str = "",
                extra_headers: dict = None, allow_redirect: bool = True):
        headers = {
            "Host": self.host,
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                          "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36",
            "Accept": "text/plain, */*; q=0.01" if body else "*/*",
            "Referer": f"{self.scheme}://{self.host}/portal/2021/login.jsp",
        }
        if self.cookies:
            headers["Cookie"] = self._cookie_header()
        if body:
            headers["Content-Type"] = "application/x-www-form-urlencoded; charset=UTF-8"
            headers["X-Requested-With"] = "XMLHttpRequest"
        if extra_headers:
            headers.update(extra_headers)

        conn = self._conn()
        try:
            conn.request(method, path, body=body, headers=headers)
            resp = conn.getresponse()
            data = resp.read()
            headers_dict = dict(resp.getheaders())
            self._store_cookies(headers_dict)
            status = resp.status
            location = resp.getheader("Location")
            if allow_redirect and status in (301, 302, 303, 307, 308) and location:
                next_path = self._resolve(location)
                return self.request(method, next_path, body="",
                                    extra_headers=extra_headers, allow_redirect=True)
            data = self._decompress(headers_dict, data)
            return status, headers_dict, data.decode("utf-8", "replace")
        finally:
            conn.close()

    @staticmethod
    def _decompress(headers: dict, data: bytes) -> bytes:
        enc = (headers.get("Content-Encoding") or headers.get("content-encoding") or "").lower()
        if not data:
            return data
        try:
            if enc == "gzip":
                return gzip.decompress(data)
            if enc == "deflate":
                return zlib.decompress(data)
        except Exception:
            return data
        return data

    def _resolve(self, location: str) -> str:
        if location.startswith("http://") or location.startswith("https://"):
            u = urllib.parse.urlparse(location)
            return u.path + (("?" + u.query) if u.query else "")
        return location

    def open_entry(self, nasip: str):
        """先访问门户入口与登录页，建立会话 Cookie（前端控制器在非浏览器下可能返回空，属正常）。"""
        status, _, body = self.request("GET", ENTRY_PATH.format(nasip=nasip))
        self.request("GET", LOGIN_JSP_PATH.format(nasip=nasip))
        return status, body


# --------------------------------------------------------------------------- #
# 报文解析
# --------------------------------------------------------------------------- #
def decode_payload(text: str):
    """门户返回：Base64 -> URL 解码 -> JSON。失败时返回原始字符串。"""
    if text is None:
        return None
    text = text.strip()
    if not text:
        return None
    try:
        raw = base64.b64decode(text)
        decoded = urllib.parse.unquote(raw.decode("utf-8", "replace"))
        try:
            return json.loads(decoded)
        except Exception:
            return decoded
    except Exception:
        return text


def _error_message(obj) -> str:
    if isinstance(obj, dict):
        e_d = obj.get("e_d")
        if e_d and obj.get(e_d):
            return str(obj[e_d])
        if obj.get("errorMessage"):
            return str(obj["errorMessage"])
        return json.dumps(obj, ensure_ascii=False)
    return str(obj)


# --------------------------------------------------------------------------- #
# 业务动作
# --------------------------------------------------------------------------- #
def build_form(cfg: dict, username: str, password: str, userip: str = "",
               userurl: str = "") -> str:
    fields = list(DEFAULT_FIELDS)
    form = dict(fields)
    form["userName"] = username
    form["userPwd"] = base64.b64encode(password.encode("utf-8")).decode("ascii")
    form["userip"] = userip
    form["userurl"] = userurl
    # basip 隐藏域登录页渲染即为空，服务端按来源 IP 鉴权。
    # basip renders empty on the login page; the server authenticates by source IP.
    # 按默认顺序输出
    ordered = [(k, form.get(k, "")) for k, _ in fields]
    return urllib.parse.urlencode(ordered)


def login(cfg: dict, username: str = "", password: str = "", log=print):
    username = username or cfg.get("username", "")
    password = password or get_password(cfg)
    if not username or not password:
        return False, "未配置账号或密码，请先在图形界面中保存。"

    client = PortalClient(cfg)
    log("正在连接门户 %s ..." % f"{client.scheme}://{client.host}")
    try:
        client.open_entry(cfg.get("nasip") or DEFAULT_NASIP)
    except Exception as e:
        log(f"访问门户入口失败：{e}")

    body = build_form(cfg, username, password)
    log("正在提交认证请求 ...")
    try:
        status, headers, text = client.request("POST", LOGIN_PATH, body=body)
    except Exception as e:
        return False, f"请求异常：{e}"

    obj = decode_payload(text)
    log(f"门户返回 HTTP {status}")
    if isinstance(obj, dict) and str(obj.get("errorNumber")) == "1":
        return True, "认证成功，已上线。"
    return False, "认证失败：" + _error_message(obj)


def logout(cfg: dict, log=print):
    client = PortalClient(cfg)
    try:
        client.open_entry(cfg.get("nasip") or DEFAULT_NASIP)
    except Exception as e:
        log(f"访问门户入口失败：{e}")

    # 下线 t=lo 实测表单 userName/userPwd 均为空：在线会话按来源 IP / 会话释放，不提交凭据。
    # The logout form carries empty userName/userPwd; the session is released by source IP.
    body = build_form(cfg, "", "")
    log("正在提交注销请求 ...")
    try:
        status, headers, text = client.request("POST", LOGOUT_PATH, body=body)
    except Exception as e:
        return False, f"请求异常：{e}"

    obj = decode_payload(text)
    if isinstance(obj, dict) and str(obj.get("errorNumber")) == "1":
        return True, "已下线。"
    return False, "注销结果：" + _error_message(obj)


def is_online(timeout: int = 6) -> bool:
    """通过 Windows/系统通用强制门户探测地址判断是否已联网。"""
    probe = "http://www.msftconnecttest.com/connecttest.txt"
    try:
        req = urllib.request.Request(probe, headers={"User-Agent": "Microsoft NCSI"})
        with urllib.request.urlopen(req, timeout=timeout) as r:
            content = r.read(256).decode("utf-8", "replace")
            return "Microsoft" in content
    except urllib.error.HTTPError as e:
        # 被门户 302 重定向视为未上线
        return False
    except Exception:
        return False


def main():
    cfg = load_config()
    cmd = sys.argv[1] if len(sys.argv) > 1 else "status"
    if cmd == "status":
        print("在线" if is_online() else "未上线（需要认证）")
    elif cmd == "login":
        ok, msg = login(cfg)
        print(msg)
        sys.exit(0 if ok else 1)
    elif cmd == "logout":
        ok, msg = logout(cfg)
        print(msg)
        sys.exit(0 if ok else 1)
    else:
        print("用法: python campus_core.py [status|login|logout]")


if __name__ == "__main__":
    main()
