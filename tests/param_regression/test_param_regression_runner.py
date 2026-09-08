import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class ParamRegressionRunnerTests(unittest.TestCase):
    def test_default_smoke_generates_parameter_artifacts_without_slicer_binaries(self):
        repo_root = Path(__file__).resolve().parents[2]
        with tempfile.TemporaryDirectory() as tmp:
            out_dir = Path(tmp) / "param-regression"
            missing_cli = Path(tmp) / "missing-owzx-cli.exe"

            proc = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    "scripts/param_regression.ps1",
                    "-Mode",
                    "smoke",
                    "-OutDir",
                    str(out_dir),
                    "-OwzxCli",
                    str(missing_cli),
                ],
                cwd=repo_root,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=60,
            )

            self.assertEqual(proc.returncode, 0, proc.stderr)
            summary = json.loads((out_dir / "summary.json").read_text(encoding="utf-8-sig"))
            self.assertEqual(summary["mode"], "smoke")
            self.assertFalse(summary["run_slicing"])
            self.assertEqual(len(summary["results"]), 2)
            self.assertEqual(
                {result["status"] for result in summary["results"]},
                {"parameter_artifacts_generated"},
            )
            for result in summary["results"]:
                self.assertTrue(Path(result["owzx_settings"]).is_file())
                self.assertTrue(Path(result["upstream_settings"]).is_file())
                self.assertIsNone(result["owzx"])
                self.assertIsNone(result["upstream"])


if __name__ == "__main__":
    unittest.main()
