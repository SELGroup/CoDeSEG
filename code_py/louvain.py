from datetime import datetime
import numpy as np
import igraph as ig
import xmeasures


def LOU(g_path,t_path,name):
    edges = np.load(g_path, allow_pickle=True)
    true_labels = np.load(t_path, allow_pickle=True)



    node_idx = {}
    nodes = []
    edges_m = []
    weight = []
    for a, b, w in edges:
        a = int(a)
        b = int(b)

        if a not in node_idx:
            node_idx[a] = len(nodes)
            nodes.append(a)

        if b not in node_idx:
            node_idx[b] = len(nodes)
            nodes.append(b)
        edges_m.append((node_idx[a], node_idx[b]))
        weight.append(w)

    G = ig.Graph()
    G.add_vertices(len(nodes))
    G.add_edges(edges_m)
    G.es['weight'] = weight

    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    start = datetime.now().strftime("%m/%d %H:%M:%S")

    pre_l = G.community_multilevel().membership

    end = datetime.now().strftime("%m/%d %H:%M:%S")
    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    start_time = datetime.strptime(start, "%m/%d %H:%M:%S")
    end_time = datetime.strptime(end, "%m/%d %H:%M:%S")
    time_diff = end_time - start_time
    seconds = time_diff.total_seconds()

    print(f"time consuming:{seconds}")

    pre_comm_d = {}
    pre_label_map = [-1] * len(nodes)

    for n, com in enumerate(pre_l):
        if com not in pre_comm_d:
            pre_comm_d[com] = []
        pre_comm_d[com].append(nodes[n])
        pre_label_map[nodes[n]] = com

    pre_comm = list(pre_comm_d.values())

    ture_comm_d = {}

    for n, com in enumerate(true_labels):
        if com not in ture_comm_d:
            ture_comm_d[com] = []
        ture_comm_d[com].append(n)
    ture_comm = list(ture_comm_d.values())

    print('n_clusters gt: ', len(ture_comm))
    print('n_clusters pred: ', len(pre_comm))

    f1 = xmeasures.f1(pre_comm, ture_comm, name)
    print('f1: ', f1)
    nmi = xmeasures.nmi(pre_comm, ture_comm, name)
    print('nmi: ', nmi)



    print("-" * 40)


DATASETS={
    "tweet2012": ["../graph_dataset/tweet2012/awedges.npy", "../graph_dataset/tweet2012/labels.npy"],
    "tweet2018": ["../graph_dataset/tweet2018/awedges.npy", "../graph_dataset/tweet2018/labels.npy"]
}
if __name__ == "__main__":
    for name in ["tweet2012", "tweet2018"]:
        print("-" * 40)
        print(f"dataset:{name}")
        LOU(DATASETS[name][0],DATASETS[name][1],name)