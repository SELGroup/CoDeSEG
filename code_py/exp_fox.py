from datetime import datetime
from lazy_fox import LazyFox
import onmi
import xmeasures
from comm_utils import DATASETS

def read_graph(edge_file, cmty_file):
    edges = set()
    with open(edge_file) as f:
        for line in f:
            if line.startswith('#'): continue
            if line==" ": continue
            e = line.rstrip('\n').split('\t')

            edges.add((int(e[0]), int(e[1])) if e[0] < e[1] else (int(e[1]), int(e[0])))

    cmty = []
    with open(cmty_file) as f:
        for line in f:
            if line.startswith('#'): continue

            nodes = line.rstrip('\n').split('\t')
            # nodes = line.rstrip('\n').split(' ')
            nodes = [int(n) for n in nodes]
            cmty.append(nodes)

    return list(edges), cmty



def run_experiment(data_name):
    edge_file = DATASETS[data_name][0]
    cmty_file = DATASETS[data_name][1]

    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    start = datetime.now().strftime("%m/%d %H:%M:%S")

    edges, gt_cmty = read_graph(edge_file, cmty_file)

    gt_cmty = [c for c in gt_cmty if len(c) > 2]
    gt_nodes = set(int(n) for c in gt_cmty for n in c)
    all_nodes = set(n for e in edges for n in e)
    fox = LazyFox()
    div = fox.fit(node_num=len(all_nodes), edges=edges)


    pred_div = []
    nodes =[]
    for v in div:
        nodes += v
        v = set(v).intersection(gt_nodes)
        if len(v) > 0:
            pred_div.append(list(v))
    fo_nodes = list(all_nodes - set(nodes))
    pred_div.append(fo_nodes)


    print('n_node: ', len(all_nodes))
    print('n_edge: ', len(edges))

    print('n_clusters gt: ', len(gt_cmty))
    print('n_clusters pred: ', len(pred_div))
    end = datetime.now().strftime("%m/%d %H:%M:%S")
    start_time = datetime.strptime(start, "%m/%d %H:%M:%S")
    end_time = datetime.strptime(end, "%m/%d %H:%M:%S")
    time_diff = end_time - start_time
    seconds = time_diff.total_seconds()

    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    print(f"time consuming:{seconds}")
    pred_path = f"../eva_txt/fox/{name}_pre.txt"
    with open(pred_path, 'w', encoding='utf8') as f:
        for c in pred_div:
            f.write(" ".join([str(n) for n in c]))
            f.write("\n")
        f.flush()

    target_path = f"../eva_txt/fox/{name}_gt.txt"
    with open(target_path, 'w', encoding='utf8') as f:
        for c in gt_cmty:
            f.write(" ".join([str(n) for n in c]))
            f.write("\n")
        f.flush()

    nmi = onmi.overlapping_normalized_mutual_information(pred_div, gt_cmty,data_name)
    print('onmi: ', nmi, flush=True)

    f1 = xmeasures.f1(pred_div, gt_cmty,data_name)
    print('f1: ', f1)

    print("-" * 20)


if __name__ == "__main__":
    for name in [ "amazon" ,"youtube", "dblp","LiveJournal","orkut", "friendster","wiki"]:
        print("exp on dataset", name)
        run_experiment(name)

    exit(0)
