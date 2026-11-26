#! /usr/bin/env python3
#
# Copyright (C) 2018-2026 CERN
# License Apache2 - see LICENCE file
 
"""Test harness for the power monitor (powermon)
 
A small local HTTP server (or a plain text file) provides Prometheus-style
text with known values, so the test does not depend on the machine having
any power metrics hardware or exporter installed."""
 
import http.server
import json
import os
import subprocess
import tempfile
import threading
import unittest
 
from prmon_test_utils import dump_on_failure
 
# Nothing listens on port 1: connections are refused immediately. Used so that
# the default exporter ports (9100, 9290) are never tried by the tests, as a
# real exporter running on the test machine would change the results
DEAD_URL = "http://127.0.0.1:1/metrics"
 
POWER_KEYS = ("dcmi", "ipmi", "hwmon", "rapl_1", "rapl_2")
 
 
def prometheus_text(dcmi=None, ipmi=None, hwmon=None, rapl=()):
    """Build Prometheus exposition text. Values that are None are left out.
    Includes # HELP / # TYPE lines, which must not be taken as samples"""
    lines = []
    if dcmi is not None:
        lines += [
            "# HELP ipmi_dcmi_power_consumption_watts Current power consumption.",
            "# TYPE ipmi_dcmi_power_consumption_watts gauge",
            f"ipmi_dcmi_power_consumption_watts {dcmi}",
        ]
    if ipmi is not None:
        lines += [
            "# HELP ipmi_power_watts Power reading of the sensor.",
            "# TYPE ipmi_power_watts gauge",
            f'ipmi_power_watts{{id="1",name="Pwr Consumption"}} {ipmi}',
        ]
    if hwmon is not None:
        lines += [
            "# HELP node_hwmon_power_watt Hardware monitor for power.",
            "# TYPE node_hwmon_power_watt gauge",
            f'node_hwmon_power_watt{{chip="chip0",sensor="power1"}} {hwmon}',
        ]
    if rapl:
        lines += [
            "# HELP node_rapl_package_joules_total Current RAPL package value.",
            "# TYPE node_rapl_package_joules_total counter",
        ]
        for index, value in enumerate(rapl):
            lines.append(
                f'node_rapl_package_joules_total{{index="{index}",'
                f'path="/sys/class/powercap/intel-rapl:{index}"}} {value}'
            )
    return "\n".join(lines) + "\n"
 
 
class MetricsHandler(http.server.BaseHTTPRequestHandler):
    """Serves a fixed body for any GET request"""
 
    body = b""
 
    def do_GET(self):  # noqa: N802
        self.send_response(200)
        self.send_header("Content-Type", "text/plain; version=0.0.4")
        self.send_header("Content-Length", str(len(self.body)))
        self.end_headers()
        self.wfile.write(self.body)
 
    def log_message(self, *args):  # keep the test output clean
        pass
 
 
