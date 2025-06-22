import subprocess
import re
import platform
from pathlib import Path

BUILD_DIR = Path(__file__).resolve().parent.parent / 'build'


def host_spec() -> str:
    """Return a short description of the host CPU."""
    try:
        out = subprocess.check_output(['lscpu'], text=True)
        model = re.search(r'Model name:\s+(.+)', out)
        arch = re.search(r'Architecture:\s+(.+)', out)
        cpus = re.search(r'CPU\(s\):\s+(\d+)', out)
        if model and arch and cpus:
            return f"{model.group(1)} ({arch.group(1)}, {cpus.group(1)} CPUs)"
    except Exception:
        pass
    u = platform.uname()
    return f"{u.system} {u.machine} {u.processor}".strip()

def run_bench(dec, prep, disp, iters=1000):
    cmd = [
        './bench_tests',
        '../tests/fixtures',
        str(iters),
        '-stage1',
        str(dec),
        '-stage2',
        str(prep),
        '-stage3',
        str(disp),
    ]
    res = subprocess.run(cmd, cwd=BUILD_DIR, text=True, capture_output=True)
    return res.stdout

def measure(dec, prep, disp):
    out = run_bench(dec, prep, disp)
    m = re.search(r'Overall: ([0-9.]+) cmds/s', out)
    return float(m.group(1)) if m else 0.0

best = (0.0, 1, 1, 1)
for d in (1, 2, 3):
    for p in (1, 2, 3):
        for t in (1, 2, 3):
            cps = measure(d, p, t)
            if cps > best[0]:
                best = (cps, d, p, t)

output = run_bench(best[1], best[2], best[3], 1000000)

pattern = re.compile(r'^(\w+): ([0-9.]+) cmds/s$', re.MULTILINE)
rows = [(n, c) for (n, c) in pattern.findall(output) if n != 'Overall']
overall = re.search(r'Overall: ([0-9.]+) cmds/s', output)

md_lines = [f"**Host**: {host_spec()}",
            f"**Best configuration**: decode={best[1]} prepare={best[2]} dispatch={best[3]}",
            "",
            "| Shader | Cmds/s |",
            "|-------|-------:|"]
for name, cps in rows:
    md_lines.append(f"| {name} | {cps} |")
if overall:
    md_lines.append(f"| **Overall** | {overall.group(1)} |")

(Path(__file__).resolve().parent.parent / 'THRUPUT.md').write_text("\n".join(md_lines) + "\n")
print(f"Best combo: decode={best[1]} prepare={best[2]} dispatch={best[3]}")
print('\n'.join(md_lines))
