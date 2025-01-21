
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

    FloatValue graph_vol = 0.; // Graph volume, also known as the sum of edges weights
    FloatValue graph_se_max = 0.;

    FloatArray cmty_vol; // Community volumes
    FloatArray cmty_cut; // Sum of cut edge weights of each node i to other community
    FloatArray node_deg; // Degree (sum of edges weights) of each node i

    FloatArray cmty_se_sum; // Structural entropy delta sum for nodes in communities
    UShortArray cmty_version; // Version of communities

    NodeIdxArray cmty_size; // Size of each community
    NodeIdxArray node_cmty; // Array of node community index
    NodeCmtyOvlp node_cmty_ovlp; // Array of node community index

    std::vector<std::unique_ptr<std::mutex> > stats_mutex; // synchronization
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
    CoDeSEG &add_edges(EdgeArray &edges);

    /**
     * Run the community detection algorithm
     * @param max_iter The max limit iteration 
     * @param overlapping If true, detect overlapping communities.
     * @param verbose Produce verbose output.
     * @param se_threshold
     * @return The reference of this instance
     */
    CoDeSEG &detect_cmty(unsigned int max_iter, bool overlapping, bool verbose = false, float se_threshold = 0.25);

    CoDeSEG &detect_cmty(unsigned int num_worker, unsigned int max_iter, bool overlapping,
                         bool verbose, float se_threshold = 0.25);

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

    /**
     * Compute the structural entropy delta when node i leave its community
     * @param i A graph node index
     * @param cmty_idx The community index
     * @param deg_i_cmty The sum of edges from node i to the community
     * @param i_in_cmty An indicator if i is in the community
     * @return The structural entropy delta
     */
    float _delta_leave(NodeIdx i, NodeIdx cmty_idx, FloatValue deg_i_cmty, bool i_in_cmty = true);

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

    /**
     * try to overlap node to neighbor communities
     * @param i node index
     * @param multi_thread run in multi thread env
     * @return
     */
    std::tuple<float, unsigned long> _ovlp_node(NodeIdx i, bool multi_thread = false);
};


#endif //CODESEG_H