def start_exporter(text):
    """Start a local exporter on a free port. Returns (server, url)"""
    handler = type("Handler", (MetricsHandler,), {"body": text.encode()})
    server = http.server.HTTPServer(("127.0.0.1", 0), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server, f"http://127.0.0.1:{server.server_address[1]}/metrics"
 
 
def stop_exporter(server):
    server.shutdown()
    server.server_close()
 
 
class PowerMonitor(unittest.TestCase):
    """Tests for the power monitor"""
 
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmpdir.cleanup)
        self.servers = []
 
    def tearDown(self):
        for server in self.servers:
            stop_exporter(server)
 
    def exporter(self, text):
        server, url = start_exporter(text)
        self.servers.append(server)
        return url
 
    def run_prmon(self, power_source, units=False):
        """Run prmon on a short sleep with the given PROMETHEUS_LOCAL_URL
        and return the JSON summary"""
        env = dict(os.environ)
        env["PROMETHEUS_LOCAL_URL"] = power_source
        # Isolate from any default file in the user's home directory
        env["HOME"] = self.tmpdir.name
 
        prmon_cmd = ["../prmon", "--interval", "1"]
        if units:
            prmon_cmd.append("--units")
        prmon_cmd.extend(["--", "sleep", "3"])
 
        prmon_rc = subprocess.Popen(prmon_cmd, shell=False, env=env).wait()
        self.assertEqual(prmon_rc, 0, "Non-zero return code from prmon")
 
        with open("prmon.json") as infile:
            return json.load(infile)
 
    def check_values(self, prmon_json, **expected):
        for key, value in expected.items():
            self.assertIn(key, prmon_json["Max"], f"Missing '{key}' in the summary")
            self.assertEqual(
                prmon_json["Max"][key],
                value,
                f"Wrong value for '{key}' "
                f"(expected {value}, got {prmon_json['Max'][key]})",
            )
 
    @dump_on_failure
    def test_single_exporter(self):
        """All the metrics from one exporter URL"""
        url = self.exporter(
            prometheus_text(
                dcmi=300, ipmi=280, hwmon=50, rapl=(1000000, 2000000)
            )
        )
        prmon_json = self.run_prmon(url)
        self.check_values(
            prmon_json,
            dcmi=300,
            ipmi=280,
            hwmon=50,
            rapl_1=1000000,
            rapl_2=2000000,
        )
 
    @dump_on_failure
    def test_single_rapl_package(self):
        """A machine with one RAPL package: its value goes to rapl_1.
        Guards against # HELP / # TYPE lines being counted as samples"""
        url = self.exporter(prometheus_text(dcmi=300, rapl=(1000000,)))
        prmon_json = self.run_prmon(url)
        self.check_values(prmon_json, dcmi=300, rapl_1=1000000, rapl_2=0)
 
    @dump_on_failure
    def test_metrics_split_over_exporters(self):
        """Metrics spread over two exporters (as node_exporter and
        ipmi_exporter) are all picked up"""
        node_url = self.exporter(prometheus_text(hwmon=50, rapl=(1000000, 2000000)))
        ipmi_url = self.exporter(prometheus_text(dcmi=300, ipmi=280))
        prmon_json = self.run_prmon(f"{node_url},{ipmi_url}")
        self.check_values(
            prmon_json,
            dcmi=300,
            ipmi=280,
            hwmon=50,
            rapl_1=1000000,
            rapl_2=2000000,
        )
 
    @dump_on_failure
    def test_read_from_file(self):
        """Metrics read from a text file when no exporter answers"""
        path = os.path.join(self.tmpdir.name, "power.prom")
        with open(path, "w") as outfile:
            outfile.write(
                prometheus_text(
                    dcmi=600, ipmi=560, hwmon=100, rapl=(3000000, 4000000)
                )
            )
        prmon_json = self.run_prmon(f"{DEAD_URL},{path}")
        self.check_values(
            prmon_json,
            dcmi=600,
            ipmi=560,
            hwmon=100,
            rapl_1=3000000,
            rapl_2=4000000,
        )
 
    @dump_on_failure
    def test_no_source(self):
        """No exporter and no file: prmon must still run fine and simply
        not report any power values"""
        prmon_json = self.run_prmon(DEAD_URL)
        for key in POWER_KEYS:
            self.assertNotIn(key, prmon_json["Max"], f"Unexpected '{key}' in summary")
 
    @dump_on_failure
    def test_units(self):
        """Every reported power value has a unit and vice versa"""
        url = self.exporter(
            prometheus_text(
                dcmi=300, ipmi=280, hwmon=50, rapl=(1000000, 2000000)
            )
        )
        prmon_json = self.run_prmon(url, units=True)
        for group in ("Max", "Avg"):
            value_params = set(prmon_json[group].keys())
            unit_params = set(prmon_json["Units"][group].keys())
            missing = value_params - unit_params
            self.assertEqual(
                len(missing),
                0,
                f"Wrong number of unit values for '{group}' - "
                f"missing parameters are {missing}",
            )
            extras = unit_params - value_params
            self.assertEqual(
                len(extras),
                0,
                f"Wrong number of unit values for '{group}' - "
                f"extra parameters are {extras}",
            )
 
 
if __name__ == "__main__":
    unittest.main()
 