# -*- coding: utf8 -*-
import glob
import os
import re
import shutil
import subprocess
from typing import Tuple, List

from sklearn import metrics
from BenchmarkRun import BenchmarkRun
lazy_fox_binary = "./lazy-fox/LazyFox"

dataset_directory = "./tmp/lazy_fox_in"
output_directory = "./tmp/lazy_fox_out"


class LazyFox:
    def __init__(self, queue_size=1, thread_count=1,
                 wcc_threshold=0.01, disable_dumping=False):
        self.queue_size = queue_size
        self.thread_count = thread_count
        self.wcc_threshold = wcc_threshold
        # self.disable_dumping = disable_dumping

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
            "--wcc-threshold", str(self.wcc_threshold)
        ]

        # if self.disable_dumping:
        #     cmd_lazy_fox.append("--disable-dumping")



        subprocess.run(cmd_lazy_fox, capture_output=True)
        # out = subprocess.run(["ls", "-l", "/dev/null"], capture_output=True)
        # print(out.stdout)
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

        # self.labels_ = [-1 for _ in range(node_num)]
        # label = -1
        # for nodes in clusters:
        #     label += 1
        #     for v in nodes:
        #         if self.labels_[v] < 0 and v < node_num:
        #             self.labels_[v] = label
        #
        # for i, _ in enumerate(self.labels_):
        #     if self.labels_[i] < 0:
        #         label += 1
        #         self.labels_[i] = label

        return clusters


if __name__ == '__main__':
    graph_node_num = 6
    graph_edges = [
        (0, 1), (0, 4),
        (1, 0), (1, 2), (1, 4),
        (2, 1), (2, 3), (2, 5),
        (3, 2), (3, 4), (3, 5),
        (4, 0), (4, 1), (4, 3),
        (5, 2), (5, 3)
    ]

    tags = [0, 0, 1, 1, 0, 1]

    fox = LazyFox()
    fox.fit(graph_node_num, graph_edges)
    preds = fox.labels_

    score_funcs = [
        ("NMI", metrics.normalized_mutual_info_score),
        ("AMI", metrics.adjusted_mutual_info_score),
        ("ARI", metrics.adjusted_rand_score),
    ]

    scores = {m: fun(tags, preds) for m, fun in score_funcs}

    print(scores)

    # for c in find_cliques(g):
    #     print({i + 1 for i in c})

    # line = "".join(['*'] * 40)
    # print(f"\n{line}")
    # for c in clique_percolate(g, 3):
    #     print(c)

    exit()
