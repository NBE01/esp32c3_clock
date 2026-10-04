"""Read literal .env values and encode them safely as C++ string literals."""


def read_dotenv(path):
    if not path.exists():
        return {}
    values = {}
    for number, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[7:].lstrip()
        key, separator, value = line.partition("=")
        key, value = key.strip(), value.strip()
        if not separator or key not in {"WIFI_SSID", "WIFI_PASSWORD", "NTP_SERVER"}:
            raise ValueError(f".env line {number}: expected WIFI_SSID, WIFI_PASSWORD or NTP_SERVER assignment")
        if value.startswith(("'", '"')):
            if len(value) < 2 or value[-1] != value[0]:
                raise ValueError(f".env line {number}: unmatched quotes")
            value = value[1:-1]
        if key in values:
            raise ValueError(f".env line {number}: duplicate {key}")
        if "\0" in value:
            raise ValueError(f".env line {number}: NUL is not allowed")
        values[key] = value
    return values


def cpp_string(value):
    # Octal escapes have fixed length, preserve UTF-8 and avoid shell expansion.
    result = []
    for byte in value.encode("utf-8"):
        if byte in (34, 92):
            result.append("\\" + chr(byte))
        elif 32 <= byte < 127:
            result.append(chr(byte))
        else:
            result.append(f"\\{byte:03o}")
    return '"' + "".join(result) + '"'


def make_header(values):
    return "#pragma once\n" + "".join(
        f"#define {key} {cpp_string(values.get(key, default))}\n"
        for key, default in (
            ("WIFI_SSID", ""), ("WIFI_PASSWORD", ""), ("NTP_SERVER", "pool.ntp.org")
        )
    )
