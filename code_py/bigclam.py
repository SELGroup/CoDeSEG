# -*- coding: utf8 -*-
import os
import subprocess
from datetime import datetime

import numpy as np

import onmi
import xmeasures
from comm_utils import read_ture_cluster, filter_nodes, DATASETS

bigclam = "../Snap-6.0/examples/bigclam/bigclam"


def start(in_path,out_path,ground_truth,name, cmty_num= 25000):
    ture_comm, keep_nodes, _ = read_ture_cluster(ground_truth, name)

    args = [
        bigclam,
        f"-i:{in_path}",
        f"-o:{out_path}",
        f"-d:{name}",
        f"-c:{cmty_num}",
        f"-nt:1",
    ]

    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    start = datetime.now().strftime("%m/%d %H:%M:%S")
    result = subprocess.run(
        args,
        check=True,  
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,  
        text=True  
    )
    output = result.stdout
    print(output)

    end = datetime.now().strftime("%m/%d %H:%M:%S")
    start_time = datetime.strptime(start, "%m/%d %H:%M:%S")
    end_time = datetime.strptime(end, "%m/%d %H:%M:%S")
    time_diff = end_time - start_time
    seconds = time_diff.total_seconds()

    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    print(f"time consuming:{seconds}")

    division = []
    with open(out_path+name+"cmtyvv.txt") as f:
        for line in f:
            nodes = line.strip().split()
            nodes = [int(n) for n in nodes]

            division.append(nodes)
    nxx = set()
    for x in division:
        for y in x:
            nxx.add(y)
    pre_comm = filter_nodes(division, keep_nodes)

    print('n_clusters gt: ', len(ture_comm))
    print('n_clusters pred: ', len(pre_comm))

    nmi = onmi.overlapping_normalized_mutual_information(pre_comm, ture_comm, name)
    print('onmi: ', nmi, flush=True)

    f1 = xmeasures.f1(pre_comm, ture_comm, name)
    print('f1: ', f1)





if __name__ == '__main__':
   for name in [ "amazon" ,"youtube", "dblp","LiveJournal","orkut", "friendster","wiki"]:
            print("-" * 40)
            print(f"dataset:{name}")
            out_path = f"../Code_com/eva_txt/bigclam/"
            start(DATASETS[name][0],out_path,DATASETS[name][1], name)

