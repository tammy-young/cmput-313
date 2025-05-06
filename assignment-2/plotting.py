#!/usr/bin/env python3
import argparse
import re
import numpy as np
from pathlib import Path
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches


def get_ping(data_dir):
    RE_PING = re.compile(r"(\d+\.\d+) \d+ bytes from .+: icmp_seq=\d+ ttl=\d+ time=(\d+(?:\.\d+)?) ms")
    ping = data_dir / "ping.txt"

    # time, rtt
    with open(ping, 'r') as f:
        ping = f.readlines()
        ping = map(lambda x: x.strip(), ping)
        ping = map(lambda x: RE_PING.match(x), ping)
        ping = filter(lambda x: x is not None, ping)
        ping = map(lambda x: [float(x[1]), float(x[2])], ping)
        ping = np.array(list(ping))

    return ping


def get_curl(data_dir):
    RE_CURL = re.compile(r"([\d\.]+),([\d\.]+)")
    curl = data_dir / 'curl.txt'

    # start_time, end_time
    with open(curl, 'r') as f:
        curl = f.readlines()
        curl = map(lambda x: RE_CURL.match(x), curl)
        curl = map(lambda x: [float(x[1]), float(x[2])], curl)
        curl = map(lambda x: [x[1] - x[0], x[1]], curl)
        curl = np.array(list(curl))

    return curl


def get_tcp_probe(data_dir):
    RE_UNX_TIME = re.compile(r"^(\d+\.\d+)")
    RE_TCP_CWND = re.compile(r"snd_cwnd=(\d+)")

    tcp_probe = args.data_dir / "tcp_probe.txt"
    # time, cwnd
    with open(tcp_probe, 'r') as f:
        tcp_probe = f.readlines()
        tcp_probe = map(lambda x: [RE_UNX_TIME.search(x), RE_TCP_CWND.search(x)], tcp_probe)
        tcp_probe = map(lambda x: [float(x[0][1]), int(x[1][1])], tcp_probe)
        tcp_probe = np.array(list(tcp_probe))

    return tcp_probe


def get_qlen(data_dir):
    qlen = args.data_dir / "qlen.txt"

    # time, queue_length
    with open(qlen, 'r') as f:
        qlen = f.readlines()
        qlen = map(lambda x: x.strip().split(','), qlen)
        qlen = map(lambda x: [float(x[0]), int(x[1])], qlen)
        qlen = np.array(list(qlen))

    return qlen


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("data_dir", type=Path)
    parser.add_argument("tcp_algo", type=str)
    parser.add_argument("cutoff", type=int)
    args = parser.parse_args()

    ping = get_ping(args.data_dir)
    curl = get_curl(args.data_dir)
    tcp_probe = get_tcp_probe(args.data_dir)
    qlen = get_qlen(args.data_dir)

    # Normalize time values
    start_time = np.min([ping[0,0], curl[0,0], tcp_probe[0,0], qlen[0,0]])
    curl -= start_time
    ping[:, 0] -= start_time
    tcp_probe[:, 0] -= start_time
    qlen[:, 0] -= start_time

    # Crop out irrelevant data
    cutoff = max(args.cutoff, curl[-1,-1])
    ping = ping[ping[:, 0] <= cutoff]
    tcp_probe = tcp_probe[tcp_probe[:, 0] <= cutoff]
    qlen = qlen[qlen[:, 0] <= cutoff]

    # Plotting!
    fig, ax1 = plt.subplots(figsize=(14, 6), dpi=300)

    # TCP CWND
    tcp_cwnd_line, = ax1.plot(tcp_probe[:, 0], tcp_probe[:, 1], label='TCP CWND', color='blue')
    ax1.set_xlabel('Time (s)')
    ax1.set_ylabel('TCP CWND (MSS)', color='blue')
    ax1.tick_params(axis='y', labelcolor='blue')

    # qlen
    ax2 = ax1.twinx()
    qlen_line = ax2.plot(qlen[:, 0], qlen[:, 1], label='Queue Length', color='red')
    ax2.set_ylabel('Queue Length', color='red')
    ax2.tick_params(axis='y', labelcolor='red')

    # RTT
    ax3 = ax1.twinx()
    ax3.spines['right'].set_position(('outward', 40))
    rtt_line, = ax3.plot(ping[:, 0], ping[:, 1], label='Ping RTT', color='green')
    ax3.set_ylabel('Ping RTT (ms)', color='green')
    ax3.tick_params(axis='y', labelcolor='green')

    # curl
    for start, end in curl:
        ax1.axvspan(start, end, color='yellow', alpha=0.3)

    curl_highlight = mpatches.Patch(color='yellow', alpha=0.3, label='Curl Highlights')

    lines_labels = [
        ax1.get_legend_handles_labels(),
        ax2.get_legend_handles_labels(),
        ax3.get_legend_handles_labels(),
    ]
    lines, labels = [sum(lol, []) for lol in zip(*lines_labels)]
    lines.append(curl_highlight)
    labels.append('Curl Request')
    ax1.legend(lines, labels, loc='upper left', bbox_to_anchor=(-0.16, 1.15))

    plt.title(f'BufferBloat during intermittent network requests, using {args.tcp_algo}')
    plt.grid(True)

    output_file = args.data_dir / f"{args.data_dir.name}.png"
    plt.savefig(output_file, format='png')
