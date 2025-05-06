#!/usr/bin/env python3
import argparse
import multiprocessing
import numpy as np
import re
import shlex
import subprocess
import time
from pathlib import Path

from mininet.topo import Topo
from mininet.node import CPULimitedHost, OVSController
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.util import dumpNodeConnections

IPERF_PORT = 9004
WEBSERVER_PORT = 8001

class NetworkTopology(Topo):
    def build(self, queue_size):
        # add hosts and router
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        router = self.addSwitch('s1')

        # PART 2 - commented out for part 3
        # self.addLink(h1, router, cls=TCLink, bw=1000, delay='5ms')
        # self.addLink(router, h2, cls=TCLink, bw=1.5, delay='5ms', max_queue_size=queue_size)

        # PART 3 - commented out for part 2
        self.addLink(h1, router, cls=TCLink, bw=1000, delay='5ms', loss=5)
        self.addLink(router, h2, cls=TCLink, bw=1.5, delay='5ms', max_queue_size=queue_size, loss=5)

def start_ping(server_host, client_host, data_dir):
    with open(data_dir / 'ping.txt', 'bw') as f:
        ping = server_host.popen(
            f"ping -i .1 {client_host.IP()} | while read line; do echo \"$(date +%s.%N) $line\"; done",
            stdout=f.fileno(),
            shell=True
        )
    return ping


def start_iperf(server_host, client_host):
    iperf_server_proc = client_host.popen(
        f'iperf --server --port {IPERF_PORT} --window 16M'
    )
    time.sleep(.1)
    iperf_client_proc = server_host.popen(
        f'iperf --client {client_host.IP()} --time 10000 --port {IPERF_PORT}'
    )

    return iperf_server_proc, iperf_client_proc


def stop_iperf(iperf, data_dir):
    for x in iperf:
        x.terminate()


def request_from_webserver(experiment_time, server_host, client_host, gap, data_dir):
    start_time = time.time()
    download_times = list()

    while True:
        time.sleep(gap)

        if time.time() - start_time > experiment_time:
            break

        curl = client_host.popen([
            "curl",
            "--output",
            "/dev/null",
            "--silent",
            "--write-out",
            "%{time_total}",
            f"{server_host.IP()}:{WEBSERVER_PORT}",
        ])

        try:
            download_time, _ = curl.communicate(timeout=20)
            download_times.append((float(download_time), time.time()))
        except subprocess.TimeoutExpired:
            curl.kill()
            download_times.append((20.0, time.time()))

    with open(data_dir / 'curl.txt', 'bw') as f:
        for d, e in download_times:
            f.write(f"{d},{e}\n".encode('utf-8'))


def start_tcp_probe():
    with open('/sys/kernel/debug/tracing/tracing_on', 'w') as f:
        f.write('1')
    with open('/sys/kernel/debug/tracing/events/tcp/tcp_probe/enable', 'w') as f:
        f.write('1')
    with open('/sys/kernel/debug/tracing/events/tcp/tcp_probe/filter', 'w') as f:
        f.write(f"dport == {IPERF_PORT}")
    with open('/sys/kernel/debug/tracing/trace', 'w') as f:
        f.write('')

    return open('/sys/kernel/debug/tracing/trace_pipe', 'r')


def tcp_probe_cwnd(trace_pipe, data_dir):
    with open(data_dir / 'tcp_probe.txt', 'w') as tpf:
        while True:
            line = trace_pipe.readline()

            if trace_pipe.closed:
                break

            if line:
                tpf.write(f"{time.time()},{line}")
                tpf.flush()


def measure_qlen(device, data_dir):
    pat_queued = re.compile(r'backlog\s[^\s]+\s([\d]+)p')

    with open(data_dir / 'qlen.txt', 'w') as qlf:
        while True:
            tc = subprocess.run(
                ["tc", "-s", "qdisc", "show", "dev", device],
                capture_output=True
            )

            matches = pat_queued.findall(tc.stdout.decode('utf-8'))
            if matches and len(matches) > 1:
                qlf.write(f"{time.time()},{matches[1]}\n")
            qlf.flush()
            time.sleep(0.1)

