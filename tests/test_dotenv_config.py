import tempfile
import unittest
from pathlib import Path

from scripts.dotenv_config import cpp_string, make_header, read_dotenv


class DotenvConfigTests(unittest.TestCase):
    def parse(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / ".env"
            path.write_text(text, encoding="utf-8-sig")
            return read_dotenv(path)

    def test_literal_credentials(self):
        values = self.parse('# comment\nWIFI_SSID=" 日本語 SSID "\n'
                            "export WIFI_PASSWORD='a#b=c$HOME\\path\"'\n")
        self.assertEqual(values["WIFI_SSID"], " 日本語 SSID ")
        self.assertEqual(values["WIFI_PASSWORD"], 'a#b=c$HOME\\path"')

    def test_missing_file_and_defaults(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(read_dotenv(Path(directory) / ".env"), {})
        header = make_header({})
        self.assertIn('#define WIFI_SSID ""', header)
        self.assertIn('#define NTP_SERVER "pool.ntp.org"', header)

    def test_cpp_escaping(self):
        self.assertEqual(cpp_string('a"\\\n'), '"a\\"\\\\\\012"')
        self.assertEqual(cpp_string("あ"), '"\\343\\201\\202"')

    def test_errors_do_not_expose_values(self):
        for text in ('WIFI_PASSWORD="secret', 'OTHER=secret',
                     'WIFI_SSID=one\nWIFI_SSID=secret'):
            with self.assertRaises(ValueError) as caught:
                self.parse(text)
            self.assertNotIn("secret", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
