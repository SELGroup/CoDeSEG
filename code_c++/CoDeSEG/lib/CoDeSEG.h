#ifndef CODESEG_H
#define CODESEG_H

#include <future>
#include <vector>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include "DynamicArray.h"

/**
 * Type aliases for node, edge, adjacency matrix, etc.
 */
typedef std::string Node;
typedef unsigned long NodeIdx;
typedef std::tuple<Node, Node, float> Edge;
typedef DynamicArray<Node> NodeArray;
typedef DynamicArray<Edge> EdgeArray;
typedef std::unordered_map<Node, NodeIdx> NodeIdxMap;
typedef DynamicArray<NodeIdx> NodeIdxArray;
typedef std::unordered_map<NodeIdx, float> NodeAdj;
typedef DynamicArray<NodeAdj> AdjArray;
typedef DynamicArray<float> FloatArray;
typedef DynamicArray<unsigned short> UShortArray;
typedef float FloatValue;
typedef std::unordered_map<unsigned long, std::set<std::string> > CmtyMap;
typedef std::set<unsigned long> CmtyIdxSet;
typedef DynamicArray<std::set<unsigned long> > NodeCmtyOvlp;
typedef std::vector<std::set<std::string> > CmtySet;
typedef std::future<std::tuple<float, unsigned long, unsigned long> > TaskReturn;


class CoDeSEG {
    /**
     * Private members
     */
    NodeArray nodes; // Array of node names
    NodeIdxMap node_idx; // Map of node name to node index
    AdjArray graph_adj; // Adjacency matrix
    AdjArray graph_adj_out; // Adjacency matrix of out-degree in directed graph

    FloatValue graph_vol = 0.; // Graph volume, also known as the sum of edges weights
    FloatValue se_1d = 0.; // 1D SE of graph 
    FloatValue avg_se_1d = 0.; // Avg. 1D SE of each node 
    FloatArray cmty_vol; // Community volumes
    FloatArray cmty_cut; // Sum of cut edge weights of each node i to other community
    FloatArray node_deg; // Degree (sum of edges weights) of each node i

    FloatArray cmty_se_sum; // Structural entropy delta sum for nodes in communities
    UShortArray cmty_version; // Version of communities

    NodeIdxArray cmty_size; // Size of each community
    NodeIdxArray node_cmty; // Array of node community index
    NodeCmtyOvlp node_cmty_ovlp; // Array of node community index

    std::mutex stat_mutex; // synchronization

    CmtyMap cmtis; // Set of communities

public:
    /**
     * The default constructor
     */
    CoDeSEG() = default;

    /**
     * Add edges to the graph instance
     * @param edges Edge Arrary
     * @return The reference of this instance
     */
    CoDeSEG &add_edges(EdgeArray &edges, bool direct = false);

    /**
     * Add an edge to this CoDeSEG graph, and encode nodes to indice
     * @param src The source node of edge
     * @param tgt The target node of edge
     * @param weight The edge weight, default value is 1.0
     * @return The reference of this instance
     */
    CoDeSEG &add_edge(const Node &src, const Node &tgt, float weight = 1.0, bool direct = false);

    /**
     * Run the community detection algorithm
     * @param max_iter The max limit iteration 
     * @param overlapping If true, detect overlapping communities.
     * @param verbose Produce verbose output.
     * @return The reference of this instance
     */
    CoDeSEG &detect_cmty(unsigned int max_iter, bool overlapping, float tau, float alpha, bool verbose = false);

    CoDeSEG &detect_cmty(unsigned int num_worker, unsigned int max_iter,
                         bool overlapping, float tau, float alpha, bool verbose);

    CoDeSEG &detect_cmty_direct(unsigned int max_iter, bool overlapping, float tau, float alpha, bool verbose = false);

    CoDeSEG &detect_cmty_direct(unsigned int num_worker, unsigned int max_iter,
                         bool overlapping, float tau, float alpha, bool verbose);

    /**
     * The community labels for each node
     * @return The array of node community labels
     */
    const NodeIdxArray &labels() { return node_cmty; }

    /**
     * Get the set of communities
     * @return The set of communities
     */
    CmtyMap &communities();

    /**
     * Get graph node array
     * @return Node array
     */
    const NodeArray &get_nodes() { return nodes; }

protected:
    /**
     * Initialize each node as anindividual community.
     */
    void _init_cluster();

    /**
     * Transfer a node i from a source community to a target community.
     * @param i A graph node index
     * @param src_cmty The source community index
     * @param deg_i_src_cmty The sum of edges from node i to the source community
     * @param tgt_cmty The target community index
     * @param deg_i_tgt_cmty The sum of edges from node i to the target community
     */
    void _transfer(NodeIdx i,
                   NodeIdx src_cmty, float deg_i_src_cmty,
                   NodeIdx tgt_cmty, float deg_i_tgt_cmty);
    void _direct_transfer(NodeIdx i,
                   NodeIdx src_cmty, float deg_i_src_cmty_in, float deg_i_src_cmty_out,
                   NodeIdx tgt_cmty, float deg_i_tgt_cmty_in, float deg_i_tgt_cmty_out);

    /**
     * Compute the structural entropy delta when node i leave its community
     * @param i A graph node index
     * @param cmty_idx The community index
     * @param deg_i_cmty The sum of edges from node i to the community
     * @param i_in_cmty An indicator if i is in the community
     * @return The structural entropy delta
     */
    float _delta_leave(NodeIdx i, NodeIdx cmty_idx, FloatValue deg_i_cmty, bool i_in_cmty = true);
    float _direct_delta_leave(NodeIdx i, NodeIdx cmty_idx, FloatValue deg_i_cmty,  FloatValue deg_i_cmty_out, bool i_in_cmty = true);

    /**
     * Compute the strategy for node i
     * @param i A graph node index
     * @return A 7-tuple of
     * <
     * 1. The structural entropy delta,
     * 2. The entropy delta for leave,
     * 3. The entropy delta for join,
     * 4. The source community index,
     * 5. The sum of edges from node i to the source community
     * 6. The target community index
     * 7. The sum of edges from node i to the target community
     * >
     */
    std::tuple<float, float, float, unsigned long, float, unsigned long, float> _node_strategy(NodeIdx i);
    std::tuple<float, float, float, unsigned long, float, float, unsigned long, float, float> _direct_node_strategy(const NodeIdx i);

    /**
     * try to overlap node to neighbor communities
     * @param i node index
     * @param multi_thread run in multi thread env
     * @return
     */
    std::tuple<float, unsigned long> _ovlp_node(NodeIdx i, float alpha, bool multi_thread=false);
    std::tuple<float, unsigned long> _direct_ovlp_node(NodeIdx i, float alpha, bool multi_thread=false);

    /**
     * Get the index of the node. If the name is not in the graph create
     * a new index for it.
     * @param node The node name
     * @return The node index
     */
    NodeIdx _add_or_get_node(const Node &node, bool direct = false);

   
};


#endif //CODESEG_H