def get_mean_stdev_rtt(filename):
    '''
    measure average and standard deviation of RTT (ping times written to ping.txt)
    '''

    # read ping times from the file
    rtt_values = []
    rtt_pattern = re.compile(r'icmp_seq=\d+.*?time=([\d\.]+) ms')
    with open(filename, 'r') as file:
        for line in file:
            match = rtt_pattern.search(line)
            if match:
                # Extract RTT value from the matched group
                rtt_values.append(float(match.group(1)))

    # Calculate the average and standard deviation of RTT
    if rtt_values:
        avg_rtt = np.mean(rtt_values)
        stdev_rtt = np.std(rtt_values)
        print(f"Ping times ({len(rtt_values)}): {avg_rtt:0.3f}ms +- {stdev_rtt:0.3f}ms")
    else:
        print('no rtt went thru')

def get_mean_stdev_curl(filename):
    '''
    measure average and standard deviation of curl requests (download times written to curl.txt)
    '''

    curl_times = []

    with open(filename, 'r') as file:
        for line in file:
            # Split each line by comma and extract the first value
            parts = line.split(',')
            if len(parts) > 1:
                try:
                    curl_times.append(float(parts[0]))  # The first value is the time in seconds
                except ValueError:
                    continue  # If there's a malformed line, skip it

    # Calculate the average and standard deviation of curl request times
    if curl_times:
        avg_curl = np.mean(curl_times)
        stdev_curl = np.std(curl_times)
        print(f"Download times ({len(curl_times)}): {avg_curl:0.3f}s +- {stdev_curl:0.3f}s")
    else:
        print('no curls worked')


def buffer_bloat_experiment(args):
    # Ensure Mininet is in a fresh state
    subprocess.run(shlex.split("mn --clean"), capture_output=True)

    # Change TCP congestion control algorithm (Linux uses CUBIC TCP by default)
    subprocess.run(shlex.split(f"sudo sysctl -w net.ipv4.tcp_congestion_control={args.tcp_algo}"))

    # Standard mininet startup
    topo = NetworkTopology(args.queue_size)
    net = Mininet(topo, waitConnected=True)

    server_host = net.get('h1')
    client_host = net.get('h2')

    # Must measure the number of packets in the outgoing queue from s0 to h2,
    # the link that was added last to our topology
    queued_intf = net.get('s1').intfNames()[-1]

    net.start()

    # Print network topology
    dumpNodeConnections(net.hosts)

    # Ensure hosts can reach each other
    net.pingAll()

    # Record TCP cwnd size
    trace_pipe = start_tcp_probe()
    tcp_probe = multiprocessing.Process(
        target=tcp_probe_cwnd,
        args=(trace_pipe, args.data_dir),
    )
    tcp_probe.start()

    # Record length of router's outgoing queue to client
    qlen_monitor = multiprocessing.Process(
        target=measure_qlen,
        args=(queued_intf, args.data_dir),
    )
    qlen_monitor.start()

    iperf = start_iperf(server_host, client_host)
    ping = start_ping(server_host, client_host, args.data_dir)
    web = server_host.popen(["python3", "webserver.py"])

    request_from_webserver(
        args.runtime,
        server_host,
        client_host,
        3,
        args.data_dir,
    )

    # Shutdown
    tcp_probe.terminate()
    trace_pipe.close()
    qlen_monitor.terminate()
    web.terminate()
    stop_iperf(iperf, args.data_dir)
    ping.terminate()

    net.stop()

    get_mean_stdev_rtt(f"{args.data_dir}/ping.txt")
    get_mean_stdev_curl(f"{args.data_dir}/curl.txt")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run bufferbloat experiment')

    parser.add_argument('queue_size', type=int, help='Length of buffer')
    parser.add_argument(
        'runtime',
        type=int,
        help='Length of experiment in seconds',
    )
    parser.add_argument(
        'tcp_algo',
        type=str,
        help='Tcp congestion control algorithm',
    )
    parser.add_argument(
        'data_dir',
        type=Path,
        help='Directory to output results',
    )

    args = parser.parse_args()
    args.data_dir.mkdir(exist_ok=True)

    buffer_bloat_experiment(args)
