

#DATASETS [Edges, Ground Truth, Overlapping, Weighted, Directed]
DATASETS={
    "amazon":["../graph_dataset/com-amazon/com-amazon.ungraph.txt","../graph_dataset/com-amazon/com-amazon.all.dedup.cmty.txt", True, False, False],
    "youtube":["../graph_dataset/youtube/com-youtube.ungraph.txt","../graph_dataset/youtube/com-youtube.all.cmty.txt", True, False, False],
    "LiveJournal":["../graph_dataset/LiveJournal/com-lj.ungraph.txt","../graph_dataset/LiveJournal/com-lj.all.cmty.txt", True, False, False],
    "dblp":["../graph_dataset/dblp/com-dblp.ungraph.txt","../graph_dataset/dblp/com-dblp.all.cmty.txt", True, False, False],
    "orkut":["../graph_dataset/orkut/com-orkut.ungraph.txt","../graph_dataset/orkut/com-orkut.all.cmty.txt", True, False, False],
    "friendster":["../graph_dataset/friendster/com-friendster.ungraph.txt","../graph_dataset/friendster/com-friendster.all.cmty.txt", True, False, False],
    "tweet2012":["../graph_dataset/tweet2012/awgraph.txt","../graph_dataset/tweet2012/cmty.txt",  False, True, False ],
    "tweet2018":["../graph_dataset/tweet2018/awgraph.txt","../graph_dataset/tweet2018/cmty.txt", False, True, False ],
    "wiki":["../graph_dataset/wiki/com-Wiki.ungraph.txt","../graph_dataset/wiki/com-Wiki.all.cmty.txt",True,False,True],
    "lfr_overlap":["../graph_dataset/lfr_overlap/lfr_graph.txt","../graph_dataset/lfr_overlap/lfr_gt.txt", True, False, False],
}

def read_ture_cluster(path,name):
    cmty = []
    keep_nodes = []
    cmty_num = 0
    with open(path) as f:
        for line in f:
            if line.startswith('#') : continue
            nodes = line.rstrip('\n').split('\t')
            # nodes = line.rstrip('\n').split(' ')
            nodes = [int(n) for n in nodes]
            cmty_num += 1
            if name == "tweet2012" or name =="tweet2018" or name == "wiki":
                cmty.append(nodes)
                keep_nodes += nodes
            else:
                if len(nodes)>2:
                    cmty.append(nodes)
                    keep_nodes += nodes

    return cmty, set(keep_nodes),cmty_num

def filter_nodes(division,gt_nodes):
    pred_comm = []
    if type(division) == list:
        for v in division:
            v = gt_nodes.intersection(v)
            if len(v) > 0:
                pred_comm.append(v)
    else:
        for k, v in division.items():
            v = gt_nodes.intersection(v)
            if len(v) > 0:
                pred_comm.append(v)
    return pred_comm
















