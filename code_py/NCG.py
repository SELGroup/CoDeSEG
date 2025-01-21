#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from datetime import datetime

import numpy as np
import copy as CPY

import pandas as pd

import onmi
import xmeasures
from comm_utils import read_ture_cluster, DATASETS, filter_nodes
from draw import coms_to_csv


#similarity types:
    # MN : Meet of Neighbors
    # JC : Jaccard Coefficient
    # SI : Saltin Index
    # SO : Sorensen Index
    # HP : Hub Promoted Index
    # HD : Hub Depressed Index
    # CUSTOM : Custom Definition

def DETECT_COMMUNITIES(adj_list_name,gt_comm,name, stopCRITERION=0.01,similarity_type='HP'):
    def rewrite_ADJACENCY_LIST_func(g_path):
        if name == "wiki":
            df = pd.read_csv(g_path, delimiter='\t', skiprows=4, header=None, names=['FromNodeId', 'ToNodeId'])
            df['FromNodeId'], df['ToNodeId'] = np.minimum(df['FromNodeId'], df['ToNodeId']), np.maximum(
                df['FromNodeId'], df['ToNodeId'])

            df = df.drop_duplicates()

        else:
            df = pd.read_csv(g_path, delimiter='\t', skiprows=4, header=None, names=['FromNodeId', 'ToNodeId'])
        nodes = []
        nodes_idx = {}
        neigh = {}
        for a, b in df.values:
            if a not in neigh:
                nodes_idx[a] = len(nodes)
                nodes.append(a)
                neigh[a] = []

            if b not in neigh:
                nodes_idx[b] = len(nodes)
                nodes.append(b)
                neigh[b] = []

            neigh[a].append(nodes_idx[b])
            neigh[b].append(nodes_idx[a])
        return list(neigh.values()), nodes, df.values


    def nodes_similarity_calculator_func(var_adj,var_deg,var_type):
        var_allnodes_similarity=[]
        num = len(var_adj)
        for var_node, var_neighs in enumerate(var_adj):
            var_eachnode_similarity_list=[]
            var_node_nei_set = set(var_neighs)
            for var_neigh in var_neighs:
                var_sim= len(var_node_nei_set.intersection(var_adj[var_neigh]))
                var_sim = var_sim / min(var_deg[var_node], var_deg[var_neigh])
                # if var_type=='MN': var_sim=var_sim/1
                # if var_type=='SI': var_sim=var_sim/np.sqrt(var_deg[var_node]*var_deg[var_neigh])
                # if var_type=='SO': var_sim=2.0*var_sim/(var_deg[var_node]+var_deg[var_neigh])
                # if var_type=='JC': var_sim=var_sim/(var_deg[var_node]+var_deg[var_neigh]-var_sim)
                # if var_type=='HP': var_sim=var_sim/min(var_deg[var_node],var_deg[var_neigh])
                # if var_type=='HD': var_sim=var_sim/max(var_deg[var_node],var_deg[var_neigh])
                #if var_type=='CUSTOM': var_sim= Custom Definition
                var_eachnode_similarity_list.append(var_sim)
            var_allnodes_similarity.append(var_eachnode_similarity_list)
        return var_allnodes_similarity
           
    def U1_func(var_adj,var_S,var_similarity,var_deg,var1):
        var_pay=0
        var1_s = var_S[var1]
        neighs = var_adj[var1]
        var1_sim = var_similarity[var1]
        for neigh, sim in zip(neighs,var1_sim):
            if var1_s == var_S[neigh]:
                var_pay += sim +1
        # for i in range(var_deg[var1]):
        #     if var_S[var1]==var_S[var_adj[var1][i]]:
        #         var_pay+= (  var_similarity[var1][i]  +1)
        return var_pay
    
    def U2_func(var_adj,var_S,var_similarity,var_deg,var1):
        var_pay=0
        neighs = var_adj[var1]
        var1_sim = var_similarity[var1]
        var1_set = set(var_S[var1])
        for neigh, sim in zip(neighs, var1_sim):
            var_s_nei = var_S[neigh]
            shared = len(var1_set.intersection(var_s_nei))
            if shared >0:
                var_pay += (sim +1)/ len(var_s_nei)**0.5
        # for i in range(var_deg[var1]):
        #     var_w=( var_similarity[var1][i]  + 1 )
        #     shared=len(set(var_S[var1]) & set(var_S[var_adj[var1][i]]))
        #     if shared>0:
        #         var_pay+= var_w / (len(set(var_S[var_adj[var1][i]])))**(0.5)
        return var_pay

    
    def thresh_calculator_func(var_MB):
        #var_MB : membership coefficients 
        return np.sqrt(np.mean(np.array(var_MB)**2))
    
    #>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    start = datetime.now().strftime("%m/%d %H:%M:%S")
    #<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<START

    GRAPH_adj, dict2,edges = rewrite_ADJACENCY_LIST_func(adj_list_name)
    DEG=[]
    for nodesneigh in GRAPH_adj: DEG.append(len(nodesneigh))
    DEG=np.array(DEG)  #nodes degree list
    n=len(GRAPH_adj) #total number of nodes

    #>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Initialization
    nodes_similarity=nodes_similarity_calculator_func(GRAPH_adj,DEG,similarity_type) 
    S=np.arange(n,dtype=int)
    # for i in range(n):
    #     S[i]=i
    fix = 0
    Sprev = S + 0
    Iteration = 0
    diff = np.sum(S == Sprev)
    print("Non-overlapping:")
    while fix < 1 and Iteration < 2:
        Iteration += 1
        for step in range(n):
            node = (step + 0) % n
            neighs = GRAPH_adj[node]
            U_List = []
            G1 = [S[node]]
            for i in neighs:
                G1.append(S[i])
            G1 = list(set(G1))
            for i in G1:
                saveparam = S[node] + 0
                S[node] = i + 0
                U = U1_func(GRAPH_adj, S, nodes_similarity, DEG, node)
                S[node] = saveparam + 0
                U_List.append(U)
            select = G1[U_List.index(max(U_List))]
            S[node] = select
        if abs(np.sum(S == Sprev) - diff) <= int(stopCRITERION * n):
            fix += 1
        else:
            fix = 0
        diff = np.sum(S == Sprev)
        Sprev = S + 0
    # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Phase2
    SS = []
    for i in S:
        SS.append([i])

    COM_ID = []  # Community IDs which nodes belong to
    MEMB_COE = []  # membership coefficient of nodes
    thresh = []
    for node in range(n):
        print(f"\rOverlapping: {node/n*100 :.2f} %",end="",flush=True)
        neighs = GRAPH_adj[node]
        U_List = []
        G1 = CPY.copy(SS[node])
        for i in neighs:
            G1 += SS[i]
        G1 = list(set(G1))
        for i in G1:
            saveparam = SS[node] + []
            SS[node] = [i]
            U = U2_func(GRAPH_adj, SS, nodes_similarity, DEG, node)
            SS[node] = saveparam + []
            U_List.append(U)
        U_List = np.array(U_List)
        memb_coe = U_List / max(U_List)
        com_id = G1
        MEMB_COE.append(memb_coe)
        COM_ID.append(com_id)
        thresh.append(thresh_calculator_func(memb_coe))
    crisp_comms_dict = {}
    nodes_total_membership = np.zeros(n)

    SS = []
    for node in range(n):
        newS = []
        for i in range(len(MEMB_COE[node])):
            if MEMB_COE[node][i] >= thresh[node]:
                try:
                    crisp_comms_dict[COM_ID[node][i]] += [node]
                except:
                    crisp_comms_dict[COM_ID[node][i]] = [node]
                nodes_total_membership[node] += 1
                newS.append(COM_ID[node][i])
        SS.append(newS)

    crisp_comms = []
    for i in crisp_comms_dict:
        crisp_comms.append(crisp_comms_dict[i])
    COMMUNITY_STRUCTURE = []
    for line in crisp_comms:
        COMMUNITY_STRUCTURE.append([dict2[key] for key in line])
        
    ture_comm, keep_nodes, _  = read_ture_cluster(gt_comm,name)

    pre_comm = filter_nodes(COMMUNITY_STRUCTURE,keep_nodes)

    print('n_clusters gt: ', len(ture_comm))
    print('n_clusters pred: ', len(pre_comm))
    end = datetime.now().strftime("%m/%d %H:%M:%S")
    start_time = datetime.strptime(start, "%m/%d %H:%M:%S")
    end_time = datetime.strptime(end, "%m/%d %H:%M:%S")
    time_diff = end_time - start_time
    seconds = time_diff.total_seconds()

    print(datetime.now().strftime("%m/%d %H:%M:%S"))
    print(f"time consuming:{seconds}")

    pred_path = f"../eva_txt/ncg/{name}_pre.txt"
    with open(pred_path, 'w', encoding='utf8') as f:
        for c in pre_comm:
            f.write(" ".join([str(n) for n in c]))
            f.write("\n")
        f.flush()

    target_path = f"../eva_txt/ncg/{name}_gt.txt"
    with open(target_path, 'w', encoding='utf8') as f:
        for c in ture_comm:
            f.write(" ".join([str(n) for n in c]))
            f.write("\n")
        f.flush()
    coms_to_csv(pre_comm, "Ncg")
    nmi = onmi.overlapping_normalized_mutual_information(pre_comm, ture_comm, name)
    print('onmi: ', nmi, flush=True)

    f1 = xmeasures.f1(pre_comm, ture_comm, name)
    print('f1: ', f1)


if __name__ == '__main__':
    for name in [ "amazon" ,"youtube", "dblp","LiveJournal","orkut", "friendster","wiki"]:
        print("-" * 40)
        print(f"dataset:{name}")
        DETECT_COMMUNITIES(DATASETS[name][0],DATASETS[name][1],name)