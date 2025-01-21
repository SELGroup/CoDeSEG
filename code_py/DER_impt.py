from datetime import datetime
from cdlib import algorithms
import networkx as nx
import numpy as np
import xmeasures


def start(g_path,t_path,name):
   
    edges = np.load(g_path, allow_pickle=True)
    true_labels = np.load(t_path, allow_pickle=True)

    node_idx = {}
    nodes = []
    edges_m = []
    for a, b, w in edges:
        a = int(a)
        b = int(b)

        if a not in node_idx:
            node_idx[a] = len(nodes)
            nodes.append(a)

        if b not in node_idx:
            node_idx[b] = len(nodes)
            nodes.append(b)
        edges_m.append((node_idx[a], node_idx[b],{'weight': w}))

    G = nx.Graph()
    G.add_edges_from(edges_m)


    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    start = datetime.now().strftime("%m/%d %H:%M:%S")

    comms = algorithms.der(G, 3, .00001, 50).communities

    end = datetime.now().strftime("%m/%d %H:%M:%S")
    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    start_time = datetime.strptime(start, "%m/%d %H:%M:%S")
    end_time = datetime.strptime(end, "%m/%d %H:%M:%S")
    time_diff = end_time - start_time
    seconds = time_diff.total_seconds()

    print(f"time consuming:{seconds}")

    pre_comm =[]

    for com in comms:
        map_com = []
        for node in com:
            map_com.append(nodes[node])
        pre_comm.append(map_com)

    ture_comm_d = {}

    for n, com in enumerate(true_labels):
        if com not in ture_comm_d:
            ture_comm_d[com] = []
        ture_comm_d[com].append(n)
    ture_comm = list(ture_comm_d.values())

    print('n_clusters gt: ', len(ture_comm))
    print('n_clusters pred: ', len(pre_comm))

    nmi = xmeasures.nmi(pre_comm, ture_comm, name)
    print('nmi: ', nmi)
    f1 = xmeasures.f1(pre_comm, ture_comm, name)
    print('f1: ', f1)
    



    print("-" * 40)


DATASETS={
    "tweet2012": ["../graph_dataset/tweet2012/awedges.npy", "../graph_dataset/tweet2012/labels.npy"],
    "tweet2018": ["../graph_dataset/tweet2018/awedges.npy", "../graph_dataset/tweet2018/labels.npy"]
}
if __name__ == "__main__":
    for name in ["tweet2012", "tweet2018"]:
        print("-" * 40)
        print(f"dataset:{name}")
        start(DATASETS[name][0],DATASETS[name][1],name)