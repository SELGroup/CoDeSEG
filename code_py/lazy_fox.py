# -*- coding: utf8 -*-
import glob
import os
import re
import shutil
import subprocess
from typing import Tuple, List


lazy_fox_binary = "../fox/Lazyfox"

dataset_directory = "../cache/lazy_fox_in"
output_directory = "../cache/lazy_fox_out"


class LazyFox:
    def __init__(self, queue_size=1, thread_count=1,
                 wcc_threshold=0.1):
        self.queue_size = queue_size
        self.thread_count = thread_count
        self.wcc_threshold = wcc_threshold

        self.labels_ = None

    def fit(self, node_num, edges: List[Tuple[int, int]]):
        shutil.rmtree(dataset_directory, ignore_errors=True)
        shutil.rmtree(output_directory, ignore_errors=True)

        os.makedirs(dataset_directory, exist_ok=True)
        os.makedirs(output_directory, exist_ok=True)

        graph_path = os.path.join(dataset_directory, "graph.txt")
        with open(graph_path, 'w', encoding='utf8') as f:
            for v1, v2 in edges:
                f.write(f"{v1}\t{v2}\n")
            f.flush()

        cmd_lazy_fox = [
            lazy_fox_binary,
            "--input-graph", graph_path,
            "--output-dir", output_directory,
            "--queue-size", str(self.queue_size),
            "--thread-count", str(self.thread_count),
            "--wcc-threshold", str(self.wcc_threshold),
        ]


        out = subprocess.run(cmd_lazy_fox, capture_output=True,text=True)
        cluster_files = glob.glob(os.path.join(output_directory, "*/*/*clusters.txt"))
        cluster_files.sort(key=os.path.getmtime, reverse=True)
        cluster_result = cluster_files[-1]
        clusters = []
        with open(cluster_result, 'r', encoding='utf8') as f:
            for line in f:
                line = line.strip()
                nodes = re.split(r"\s+", line)
                if len(nodes) > 0:
                    nodes = [int(n) for n in nodes]
                    clusters.append(nodes)


        return clusters


