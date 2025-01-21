# -*- coding: utf8 -*-
import os
import subprocess
from datetime import datetime
import numpy as np

import onmi
import xmeasures
from comm_utils import read_ture_cluster, filter_nodes, DATASETS

Game_se = "../code/build/CoDeSEG"


def start(in_path,out_path,ground_truth,name,overlap,weighted,directed,verbose=True, tau = "0.3", parallel = "1"):
    args = [
        Game_se,
        "-i", in_path,
        "-o", out_path,
        "-n", "10",
        "-t", ground_truth,
        "-e", tau,
        "-p", parallel,
    ]
    if (overlap):
        args.append("-x")
    if (weighted):
        args.append("-w")
    if (directed):
        args.append("-d")
    if verbose:
        args.append("-v")

    result = subprocess.run(
        args,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output = result.stdout
    print(output)

    ture_comm, keep_nodes, _ = read_ture_cluster(ground_truth, name)

    division = []
    with open(out_path) as f:
        for line in f:
            nodes = line.strip().split()
            nodes = [int(n) for n in nodes]
            division.append(nodes)

    pre_comm = filter_nodes(division, keep_nodes)

    if name == "tweet2018" or name == "tweet2012":
        xnmi = xmeasures.nmi(pre_comm, ture_comm, name)
        print('nmi: ', xnmi, flush=True)
    else:
        nmi = onmi.overlapping_normalized_mutual_information(pre_comm, ture_comm, name)
        print('onmi: ', nmi, flush=True)

    f1 = xmeasures.f1(pre_comm, ture_comm, name)
    print('f1: ', f1)

    print("\n")




if __name__ == '__main__':
    for name in [ "amazon" ,"youtube", "dblp","LiveJournal","orkut", "friendster","tweet2012","tweet2018","wiki"]:

            print("-" * 40)
            print(f"dataset:{name}")
            out_path = f"../result/{name}.txt"
            start(DATASETS[name][0],out_path,DATASETS[name][1], name,DATASETS[name][2], DATASETS[name][3], DATASETS[name][4])



