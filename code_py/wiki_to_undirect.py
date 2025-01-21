import os

if __name__ == '__main__':
    edges = "../graph_dataset/wiki-topcats/wiki-topcats.txt"
    rewrite_edge_file = "../graph_dataset/wiki(rewrite)/com-Wiki.ungraph.txt"
    ground_truth = "../graph_dataset/wiki-topcats/wiki-topcats-categories.txt"
    rewrite_label_file = "../graph_dataset//wiki(rewrite)/com-Wiki.all.cmty.txt"
    output_dir = os.path.dirname(rewrite_edge_file)
    os.makedirs(output_dir, exist_ok=True)

    with open(ground_truth) as l_f:
        with open(rewrite_label_file, 'w') as l_out_f:
            current_community = []
            for line in l_f:
                if line.startswith('#'): continue

                start = line.find(';')
                line = line[start + 2:]
                if line == "\n": continue

                nodes = line.rstrip('\n').split(' ')
                if len(nodes) > 1:
                    l_out_f.write('\t'.join(nodes) + '\n')



    with open(edges) as f:
        with open(rewrite_edge_file, 'w') as out_f:
            out_f.write("#Wiki\n")
            out_f.write("# Directed Graph\n")
            out_f.write("# Nodes: 1791489    Edges: 28511807\n")
            out_f.write("# SrcNId\tDstNId\n")
            for line in f:
                if line.startswith('#'): continue
                nodes = line.rstrip('\n').split(' ')
                if len(nodes) > 1:
                    out_f.write(f"{nodes[0]}\t{nodes[1]}\n")



