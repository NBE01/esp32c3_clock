from pathlib import Path
import sys

Import("env")
project_dir = Path(env.subst("$PROJECT_DIR"))
sys.path.insert(0, str(project_dir / "scripts"))
from dotenv_config import make_header, read_dotenv

try:
    values = read_dotenv(project_dir / ".env")
except ValueError as error:
    print(error)
    env.Exit(1)

# Keep credentials out of compiler command lines and tracked source files.
generated_dir = project_dir / ".pio" / "generated"
generated_dir.mkdir(parents=True, exist_ok=True)
header = generated_dir / "config_env.h"
content = make_header(values)
if not header.exists() or header.read_text(encoding="utf-8") != content:
    header.write_text(content, encoding="utf-8")
env.Prepend(CPPPATH=[str(generated_dir)])
