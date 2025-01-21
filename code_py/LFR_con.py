from collections import defaultdict


def read_edge_and_write_to_txt(edge_file_path, output_txt_path):
    with open(edge_file_path, 'r') as nse_file:
        lines = nse_file.readlines()

    
    with open(output_txt_path, 'w') as output_file:
        for line in lines[1:]:
            
            if line.strip(): 
                parts = line.strip().split('\t')
                if len(parts) >= 2:  
                    edge = f"{parts[0]}\t{parts[1]}"               
                    output_file.write(edge + '\n') 

def read_com_and_write_to_txt(com_file_path, output_txt_path):
    with open(com_file_path, 'r') as com_file:
        lines = com_file.readlines()
    comms= defaultdict(list)
    for line in lines:
        parts = line.strip().split()
        node = parts[0] 
        communities = parts[1:] 

        for community in communities:
            comms[community].append(node)
    with open(output_txt_path, 'w') as output_file:
        for community, nodes in comms.items():
            output_file.write("\t".join(nodes) + '\n')


if __name__ == '__main__':

    edge_file_path = '../graph_dataset/lfr_overlap/overlap_lfr_100_1.nse' 
    com_file_path = '../graph_dataset/lfr_overlap/overlap_lfr_100_1.nmc'  
    output_edge_path = '../graph_dataset/lfr_overlap/lfr_graph.txt'    
    output_com_path = '../graph_dataset/lfr_overlap/lfr_gt.txt'     
    read_edge_and_write_to_txt(edge_file_path, output_edge_path)
    read_com_and_write_to_txt(com_file_path, output_com_path)
